"""Training loops end-to-end on the mock bridge (skipped without torch)."""

from __future__ import annotations

import numpy as np
import pytest

torch = pytest.importorskip("torch")

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.mock_bridge import MockSurfBridge
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.config import HarnessConfig
from csgo_surf_rl.data.recorder import Recorder
from csgo_surf_rl.env.surf_env import SurfEnv
from csgo_surf_rl.models.policy import build_policy
from csgo_surf_rl.training.bc import train_bc
from csgo_surf_rl.training.ppo import collect_rollout, compute_gae, train_ppo


def tiny_config(tmp_path) -> HarnessConfig:
    cfg = HarnessConfig()
    cfg.model.hidden_sizes = (32,)
    cfg.train.bc_epochs = 2
    cfg.train.batch_size = 32
    cfg.train.ppo_total_ticks = 128
    cfg.train.ppo_rollout_ticks = 64
    cfg.train.ppo_epochs = 2
    cfg.train.ppo_minibatch_size = 32
    cfg.train.checkpoint_dir = str(tmp_path / "ckpt")
    return cfg


def test_bc_then_ppo_end_to_end(tmp_path):
    """BC on recorded data, then PPO from the BC checkpoint, all on the mock."""
    data_path = tmp_path / "demos.jsonl"
    Recorder(MockSurfBridge(seed=0), data_path).record_episode(
        lambda state: UserCmd(sidemove=450.0), max_ticks=64
    )
    cfg = tiny_config(tmp_path)

    bc_metrics = train_bc(cfg, data_path)
    assert bc_metrics["epochs"] == 2
    assert np.isfinite(bc_metrics["final_loss"])

    ppo_metrics = train_ppo(cfg, MockSurfBridge(seed=0), bc_checkpoint=bc_metrics["checkpoint"])
    assert ppo_metrics["iterations"] == 2
    assert ppo_metrics["total_ticks"] == 128
    for key in ("policy_loss", "value_loss", "entropy", "mean_reward"):
        assert np.isfinite(ppo_metrics["final"][key])
    # The PPO checkpoint must load back into a fresh policy (incl. log_std).
    policy = build_policy(cfg.model)
    policy.load_state_dict(torch.load(ppo_metrics["checkpoint"], weights_only=True))


def test_ppo_update_moves_policy(tmp_path):
    """One PPO iteration must actually change the policy parameters."""
    cfg = tiny_config(tmp_path)
    cfg.train.ppo_total_ticks = 64

    metrics = train_ppo(cfg, MockSurfBridge(seed=0))
    reloaded = build_policy(cfg.model)
    reloaded.load_state_dict(torch.load(metrics["checkpoint"], weights_only=True))
    # train_ppo seeds torch with cfg.train.seed before building its policy,
    # so this reconstructs the exact pre-update initialization.
    torch.manual_seed(cfg.train.seed)
    fresh_init = build_policy(cfg.model)
    with torch.no_grad():
        diffs = [
            float((p1 - p2).abs().max())
            for p1, p2 in zip(reloaded.parameters(), fresh_init.parameters())
        ]
    assert max(diffs) > 0.0


def test_sampled_log_probs_match_evaluate():
    """evaluate_actions must reproduce the log-probs from sample_actions."""
    cfg = HarnessConfig()
    cfg.model.hidden_sizes = (16,)
    policy = build_policy(cfg.model)
    features = torch.randn(8, ClientState.FEATURE_SIZE)
    actions, log_probs, values = policy.sample_actions(features)
    log_probs2, entropy, values2 = policy.evaluate_actions(features, actions)
    assert torch.allclose(log_probs, log_probs2, atol=1e-5)
    assert torch.allclose(values, values2, atol=1e-5)
    assert torch.all(entropy > 0)
    # Button channels are exact Bernoulli draws.
    assert set(actions[:, 4:].unique().tolist()) <= {0.0, 1.0}


def test_rollout_and_gae_shapes():
    cfg = HarnessConfig()
    cfg.model.hidden_sizes = (16,)
    policy = build_policy(cfg.model)
    env = SurfEnv(MockSurfBridge(seed=0), cfg.env)
    rollout = collect_rollout(env, policy, ticks=32)
    assert rollout["obs"].shape == (32, ClientState.FEATURE_SIZE)
    assert rollout["actions"].shape == (32, UserCmd.VECTOR_SIZE)
    assert rollout["log_probs"].shape == (32,)
    adv = compute_gae(rollout, gamma=0.99, lam=0.95)
    assert adv.shape == (32,)
    assert np.all(np.isfinite(adv))
    env.close()
