"""Command-line entry points: surf-rl record | train-bc | train-rl | eval.

All commands default to the MockSurfBridge so the full pipeline runs
offline; swap in a real GameClientBridge implementation to go live.
"""

from __future__ import annotations

import argparse
import json

import numpy as np

from csgo_surf_rl.client.client_state import ClientState
from csgo_surf_rl.client.mock_bridge import MockSurfBridge
from csgo_surf_rl.client.usercmd import MAX_MOVE_SPEED, UserCmd
from csgo_surf_rl.config import HarnessConfig
from csgo_surf_rl.data.recorder import Recorder
from csgo_surf_rl.env.surf_env import SurfEnv


def scripted_surf_controller(state: ClientState) -> UserCmd:
    """Baseline demonstrator for the mock ramp: strafe into the face while
    steering yaw toward down-course. Stands in for human demonstrations."""
    yaw_error = 0.0 - state.view_yaw
    return UserCmd(
        yaw_delta=float(np.clip(yaw_error * 0.5, -5.0, 5.0)),
        sidemove=MAX_MOVE_SPEED,  # press into the ramp face (-y)
    ).clamped()


def cmd_record(args: argparse.Namespace) -> None:
    bridge = MockSurfBridge(seed=args.seed)
    recorder = Recorder(bridge, args.out)
    total = 0
    for ep in range(args.episodes):
        total += recorder.record_episode(scripted_surf_controller, max_ticks=args.ticks)
    print(f"wrote {total} transitions across {args.episodes} episode(s) to {args.out}")


def cmd_train_bc(args: argparse.Namespace) -> None:
    from csgo_surf_rl.training.bc import train_bc

    metrics = train_bc(HarnessConfig(), args.data)
    print(json.dumps(metrics, indent=2))


def cmd_train_rl(args: argparse.Namespace) -> None:
    from csgo_surf_rl.training.ppo import train_ppo

    metrics = train_ppo(HarnessConfig(), MockSurfBridge(seed=args.seed), bc_checkpoint=args.checkpoint)
    print(json.dumps(metrics, indent=2))


def cmd_eval(args: argparse.Namespace) -> None:
    """Roll the scripted baseline (or a checkpoint, once trained) through the env."""
    cfg = HarnessConfig()
    bridge = MockSurfBridge(seed=args.seed)
    env = SurfEnv(bridge, cfg.env)
    _, info = env.reset()
    total_reward, ticks = 0.0, 0
    terminated = truncated = False
    while not (terminated or truncated):
        cmd = scripted_surf_controller(info["client_state"])
        _, reward, terminated, truncated, info = env.step(cmd.to_array())
        total_reward += reward
        ticks += 1
    outcome = "completed" if info["completed"] else ("failed" if info["failed"] else "timeout")
    print(json.dumps({"outcome": outcome, "ticks": ticks, "return": round(total_reward, 2)}))


def main() -> None:
    parser = argparse.ArgumentParser(prog="surf-rl", description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("record", help="record demonstration transitions from a controller")
    p.add_argument("--out", default="data/demos.jsonl")
    p.add_argument("--ticks", type=int, default=2048, help="max ticks per episode")
    p.add_argument("--episodes", type=int, default=1)
    p.add_argument("--seed", type=int, default=0)
    p.set_defaults(func=cmd_record)

    p = sub.add_parser("train-bc", help="behavior-clone a policy from recorded data")
    p.add_argument("--data", default="data/demos.jsonl")
    p.set_defaults(func=cmd_train_bc)

    p = sub.add_parser("train-rl", help="PPO fine-tuning over the surf env")
    p.add_argument("--checkpoint", default=None, help="BC checkpoint to start from")
    p.add_argument("--seed", type=int, default=0)
    p.set_defaults(func=cmd_train_rl)

    p = sub.add_parser("eval", help="evaluate the baseline controller in the env")
    p.add_argument("--seed", type=int, default=0)
    p.set_defaults(func=cmd_eval)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
