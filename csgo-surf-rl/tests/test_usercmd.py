import numpy as np

from csgo_surf_rl.client.usercmd import MAX_ANGLE_DELTA_PER_TICK, MAX_MOVE_SPEED, Buttons, UserCmd


def test_array_round_trip():
    cmd = UserCmd(yaw_delta=3.5, pitch_delta=-1.0, forwardmove=450.0, sidemove=-200.0, buttons=Buttons.JUMP)
    restored = UserCmd.from_array(cmd.to_array())
    assert np.isclose(restored.yaw_delta, cmd.yaw_delta)
    assert np.isclose(restored.sidemove, cmd.sidemove)
    assert restored.buttons == Buttons.JUMP


def test_dict_round_trip():
    cmd = UserCmd(yaw_delta=-2.0, sidemove=100.0, buttons=Buttons.JUMP | Buttons.DUCK)
    assert UserCmd.from_dict(cmd.to_dict()) == cmd


def test_clamping():
    cmd = UserCmd(yaw_delta=999.0, forwardmove=9999.0).clamped()
    assert cmd.yaw_delta == MAX_ANGLE_DELTA_PER_TICK
    assert cmd.forwardmove == MAX_MOVE_SPEED
