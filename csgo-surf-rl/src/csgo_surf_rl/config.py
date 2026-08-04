"""Dataclass configs for the harness. Kept flat and serializable."""

from __future__ import annotations

from dataclasses import asdict, dataclass, field


@dataclass
class EnvConfig:
    max_episode_ticks: int = 64 * 120  # 2 minutes at 64 tick
    fail_penalty: float = 100.0
    complete_bonus: float = 500.0
    progress_weight: float = 1.0
    speed_weight: float = 0.01


@dataclass
class DataConfig:
    trajectory_path: str = "data/demos.jsonl"
    frame_stack: int = 1  # ticks of history per observation


@dataclass
class ModelConfig:
    hidden_sizes: tuple[int, ...] = (256, 256)
    activation: str = "relu"
    log_std_init: float = -1.0  # initial exploration noise (continuous channels)


@dataclass
class TrainConfig:
    batch_size: int = 256
    learning_rate: float = 3e-4
    bc_epochs: int = 10
    ppo_total_ticks: int = 1_000_000
    ppo_rollout_ticks: int = 4096
    ppo_epochs: int = 10  # optimization passes over each rollout
    ppo_minibatch_size: int = 256
    gamma: float = 0.995
    gae_lambda: float = 0.95
    clip_range: float = 0.2
    value_coef: float = 0.5
    entropy_coef: float = 0.01
    max_grad_norm: float = 0.5
    seed: int = 0
    checkpoint_dir: str = "checkpoints"


@dataclass
class HarnessConfig:
    env: EnvConfig = field(default_factory=EnvConfig)
    data: DataConfig = field(default_factory=DataConfig)
    model: ModelConfig = field(default_factory=ModelConfig)
    train: TrainConfig = field(default_factory=TrainConfig)

    def to_dict(self) -> dict:
        return asdict(self)
