"""TrajectoryDataset: loads recorded client-domain transitions for training.

Feature extraction here calls the same ClientState.to_features() and
UserCmd.to_array() used live, so a behavior-cloned policy trains on
literally the same tensors it will see and emit at deployment.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.data.recorder import read_transitions


@dataclass(frozen=True)
class Transition:
    state: ClientState
    cmd: UserCmd
    next_state: ClientState
    failed: bool
    completed: bool


class TrajectoryDataset:
    def __init__(self, transitions: list[Transition]) -> None:
        self._transitions = transitions

    @classmethod
    def load(cls, path: str | Path) -> "TrajectoryDataset":
        transitions = [
            Transition(
                state=ClientState.from_dict(raw["state"]),
                cmd=UserCmd.from_dict(raw["cmd"]),
                next_state=ClientState.from_dict(raw["next_state"]),
                failed=raw["failed"],
                completed=raw["completed"],
            )
            for raw in read_transitions(path)
        ]
        return cls(transitions)

    def __len__(self) -> int:
        return len(self._transitions)

    def __getitem__(self, idx: int) -> Transition:
        return self._transitions[idx]

    def as_arrays(self) -> tuple[np.ndarray, np.ndarray]:
        """(observations, actions) matrices for behavior cloning."""
        obs = np.stack([t.state.to_features() for t in self._transitions])
        acts = np.stack([t.cmd.to_array() for t in self._transitions])
        return obs, acts

    def iter_batches(self, batch_size: int, rng: np.random.Generator) -> "np.ndarray":
        """Yield shuffled (obs, act) minibatches."""
        obs, acts = self.as_arrays()
        idx = rng.permutation(len(obs))
        for start in range(0, len(idx), batch_size):
            sel = idx[start : start + batch_size]
            yield obs[sel], acts[sel]
