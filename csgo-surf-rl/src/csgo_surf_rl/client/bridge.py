"""GameClientBridge: the single boundary between the harness and a client.

Every consumer — the RL environment, the demonstration recorder, and the
evaluator — talks to a client exclusively through this interface. Because
recording and acting share one boundary, training data and model outputs
are guaranteed to come from the same domain.

Concrete implementations might wrap a local offline server with a telemetry
plugin, a demo-file replayer (for demonstration data), or the pure-Python
MockSurfBridge used for tests and offline development.
"""

from __future__ import annotations

from abc import ABC, abstractmethod

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd


class GameClientBridge(ABC):
    """Tick-synchronous access to a game client."""

    @abstractmethod
    def reset(self) -> ClientState:
        """Restart the run (teleport to course start) and return the state."""

    @abstractmethod
    def step(self, cmd: UserCmd) -> ClientState:
        """Submit one tick of input and return the resulting client state."""

    @abstractmethod
    def is_run_failed(self) -> bool:
        """True if the run is unrecoverable (fell off the course, teleported back)."""

    @abstractmethod
    def is_run_complete(self) -> bool:
        """True if the client reached the course end zone."""

    @property
    @abstractmethod
    def tick_interval(self) -> float:
        """Seconds per tick (e.g. 1/64 for 64-tick)."""

    def close(self) -> None:
        """Release any client connection. Default: nothing to release."""
