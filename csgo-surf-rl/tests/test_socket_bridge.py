"""Exercises SocketClientBridge against a scripted fake plugin over a real
loopback socket, so the wire protocol, handshake, and the strict
state/reply alternation are covered without a running game."""

import json
import socket
import threading

import numpy as np

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.socket_bridge import SocketClientBridge
from csgo_surf_rl.client.usercmd import UserCmd


def _state_line(tick, x, failed=False, completed=False):
    state = ClientState(
        tick=tick,
        origin=(float(x), 0.0, 0.0),
        velocity=(250.0, 0.0, 0.0),
        view_yaw=0.0,
        view_pitch=0.0,
        on_ground=False,
    )
    return json.dumps({"type": "state", "state": state.to_dict(), "failed": failed, "completed": completed}) + "\n"


class FakePlugin:
    """Minimal plugin: connect, say hello, then answer each reply with the
    next state frame. Records the replies it received for assertions."""

    def __init__(self, host, port):
        self.sock = socket.create_connection((host, port))
        self.f = self.sock.makefile("rwb")
        self.received = []
        self._x = 0.0
        self._tick = 0
        self.f.write(json.dumps({"type": "hello", "tick_interval": 1 / 64, "protocol": 1}).encode() + b"\n")
        self.f.flush()

    def send_state(self, **kwargs):
        self.f.write(_state_line(self._tick, self._x, **kwargs).encode())
        self.f.flush()
        self._tick += 1
        self._x += 100.0

    def read_reply(self):
        line = self.f.readline()
        msg = json.loads(line.decode())
        self.received.append(msg)
        return msg

    def close(self):
        self.f.close()
        self.sock.close()


def test_handshake_reset_and_step_alternation():
    bridge = SocketClientBridge(port=0)  # OS-assigned free port
    port = bridge._listener.getsockname()[1]

    plugin_box = {}

    def run_plugin():
        plugin = FakePlugin("127.0.0.1", port)
        plugin_box["plugin"] = plugin
        # reset(): bridge pulls a state, then replies {"type":"reset"}, then
        # reads the post-reset state.
        plugin.send_state()  # state the bridge reads first
        reset_msg = plugin.read_reply()
        assert reset_msg["type"] == "reset"
        plugin.send_state()  # post-reset opening state
        # two steps: each reply is a cmd, each answered with a new state
        for _ in range(2):
            cmd_msg = plugin.read_reply()
            assert cmd_msg["type"] == "cmd"
            plugin.send_state(completed=(len(plugin.received) == 3))

    t = threading.Thread(target=run_plugin)
    t.start()
    try:
        obs0 = bridge.reset()
        assert isinstance(obs0, ClientState)
        assert bridge.tick_interval == 1 / 64

        s1 = bridge.step(UserCmd(sidemove=450.0))
        assert not bridge.is_run_complete()
        s2 = bridge.step(UserCmd(yaw_delta=1.0))
        # x advances between states, confirming ordering held
        assert s2.origin[0] > s1.origin[0]
        assert bridge.is_run_complete()
    finally:
        t.join(timeout=5)
        plugin_box["plugin"].close()
        bridge.close()

    # The plugin saw exactly: reset, cmd, cmd — the alternation we expect.
    types = [m["type"] for m in plugin_box["plugin"].received]
    assert types == ["reset", "cmd", "cmd"]
