"""MockSurfBridge: an offline stand-in for a real game client.

Implements a deliberately small caricature of Source air movement — gravity,
air-strafe wish-direction acceleration, and velocity clipping against a
single straight ramp plane running along +x. It exists so the environment,
recorder, dataset, and training loops can run end-to-end (and be tested)
without a game client. It is NOT a physics simulator; a real integration
replaces this class behind the same GameClientBridge interface.
"""

from __future__ import annotations

import numpy as np

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd

GRAVITY = 800.0  # units/s^2
AIR_ACCELERATE = 150.0  # generous, keeps the mock surfable
AIR_SPEED_CAP = 30.0  # Source's per-tick wishspeed cap in air


class MockSurfBridge(GameClientBridge):
    """A straight ramp along +x; surf on its left face, fall off below it.

    The ramp face has a fixed inward-tilted normal. Holding sidemove into the
    ramp while steering yaw down-course gains speed, like a real surf ramp.
    """

    def __init__(
        self,
        course_length: float = 4096.0,
        fail_z: float = -512.0,
        tick_rate: int = 64,
        seed: int = 0,
    ) -> None:
        self._course_length = course_length
        self._fail_z = fail_z
        self._tick_interval = 1.0 / tick_rate
        # Ramp face normal: tilted 45° off vertical, facing +y and up.
        self._ramp_normal = np.array([0.0, np.sin(np.radians(45)), np.cos(np.radians(45))])
        self._rng = np.random.default_rng(seed)
        self.reset()

    # -- GameClientBridge ---------------------------------------------------

    def reset(self) -> ClientState:
        self._tick = 0
        self._origin = np.array([0.0, -64.0, 0.0])
        self._velocity = np.array([250.0, 0.0, 0.0])
        self._yaw = 0.0
        self._pitch = 0.0
        return self._snapshot()

    def step(self, cmd: UserCmd) -> ClientState:
        cmd = cmd.clamped()
        dt = self._tick_interval
        self._tick += 1
        self._yaw += cmd.yaw_delta
        self._pitch = float(np.clip(self._pitch + cmd.pitch_delta, -89.0, 89.0))

        # Wish direction from movement axes rotated into world by view yaw.
        yaw_r = np.radians(self._yaw)
        fwd = np.array([np.cos(yaw_r), np.sin(yaw_r), 0.0])
        right = np.array([np.sin(yaw_r), -np.cos(yaw_r), 0.0])
        wish = fwd * cmd.forwardmove + right * cmd.sidemove
        wish_speed = float(np.linalg.norm(wish))
        if wish_speed > 1e-6:
            wish_dir = wish / wish_speed
            # Source-style air accelerate with the per-tick speed cap.
            current = float(self._velocity @ wish_dir)
            add = min(wish_speed, AIR_SPEED_CAP) - current
            if add > 0:
                accel = min(AIR_ACCELERATE * min(wish_speed, AIR_SPEED_CAP) * dt, add)
                self._velocity += accel * wish_dir

        self._velocity[2] -= GRAVITY * dt
        self._origin += self._velocity * dt

        # Ramp face: plane through the origin line y = z = 0. Penetrating it
        # from the surf side clips velocity into the plane (the "surf").
        depth = float(self._origin @ self._ramp_normal)
        if depth < 0.0:
            self._origin -= depth * self._ramp_normal
            into = float(self._velocity @ self._ramp_normal)
            if into < 0.0:
                self._velocity -= into * self._ramp_normal

        return self._snapshot()

    def is_run_failed(self) -> bool:
        return bool(self._origin[2] < self._fail_z)

    def is_run_complete(self) -> bool:
        return bool(self._origin[0] >= self._course_length)

    @property
    def tick_interval(self) -> float:
        return self._tick_interval

    # -- helpers ------------------------------------------------------------

    @property
    def course_length(self) -> float:
        return self._course_length

    def _snapshot(self) -> ClientState:
        near_ramp = float(self._origin @ self._ramp_normal) < 8.0
        return ClientState(
            tick=self._tick,
            origin=tuple(float(v) for v in self._origin),
            velocity=tuple(float(v) for v in self._velocity),
            view_yaw=self._yaw,
            view_pitch=self._pitch,
            on_ground=False,
            surface_normal=tuple(self._ramp_normal) if near_ramp else (0.0, 0.0, 1.0),
        )
