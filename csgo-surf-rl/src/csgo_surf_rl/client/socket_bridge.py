"""SocketClientBridge: the harness end of the telemetry link.

Speaks the newline-framed JSON protocol in plugin/PROTOCOL.md to the in-game
telemetry plugin. The harness listens; the plugin connects when the map
loads. Because the wire `state`/`cmd` objects are exactly ClientState /
UserCmd dicts, this bridge is a drop-in GameClientBridge — the env,
recorder, and training loops are identical to the mock-bridge setup.

Protocol invariant maintained here: after reset()/step() returns, the plugin
is blocked waiting for our reply to the state we just returned. The next
step()/reset() supplies that reply. This keeps the exchange strictly
alternating (state, reply, state, reply, ...).
"""

from __future__ import annotations

import json
import socket

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.client.protocol import DEFAULT_PORT, PROTOCOL_VERSION


class SocketClientBridge(GameClientBridge):
    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = DEFAULT_PORT,
        accept_timeout: float | None = None,
    ) -> None:
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind((host, port))
        self._listener.listen(1)
        self._accept_timeout = accept_timeout

        self._conn: socket.socket | None = None
        self._rxbuf = b""
        self._awaiting_reply = False
        self._tick_interval = 1.0 / 64.0
        self._failed = False
        self._completed = False

    # -- GameClientBridge ---------------------------------------------------

    def reset(self, command: str | None = None) -> ClientState:
        """Restart the run. `command` overrides the plugin's configured reset
        console command for this reset only (e.g. a specific teleport)."""
        self._ensure_connected()
        # Respond to any state the plugin is currently blocked on, so our
        # reset lands as its reply; if nothing is pending, pull one first.
        if not self._awaiting_reply:
            self._read_state()
        message: dict = {"type": "reset"}
        if command is not None:
            message["command"] = command
        self._send(message)
        self._awaiting_reply = False
        state = self._read_state()  # first state of the fresh run
        return state

    def step(self, cmd: UserCmd, command: str | None = None) -> ClientState:
        """Send this tick's UserCmd to the client. An optional `command` is a
        console command run the same tick (`;`-separate to batch several)."""
        if not self._awaiting_reply:
            raise RuntimeError("step() called before reset() established a pending state")
        message: dict = {"type": "cmd", "cmd": cmd.clamped().to_dict()}
        if command is not None:
            message["command"] = command
        self._send(message)
        self._awaiting_reply = False
        return self._read_state()

    def send_command(self, command: str) -> ClientState:
        """Send a console command to the client without a movement input.

        Intended for setup/control between episodes (teleport, cvars, load a
        map), not for interleaving inside the RL action loop: the tick this
        replies to gets no model movement (the engine's own command stands).
        Returns the next client state so the caller stays in sync.
        """
        if not self._awaiting_reply:
            self._read_state()
        self._send({"type": "command", "command": command})
        self._awaiting_reply = False
        return self._read_state()

    def is_run_failed(self) -> bool:
        return self._failed

    def is_run_complete(self) -> bool:
        return self._completed

    @property
    def tick_interval(self) -> float:
        return self._tick_interval

    def close(self) -> None:
        for sock in (self._conn, self._listener):
            if sock is not None:
                try:
                    sock.close()
                except OSError:
                    pass
        self._conn = None

    def __enter__(self) -> "SocketClientBridge":
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    # -- transport ----------------------------------------------------------

    def _ensure_connected(self) -> None:
        if self._conn is not None:
            return
        if self._accept_timeout is not None:
            self._listener.settimeout(self._accept_timeout)
        try:
            conn, _ = self._listener.accept()
        except socket.timeout as exc:  # pragma: no cover - timing dependent
            raise TimeoutError("no telemetry plugin connected within accept_timeout") from exc
        conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self._conn = conn
        self._rxbuf = b""
        self._awaiting_reply = False
        hello = self._read_message()
        if hello.get("type") != "hello":
            raise RuntimeError(f"expected hello handshake, got {hello!r}")
        if hello.get("protocol") not in (None, PROTOCOL_VERSION):
            raise RuntimeError(
                f"plugin protocol {hello.get('protocol')} != harness {PROTOCOL_VERSION}"
            )
        self._tick_interval = float(hello.get("tick_interval", self._tick_interval))

    def _send(self, message: dict) -> None:
        assert self._conn is not None
        self._conn.sendall((json.dumps(message) + "\n").encode("utf-8"))

    def _read_message(self) -> dict:
        assert self._conn is not None
        while b"\n" not in self._rxbuf:
            chunk = self._conn.recv(4096)
            if not chunk:
                raise ConnectionError("telemetry plugin disconnected")
            self._rxbuf += chunk
        line, self._rxbuf = self._rxbuf.split(b"\n", 1)
        return json.loads(line.decode("utf-8"))

    def _read_state(self) -> ClientState:
        message = self._read_message()
        if message.get("type") != "state":
            raise RuntimeError(f"expected state frame, got {message!r}")
        self._failed = bool(message.get("failed", False))
        self._completed = bool(message.get("completed", False))
        self._awaiting_reply = True
        return ClientState.from_dict(message["state"])
