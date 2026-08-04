"""PPO fine-tuning over SurfEnv.

Scaffold: rollout collection and GAE are implemented; the clipped surrogate
update is left as the clearly marked extension point. Start from a BC
checkpoint (training/bc.py) so exploration begins from human-like inputs.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.config import HarnessConfig
from csgo_surf_rl.env.surf_env import SurfEnv
from csgo_surf_rl.models.policy import build_policy


def collect_rollout(env: SurfEnv, policy, ticks: int) -> dict[str, np.ndarray]:
    """Collect one on-policy rollout. Requires torch (via the policy)."""
    import torch

    obs_buf, act_buf, rew_buf, val_buf, done_buf = [], [], [], [], []
    obs, _ = env.reset()
    for _ in range(ticks):
        obs_t = torch.from_numpy(obs).unsqueeze(0)
        with torch.no_grad():
            action = policy(obs_t).squeeze(0).numpy()
            value = float(policy.value(obs_t))
        next_obs, reward, terminated, truncated, _ = env.step(action)
        obs_buf.append(obs)
        act_buf.append(action)
        rew_buf.append(reward)
        val_buf.append(value)
        done_buf.append(terminated or truncated)
        obs = next_obs
        if terminated or truncated:
            obs, _ = env.reset()
    return {
        "obs": np.stack(obs_buf).astype(np.float32),
        "actions": np.stack(act_buf).astype(np.float32),
        "rewards": np.array(rew_buf, dtype=np.float32),
        "values": np.array(val_buf, dtype=np.float32),
        "dones": np.array(done_buf, dtype=bool),
    }


def compute_gae(rollout: dict[str, np.ndarray], gamma: float, lam: float) -> np.ndarray:
    """Generalized advantage estimation over a (possibly multi-episode) rollout."""
    rewards, values, dones = rollout["rewards"], rollout["values"], rollout["dones"]
    advantages = np.zeros_like(rewards)
    gae = 0.0
    for t in reversed(range(len(rewards))):
        next_value = 0.0 if (t == len(rewards) - 1 or dones[t]) else values[t + 1]
        delta = rewards[t] + gamma * next_value - values[t]
        gae = delta + gamma * lam * (0.0 if dones[t] else gae)
        advantages[t] = gae
    return advantages


def train_ppo(cfg: HarnessConfig, bridge: GameClientBridge, bc_checkpoint: str | Path | None = None) -> dict:
    """PPO outer loop. The policy-update step is the extension point."""
    import torch

    torch.manual_seed(cfg.train.seed)
    env = SurfEnv(bridge, cfg.env)
    policy = build_policy(cfg.model)
    if bc_checkpoint is not None:
        policy.load_state_dict(torch.load(bc_checkpoint, weights_only=True))

    total = 0
    iterations = 0
    while total < cfg.train.ppo_total_ticks:
        rollout = collect_rollout(env, policy, cfg.train.ppo_rollout_ticks)
        advantages = compute_gae(rollout, cfg.train.gamma, cfg.train.gae_lambda)
        _ = advantages  # consumed by the update step below

        # TODO(extension point): clipped-surrogate PPO update.
        # A stochastic action head (e.g. Gaussian over the continuous
        # channels, Bernoulli over buttons) with log-probs is needed for the
        # ratio term; the current deterministic head is BC-shaped. Options:
        #   - add log_std parameters to SurfPolicy and implement the update
        #   - or hand the env to a library learner (e.g. SB3 PPO) here.
        raise NotImplementedError(
            "PPO update step not implemented yet — see TODO in train_ppo(). "
            "Rollout collection and GAE above are functional."
        )

    env.close()
    return {"iterations": iterations, "total_ticks": total}
