"""ClientState: the observation domain.

Only quantities the game client itself can know appear here — predicted
origin/velocity, its own view angles, and locally derivable flags. Nothing
server-omniscient is permitted: keeping observations inside the client
domain guarantees the deployed model sees the same distribution it was
trained on.
"""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

# Normalization scales for feature extraction. Rough magnitudes for surf
# maps; exact values matter less than being fixed between train and deploy.
POSITION_SCALE = 16384.0  # Source world extent
VELOCITY_SCALE = 3500.0  # fast surf speeds


@dataclass(frozen=True)
class ClientState:
    """Client-visible state for one tick."""

    tick: int
    origin: tuple[float, float, float]  # predicted player origin
    velocity: tuple[float, float, float]  # predicted player velocity
    view_yaw: float  # degrees
    view_pitch: float  # degrees
    on_ground: bool
    # Surface normal under/beside the player from a client-side trace,
    # (0, 0, 1) when nothing relevant is hit. Client traces against the
    # static world are legitimate client-domain data.
    surface_normal: tuple[float, float, float] = (0.0, 0.0, 1.0)
    # Free-form extras (e.g. stage index from a client-parsed HUD). Excluded
    # from features unless a custom extractor uses them.
    extras: dict = field(default_factory=dict)

    FEATURE_SIZE = 13

    def to_features(self) -> np.ndarray:
        """Fixed-size normalized feature vector for the policy.

        Yaw enters only via sin/cos of the *velocity-relative* heading, so
        features are invariant to absolute world yaw — matching the
        delta-based action space.
        """
        vx, vy, vz = self.velocity
        speed_2d = float(np.hypot(vx, vy))
        vel_heading = float(np.degrees(np.arctan2(vy, vx))) if speed_2d > 1e-3 else 0.0
        rel_yaw = np.radians(self.view_yaw - vel_heading)
        return np.array(
            [
                self.origin[0] / POSITION_SCALE,
                self.origin[1] / POSITION_SCALE,
                self.origin[2] / POSITION_SCALE,
                vx / VELOCITY_SCALE,
                vy / VELOCITY_SCALE,
                vz / VELOCITY_SCALE,
                speed_2d / VELOCITY_SCALE,
                np.sin(rel_yaw),
                np.cos(rel_yaw),
                self.view_pitch / 89.0,
                1.0 if self.on_ground else 0.0,
                self.surface_normal[0],
                self.surface_normal[1],
            ],
            dtype=np.float32,
        )

    def to_dict(self) -> dict:
        return {
            "tick": self.tick,
            "origin": list(self.origin),
            "velocity": list(self.velocity),
            "view_yaw": self.view_yaw,
            "view_pitch": self.view_pitch,
            "on_ground": self.on_ground,
            "surface_normal": list(self.surface_normal),
            "extras": self.extras,
        }

    @classmethod
    def from_dict(cls, d: dict) -> "ClientState":
        return cls(
            tick=d["tick"],
            origin=tuple(d["origin"]),
            velocity=tuple(d["velocity"]),
            view_yaw=d["view_yaw"],
            view_pitch=d["view_pitch"],
            on_ground=d["on_ground"],
            surface_normal=tuple(d.get("surface_normal", (0.0, 0.0, 1.0))),
            extras=d.get("extras", {}),
        )
