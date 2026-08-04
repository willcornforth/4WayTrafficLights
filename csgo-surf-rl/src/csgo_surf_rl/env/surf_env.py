"""SurfEnv: a Gymnasium environment over any GameClientBridge.

The environment is a thin adapter: observations are ClientState feature
vectors, actions are normalized UserCmd vectors. All domain knowledge stays
in the client package; all reward shaping stays in rewards.py.
"""

from __future__ import annotations

from typing import Any

import gymnasium as gym
import numpy as np
from gymnasium import spaces

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.config import EnvConfig
from csgo_surf_rl.env.rewards import compute_reward


class SurfEnv(gym.Env):
    metadata = {"render_modes": []}

    def __init__(self, bridge: GameClientBridge, config: EnvConfig | None = None) -> None:
        super().__init__()
        self._bridge = bridge
        self._cfg = config or EnvConfig()
        self._prev_state: ClientState | None = None
        self._episode_ticks = 0

        self.observation_space = spaces.Box(
            low=-np.inf, high=np.inf, shape=(ClientState.FEATURE_SIZE,), dtype=np.float32
        )
        # Normalized UserCmd vector; button channels are thresholded at 0.5.
        self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(UserCmd.VECTOR_SIZE,), dtype=np.float32)

    def reset(self, *, seed: int | None = None, options: dict | None = None) -> tuple[np.ndarray, dict]:
        super().reset(seed=seed)
        self._prev_state = self._bridge.reset()
        self._episode_ticks = 0
        return self._prev_state.to_features(), {"client_state": self._prev_state}

    def step(self, action: np.ndarray) -> tuple[np.ndarray, float, bool, bool, dict[str, Any]]:
        assert self._prev_state is not None, "call reset() before step()"
        cmd = UserCmd.from_array(np.asarray(action, dtype=np.float32))
        state = self._bridge.step(cmd)
        self._episode_ticks += 1

        failed = self._bridge.is_run_failed()
        completed = self._bridge.is_run_complete()
        reward = compute_reward(self._prev_state, state, self._cfg, failed, completed)

        terminated = failed or completed
        truncated = self._episode_ticks >= self._cfg.max_episode_ticks
        info: dict[str, Any] = {"client_state": state, "usercmd": cmd, "failed": failed, "completed": completed}
        self._prev_state = state
        return state.to_features(), reward, terminated, truncated, info

    def close(self) -> None:
        self._bridge.close()
