"""Recorder: captures demonstration data at the client boundary.

Records (ClientState, UserCmd) pairs through the exact same
GameClientBridge interface the RL environment uses, so demonstration data
is by construction in the same domain as the model's inputs and outputs.

The UserCmds can come from any controller — a human pilot's replayed
inputs, a demo-file parser emitting the usercmds it decoded, or a scripted
baseline policy.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Callable, Iterator

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd

# A controller maps the current client state to the next tick's input.
Controller = Callable[[ClientState], UserCmd]


class Recorder:
    def __init__(self, bridge: GameClientBridge, out_path: str | Path) -> None:
        self._bridge = bridge
        self._out_path = Path(out_path)

    def record_episode(self, controller: Controller, max_ticks: int) -> int:
        """Roll out one episode, appending one JSON line per transition.

        Returns the number of transitions written. Each line stores the state
        the controller saw, the command it issued, and the resulting state's
        terminal flags — everything BC and offline RL need.
        """
        self._out_path.parent.mkdir(parents=True, exist_ok=True)
        state = self._bridge.reset()
        written = 0
        with self._out_path.open("a", encoding="utf-8") as f:
            for _ in range(max_ticks):
                cmd = controller(state)
                next_state = self._bridge.step(cmd)
                failed = self._bridge.is_run_failed()
                completed = self._bridge.is_run_complete()
                f.write(
                    json.dumps(
                        {
                            "state": state.to_dict(),
                            "cmd": cmd.to_dict(),
                            "next_state": next_state.to_dict(),
                            "failed": failed,
                            "completed": completed,
                        }
                    )
                    + "\n"
                )
                written += 1
                state = next_state
                if failed or completed:
                    break
        return written


def read_transitions(path: str | Path) -> Iterator[dict]:
    """Yield raw transition dicts from a JSONL recording."""
    with Path(path).open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                yield json.loads(line)
