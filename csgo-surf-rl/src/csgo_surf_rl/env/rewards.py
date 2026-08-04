"""Composable reward terms over client-domain state transitions.

Every term is a pure function of (previous ClientState, current ClientState),
so rewards are computable identically from live rollouts and from recorded
trajectories (useful for offline RL / reward relabeling).
"""

from __future__ import annotations

import numpy as np

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.config import EnvConfig


def progress_reward(prev: ClientState, curr: ClientState) -> float:
    """Down-course displacement this tick. The mock course runs along +x;
    for real maps, replace with displacement along the course spline."""
    return curr.origin[0] - prev.origin[0]


def speed_reward(prev: ClientState, curr: ClientState) -> float:
    """Horizontal speed gained this tick."""

    def speed_2d(s: ClientState) -> float:
        return float(np.hypot(s.velocity[0], s.velocity[1]))

    return speed_2d(curr) - speed_2d(prev)


def compute_reward(
    prev: ClientState,
    curr: ClientState,
    cfg: EnvConfig,
    failed: bool,
    completed: bool,
) -> float:
    reward = cfg.progress_weight * progress_reward(prev, curr)
    reward += cfg.speed_weight * speed_reward(prev, curr)
    if failed:
        reward -= cfg.fail_penalty
    if completed:
        reward += cfg.complete_bonus
    return float(reward)
