"""The domain-closure property under test: recorded data round-trips into
exactly the tensors the policy trains on and the env produces."""

import numpy as np

from csgo_surf_rl.cli import scripted_surf_controller
from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.mock_bridge import MockSurfBridge
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.data.dataset import TrajectoryDataset
from csgo_surf_rl.data.recorder import Recorder


def test_record_load_round_trip(tmp_path):
    out = tmp_path / "demo.jsonl"
    recorder = Recorder(MockSurfBridge(), out)
    written = recorder.record_episode(scripted_surf_controller, max_ticks=100)
    assert written > 0

    dataset = TrajectoryDataset.load(out)
    assert len(dataset) == written
    first = dataset[0]
    assert isinstance(first.state, ClientState)
    assert isinstance(first.cmd, UserCmd)


def test_dataset_arrays_match_live_domains(tmp_path):
    out = tmp_path / "demo.jsonl"
    Recorder(MockSurfBridge(), out).record_episode(scripted_surf_controller, max_ticks=50)
    dataset = TrajectoryDataset.load(out)

    obs, acts = dataset.as_arrays()
    # Observations have the same shape/dtype the env emits...
    assert obs.shape == (len(dataset), ClientState.FEATURE_SIZE)
    assert obs.dtype == np.float32
    # ...and actions have the same shape/dtype the env accepts.
    assert acts.shape == (len(dataset), UserCmd.VECTOR_SIZE)
    assert acts.dtype == np.float32


def test_batches_cover_dataset(tmp_path):
    out = tmp_path / "demo.jsonl"
    Recorder(MockSurfBridge(), out).record_episode(scripted_surf_controller, max_ticks=64)
    dataset = TrajectoryDataset.load(out)
    rng = np.random.default_rng(0)
    seen = sum(len(obs) for obs, _ in dataset.iter_batches(batch_size=10, rng=rng))
    assert seen == len(dataset)
