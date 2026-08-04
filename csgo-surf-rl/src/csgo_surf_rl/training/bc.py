"""Behavior-cloning pretraining from recorded client-domain trajectories.

Supervised regression of UserCmd vectors from ClientState features. Because
the dataset stores the same structures the policy consumes and produces,
this is a plain (features -> action) regression with no domain adaptation.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

from csgo_surf_rl.config import HarnessConfig
from csgo_surf_rl.data.dataset import TrajectoryDataset
from csgo_surf_rl.models.policy import build_policy


def train_bc(cfg: HarnessConfig, data_path: str | Path) -> dict:
    """Run behavior cloning; returns summary metrics. Requires torch."""
    import torch
    from torch import nn

    torch.manual_seed(cfg.train.seed)
    rng = np.random.default_rng(cfg.train.seed)

    dataset = TrajectoryDataset.load(data_path)
    if len(dataset) == 0:
        raise ValueError(f"no transitions found in {data_path}")

    policy = build_policy(cfg.model)
    optimizer = torch.optim.Adam(policy.parameters(), lr=cfg.train.learning_rate)
    # MSE on continuous channels, BCE on button channels.
    mse = nn.MSELoss()
    bce = nn.BCELoss()

    losses: list[float] = []
    for epoch in range(cfg.train.bc_epochs):
        epoch_losses = []
        for obs, acts in dataset.iter_batches(cfg.train.batch_size, rng):
            obs_t = torch.from_numpy(obs)
            acts_t = torch.from_numpy(acts)
            pred = policy(obs_t)
            loss = mse(pred[:, :4], acts_t[:, :4]) + bce(pred[:, 4:], acts_t[:, 4:])
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            epoch_losses.append(float(loss))
        losses.append(float(np.mean(epoch_losses)))

    ckpt_dir = Path(cfg.train.checkpoint_dir)
    ckpt_dir.mkdir(parents=True, exist_ok=True)
    ckpt_path = ckpt_dir / "bc_policy.pt"
    torch.save(policy.state_dict(), ckpt_path)
    return {"epochs": cfg.train.bc_epochs, "final_loss": losses[-1], "checkpoint": str(ckpt_path)}
