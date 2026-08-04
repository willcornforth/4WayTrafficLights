"""SurfPolicy: maps ClientState features to a UserCmd action vector.

Torch is imported lazily so the client/env/data layers stay importable in
environments without it (the training extra installs torch).
"""

from __future__ import annotations

import numpy as np

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.config import ModelConfig


def build_policy(cfg: ModelConfig):
    """Build the torch MLP policy module. Requires the [train] extra."""
    import torch
    from torch import nn

    class SurfPolicy(nn.Module):
        """MLP over ClientState features -> normalized UserCmd vector.

        Continuous channels (angle deltas, move axes) come out of tanh in
        [-1, 1]; button channels come out of sigmoid in [0, 1]. Both match
        UserCmd.from_array's expectations, so policy output IS the action
        domain — no adapter layer between model and client.

        forward() is the deterministic mean action (what BC regresses and
        what deployment uses). For PPO, the same heads parameterize a
        stochastic action distribution: a Gaussian with state-independent
        learned log-std over the continuous channels, and a Bernoulli over
        the button channels. sample_actions/evaluate_actions expose it.
        """

        def __init__(self) -> None:
            super().__init__()
            act = {"relu": nn.ReLU, "tanh": nn.Tanh}[cfg.activation]
            layers: list[nn.Module] = []
            in_size = ClientState.FEATURE_SIZE
            for hidden in cfg.hidden_sizes:
                layers += [nn.Linear(in_size, hidden), act()]
                in_size = hidden
            self.backbone = nn.Sequential(*layers)
            self.continuous_head = nn.Linear(in_size, 4)
            self.button_head = nn.Linear(in_size, 2)
            # Scalar value head so PPO can share the backbone.
            self.value_head = nn.Linear(in_size, 1)
            # State-independent exploration noise for the continuous channels.
            self.log_std = nn.Parameter(torch.full((4,), cfg.log_std_init))

        def forward(self, features: "torch.Tensor") -> "torch.Tensor":
            h = self.backbone(features)
            cont = torch.tanh(self.continuous_head(h))
            btn = torch.sigmoid(self.button_head(h))
            return torch.cat([cont, btn], dim=-1)

        def value(self, features: "torch.Tensor") -> "torch.Tensor":
            return self.value_head(self.backbone(features)).squeeze(-1)

        def _distributions(self, features: "torch.Tensor"):
            h = self.backbone(features)
            cont_mean = torch.tanh(self.continuous_head(h))
            btn_prob = torch.sigmoid(self.button_head(h)).clamp(1e-6, 1 - 1e-6)
            cont = torch.distributions.Normal(cont_mean, self.log_std.exp())
            btn = torch.distributions.Bernoulli(probs=btn_prob)
            return cont, btn, self.value_head(h).squeeze(-1)

        def sample_actions(self, features: "torch.Tensor"):
            """Sample stochastic actions; returns (actions, log_probs, values).

            Continuous channels are unbounded Gaussian samples (clip before
            handing to the env; keep the raw sample for the PPO ratio),
            button channels are 0/1 Bernoulli draws.
            """
            cont, btn, value = self._distributions(features)
            cont_a = cont.sample()
            btn_a = btn.sample()
            log_prob = cont.log_prob(cont_a).sum(-1) + btn.log_prob(btn_a).sum(-1)
            return torch.cat([cont_a, btn_a], dim=-1), log_prob, value

        def evaluate_actions(self, features: "torch.Tensor", actions: "torch.Tensor"):
            """Log-prob, entropy, and value of given actions under the current policy."""
            cont, btn, value = self._distributions(features)
            log_prob = cont.log_prob(actions[..., :4]).sum(-1) + btn.log_prob(actions[..., 4:]).sum(-1)
            entropy = cont.entropy().sum(-1) + btn.entropy().sum(-1)
            return log_prob, entropy, value

        @torch.no_grad()
        def act(self, state: ClientState) -> UserCmd:
            features = torch.from_numpy(state.to_features()).unsqueeze(0)
            action = self(features).squeeze(0).numpy().astype(np.float32)
            return UserCmd.from_array(action)

    return SurfPolicy()
