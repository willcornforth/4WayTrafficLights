# CSGO Surf RL Harness

A reinforcement learning harness for training a surf movement model for CSGO.

## Design principle: the client is the data domain

The single most important constraint in this harness is **domain closure at the
client boundary**. The model's outputs are client inputs (`UserCmd`: view-angle
deltas, movement axes, buttons), therefore everything the model is trained on —
observations, demonstrations, replays — is captured at that *same* boundary:

- **Observations** are `ClientState`: only what the game client itself knows
  (predicted origin/velocity, view angles, on-ground flag, tick). No
  server-side omniscient data (other players' server state, unpredicted
  physics results) ever enters the observation space.
- **Actions** are `UserCmd`: the exact structure the client submits to the
  engine every tick.
- **Demonstration data** is recorded as `(ClientState, UserCmd)` pairs by the
  same `GameClientBridge` used at inference time, so behavior-cloning data,
  RL rollouts, and deployment all share one domain. There is no
  train/inference domain shift by construction.

```
             ┌────────────────────────────────────────────┐
             │                Game client                 │
             └───────▲───────────────────────┬────────────┘
                     │ UserCmd               │ ClientState
             ┌───────┴───────────────────────▼────────────┐
             │            GameClientBridge (ABC)          │   ← the domain boundary
             └───▲──────────────▲──────────────▲──────────┘
                 │              │              │
           SurfEnv (RL)   Recorder (BC data)   Evaluator
```

## Layout

```
src/csgo_surf_rl/
  client/          # The client data domain
    usercmd.py     #   UserCmd — action structure (what the client sends)
    client_state.py#   ClientState — observation structure (what the client sees)
    bridge.py      #   GameClientBridge ABC — the only door in or out
    mock_bridge.py #   Deterministic surf-physics stub for offline dev & tests
  env/
    surf_env.py    # Gymnasium environment over a GameClientBridge
    rewards.py     # Composable reward terms (progress, speed, fail penalty)
  data/
    recorder.py    # Records (ClientState, UserCmd) pairs from a bridge
    dataset.py     # JSONL trajectory store + numpy loader for BC / offline RL
  models/
    policy.py      # Torch policy over ClientState features → UserCmd (lazy torch import)
  training/
    bc.py          # Behavior-cloning pretraining loop (stub)
    ppo.py         # PPO fine-tuning loop over SurfEnv (stub)
  config.py        # Dataclass configs for env, data, model, training
  cli.py           # `surf-rl record | train-bc | train-rl | eval`
tests/             # Smoke tests running entirely against the mock bridge
```

## Quick start

```bash
cd csgo-surf-rl
pip install -e ".[dev]"      # torch is an optional extra: .[train]
pytest                        # everything runs against the mock bridge
python -m csgo_surf_rl.cli record --out data/run.jsonl --ticks 1000
python -m csgo_surf_rl.cli train-bc --data data/run.jsonl
```

## Connecting a real client

Implement `GameClientBridge` for your integration (e.g. a local offline
server with a SourceMod/GOTV telemetry plugin, or a demo-file parser for
offline demonstration data) and pass it wherever the mock bridge is used
today. Nothing above the bridge changes — that is the point of the boundary.

This harness is intended for offline/local training and research use.
