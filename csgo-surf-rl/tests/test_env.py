import numpy as np

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.mock_bridge import MockSurfBridge
from csgo_surf_rl.client.usercmd import UserCmd
from csgo_surf_rl.env.surf_env import SurfEnv


def test_reset_returns_feature_vector():
    env = SurfEnv(MockSurfBridge())
    obs, info = env.reset()
    assert obs.shape == (ClientState.FEATURE_SIZE,)
    assert obs.dtype == np.float32
    assert isinstance(info["client_state"], ClientState)


def test_step_accepts_action_space_samples():
    env = SurfEnv(MockSurfBridge())
    env.reset()
    obs, reward, terminated, truncated, info = env.step(env.action_space.sample())
    assert obs.shape == (ClientState.FEATURE_SIZE,)
    assert isinstance(reward, float)
    assert isinstance(info["usercmd"], UserCmd)


def test_falling_off_terminates_with_penalty():
    env = SurfEnv(MockSurfBridge())
    env.reset()
    # No input: gravity pulls the player below fail_z eventually.
    terminated = False
    reward = 0.0
    for _ in range(2000):
        _, reward, terminated, truncated, info = env.step(np.zeros(UserCmd.VECTOR_SIZE, dtype=np.float32))
        if terminated or truncated:
            break
    assert terminated
    assert info["failed"]
    # The fail penalty dominates any single tick's progress reward.
    assert reward < -50


def test_mock_bridge_is_deterministic():
    def rollout():
        bridge = MockSurfBridge(seed=7)
        states = []
        for _ in range(50):
            states.append(bridge.step(UserCmd(sidemove=-450.0, yaw_delta=0.5)))
        return states

    a, b = rollout(), rollout()
    assert [s.origin for s in a] == [s.origin for s in b]
