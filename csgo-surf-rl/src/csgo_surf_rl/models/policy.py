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

        def forward(self, features: "torch.Tensor") -> "torch.Tensor":
            h = self.backbone(features)
            cont = torch.tanh(self.continuous_head(h))
            btn = torch.sigmoid(self.button_head(h))
            return torch.cat([cont, btn], dim=-1)

        def value(self, features: "torch.Tensor") -> "torch.Tensor":
            return self.value_head(self.backbone(features)).squeeze(-1)

        @torch.no_grad()
        def act(self, state: ClientState) -> UserCmd:
            features = torch.from_numpy(state.to_features()).unsqueeze(0)
            action = self(features).squeeze(0).numpy().astype(np.float32)
            return UserCmd.from_array(action)

    return SurfPolicy()
