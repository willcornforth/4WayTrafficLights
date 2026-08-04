"""PPO fine-tuning over SurfEnv.

On-policy rollouts are collected with the policy's stochastic heads
(Gaussian over continuous channels, Bernoulli over buttons), advantages come
from GAE, and the update is the standard clipped surrogate with a value loss
and entropy bonus. Start from a BC checkpoint (training/bc.py) so
exploration begins from human-like inputs.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

from csgo_surf_rl.client.bridge import GameClientBridge
from csgo_surf_rl.config import HarnessConfig
from csgo_surf_rl.env.surf_env import SurfEnv
from csgo_surf_rl.models.policy import build_policy


def collect_rollout(env: SurfEnv, policy, ticks: int) -> dict[str, np.ndarray]:
    """Collect one on-policy rollout of stochastic actions. Requires torch.

    The stored actions are the raw distribution samples (continuous channels
    may fall outside [-1, 1]); the env receives a clipped copy. The ratio in
    the PPO update must be computed against the raw sample, so that is what
    the buffer keeps.
    """
    import torch

    obs_buf, act_buf, logp_buf, rew_buf, val_buf, done_buf = [], [], [], [], [], []
    obs, _ = env.reset()
    for _ in range(ticks):
        obs_t = torch.from_numpy(obs).unsqueeze(0)
        with torch.no_grad():
            action_t, log_prob_t, value_t = policy.sample_actions(obs_t)
        action = action_t.squeeze(0).numpy().astype(np.float32)
        next_obs, reward, terminated, truncated, _ = env.step(np.clip(action, -1.0, 1.0))
        obs_buf.append(obs)
        act_buf.append(action)
        logp_buf.append(float(log_prob_t))
        rew_buf.append(reward)
        val_buf.append(float(value_t))
        done_buf.append(terminated or truncated)
        obs = next_obs
        if terminated or truncated:
            obs, _ = env.reset()
    return {
        "obs": np.stack(obs_buf).astype(np.float32),
        "actions": np.stack(act_buf).astype(np.float32),
        "log_probs": np.array(logp_buf, dtype=np.float32),
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


def ppo_update(policy, optimizer, rollout: dict[str, np.ndarray], advantages: np.ndarray, cfg: HarnessConfig) -> dict:
    """One clipped-surrogate PPO update over a rollout; returns loss metrics."""
    import torch
    from torch import nn

    obs = torch.from_numpy(rollout["obs"])
    actions = torch.from_numpy(rollout["actions"])
    old_log_probs = torch.from_numpy(rollout["log_probs"])
    returns = torch.from_numpy(advantages + rollout["values"])
    adv = torch.from_numpy(advantages)
    adv = (adv - adv.mean()) / (adv.std() + 1e-8)

    tc = cfg.train
    n = len(obs)
    policy_losses, value_losses, entropies = [], [], []
    for _ in range(tc.ppo_epochs):
        # Fresh shuffle each epoch, then contiguous minibatch slices of it.
        perm = torch.randperm(n)
        for start in range(0, n, tc.ppo_minibatch_size):
            mb = perm[start : start + tc.ppo_minibatch_size]
            log_prob, entropy, value = policy.evaluate_actions(obs[mb], actions[mb])
            ratio = torch.exp(log_prob - old_log_probs[mb])
            unclipped = ratio * adv[mb]
            clipped = torch.clamp(ratio, 1.0 - tc.clip_range, 1.0 + tc.clip_range) * adv[mb]
            policy_loss = -torch.min(unclipped, clipped).mean()
            value_loss = nn.functional.mse_loss(value, returns[mb])
            loss = policy_loss + tc.value_coef * value_loss - tc.entropy_coef * entropy.mean()

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(policy.parameters(), tc.max_grad_norm)
            optimizer.step()

            policy_losses.append(float(policy_loss.detach()))
            value_losses.append(float(value_loss.detach()))
            entropies.append(float(entropy.detach().mean()))
    return {
        "policy_loss": float(np.mean(policy_losses)),
        "value_loss": float(np.mean(value_losses)),
        "entropy": float(np.mean(entropies)),
    }


def train_ppo(cfg: HarnessConfig, bridge: GameClientBridge, bc_checkpoint: str | Path | None = None) -> dict:
    """PPO outer loop: rollout -> GAE -> clipped-surrogate update, repeated."""
    import torch

    torch.manual_seed(cfg.train.seed)
    env = SurfEnv(bridge, cfg.env)
    policy = build_policy(cfg.model)
    if bc_checkpoint is not None:
        # strict=False: BC checkpoints predate the log_std parameter.
        policy.load_state_dict(torch.load(bc_checkpoint, weights_only=True), strict=False)
    optimizer = torch.optim.Adam(policy.parameters(), lr=cfg.train.learning_rate)

    total = 0
    iterations = 0
    history: list[dict] = []
    while total < cfg.train.ppo_total_ticks:
        rollout = collect_rollout(env, policy, cfg.train.ppo_rollout_ticks)
        advantages = compute_gae(rollout, cfg.train.gamma, cfg.train.gae_lambda)
        metrics = ppo_update(policy, optimizer, rollout, advantages, cfg)
        total += len(rollout["rewards"])
        iterations += 1
        metrics["mean_reward"] = float(rollout["rewards"].mean())
        history.append(metrics)

    env.close()
    ckpt_dir = Path(cfg.train.checkpoint_dir)
    ckpt_dir.mkdir(parents=True, exist_ok=True)
    ckpt_path = ckpt_dir / "ppo_policy.pt"
    torch.save(policy.state_dict(), ckpt_path)
    return {
        "iterations": iterations,
        "total_ticks": total,
        "final": history[-1] if history else {},
        "checkpoint": str(ckpt_path),
    }
