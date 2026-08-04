"""UserCmd: the action domain.

This mirrors the per-tick input structure a Source-engine client submits to
the engine. The policy's output layer maps 1:1 onto this structure, and
recorded demonstrations store exactly this structure — so the model is
trained on the same domain it acts in.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntFlag

import numpy as np

# Source-engine movement axis limit (units/s) for cl_forwardspeed/cl_sidespeed.
MAX_MOVE_SPEED = 450.0

# Per-tick view-angle delta clamp (degrees). Generous enough for fast surf
# turns at 64 tick; keeps the action space bounded for the policy.
MAX_ANGLE_DELTA_PER_TICK = 30.0


class Buttons(IntFlag):
    """Subset of IN_* button flags relevant to surf movement."""

    NONE = 0
    JUMP = 1 << 0
    DUCK = 1 << 1
    ATTACK = 1 << 2
    USE = 1 << 3


@dataclass(frozen=True)
class UserCmd:
    """One tick of client input.

    View angles are expressed as *deltas* (mouse movement), not absolute
    angles, because that is what the physical input device produces and it
    keeps the action distribution translation-invariant in yaw.
    """

    yaw_delta: float = 0.0  # degrees, +left
    pitch_delta: float = 0.0  # degrees, +down
    forwardmove: float = 0.0  # units/s, +forward (W/S axis)
    sidemove: float = 0.0  # units/s, +right (A/D axis)
    upmove: float = 0.0  # units/s, +up (jump/crouch analog axis)
    buttons: Buttons = Buttons.NONE
    impulse: int = 0  # one-shot impulse input (e.g. 101); 0 = none

    # Flat vector layout shared by to_array/from_array and the policy head.
    # upmove/impulse are valid client inputs the harness may send but are
    # deliberately NOT in the learned action vector — the button axes already
    # cover jump/duck, and impulse is a scripted one-shot, not a policy output.
    VECTOR_SIZE = 6

    def clamped(self) -> "UserCmd":
        """Return a copy with all fields clamped to engine-legal ranges."""
        return UserCmd(
            yaw_delta=float(np.clip(self.yaw_delta, -MAX_ANGLE_DELTA_PER_TICK, MAX_ANGLE_DELTA_PER_TICK)),
            pitch_delta=float(np.clip(self.pitch_delta, -MAX_ANGLE_DELTA_PER_TICK, MAX_ANGLE_DELTA_PER_TICK)),
            forwardmove=float(np.clip(self.forwardmove, -MAX_MOVE_SPEED, MAX_MOVE_SPEED)),
            sidemove=float(np.clip(self.sidemove, -MAX_MOVE_SPEED, MAX_MOVE_SPEED)),
            upmove=float(np.clip(self.upmove, -MAX_MOVE_SPEED, MAX_MOVE_SPEED)),
            buttons=self.buttons,
            impulse=int(self.impulse),
        )

    def to_array(self) -> np.ndarray:
        """Normalized action vector in [-1, 1] (buttons as 0/1)."""
        return np.array(
            [
                self.yaw_delta / MAX_ANGLE_DELTA_PER_TICK,
                self.pitch_delta / MAX_ANGLE_DELTA_PER_TICK,
                self.forwardmove / MAX_MOVE_SPEED,
                self.sidemove / MAX_MOVE_SPEED,
                1.0 if Buttons.JUMP in self.buttons else 0.0,
                1.0 if Buttons.DUCK in self.buttons else 0.0,
            ],
            dtype=np.float32,
        )

    @classmethod
    def from_array(cls, arr: np.ndarray) -> "UserCmd":
        """Inverse of to_array; button channels threshold at 0.5."""
        if arr.shape != (cls.VECTOR_SIZE,):
            raise ValueError(f"expected shape ({cls.VECTOR_SIZE},), got {arr.shape}")
        buttons = Buttons.NONE
        if arr[4] >= 0.5:
            buttons |= Buttons.JUMP
        if arr[5] >= 0.5:
            buttons |= Buttons.DUCK
        return cls(
            yaw_delta=float(arr[0]) * MAX_ANGLE_DELTA_PER_TICK,
            pitch_delta=float(arr[1]) * MAX_ANGLE_DELTA_PER_TICK,
            forwardmove=float(arr[2]) * MAX_MOVE_SPEED,
            sidemove=float(arr[3]) * MAX_MOVE_SPEED,
            buttons=buttons,
        ).clamped()

    def to_dict(self) -> dict:
        return {
            "yaw_delta": self.yaw_delta,
            "pitch_delta": self.pitch_delta,
            "forwardmove": self.forwardmove,
            "sidemove": self.sidemove,
            "upmove": self.upmove,
            "buttons": int(self.buttons),
            "impulse": self.impulse,
        }

    @classmethod
    def from_dict(cls, d: dict) -> "UserCmd":
        # .get() keeps older recordings (without upmove/impulse) loadable.
        return cls(
            yaw_delta=d["yaw_delta"],
            pitch_delta=d["pitch_delta"],
            forwardmove=d["forwardmove"],
            sidemove=d["sidemove"],
            upmove=d.get("upmove", 0.0),
            impulse=d.get("impulse", 0),
            buttons=Buttons(d["buttons"]),
        )
