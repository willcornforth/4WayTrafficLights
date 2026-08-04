# Surf telemetry wire protocol

The plugin (inside the game) and the Python harness exchange one message per
client tick over a loopback TCP connection. Messages are UTF-8 JSON objects,
one per line, newline (`\n`) delimited. The field layouts of `state` and `cmd`
are byte-for-byte the same dicts produced by `ClientState.to_dict()` and
`UserCmd.to_dict()` in the Python package — so the data crossing the socket is
literally the client data domain the model trains on.

## Roles

- The **harness listens** on `127.0.0.1:<port>` (default 42813).
- The **plugin connects** out when the map loads. Start training first, then
  launch the game.

Rationale: the RL loop owns the socket's lifetime; the game may be relaunched
across training runs without the harness losing its listen socket.

## Handshake

On connect the plugin sends exactly one `hello`:

```json
{"type":"hello","tick_interval":0.015625,"protocol":1}
```

`tick_interval` is seconds per tick (1/64 = 0.015625 for a 64-tick server).

## Per-tick exchange

The plugin drives the clock. Inside each `CreateMove` it sends one `state`:

```json
{"type":"state",
 "state":{"tick":1234,"origin":[x,y,z],"velocity":[vx,vy,vz],
          "view_yaw":0.0,"view_pitch":0.0,"on_ground":false,
          "surface_normal":[0.0,0.0,1.0],"extras":{}},
 "failed":false,"completed":false}
```

`failed` / `completed` are set from map trigger events when the integration
wires them up (fell off / reached the end zone); otherwise `false`, and the
harness may derive termination itself.

The plugin then blocks (up to `response_timeout_ms`) for one reply:

```json
{"type":"cmd",
 "cmd":{"yaw_delta":0.0,"pitch_delta":0.0,
        "forwardmove":0.0,"sidemove":450.0,"buttons":0}}
```

- `yaw_delta` / `pitch_delta` are **degrees this tick**. The plugin adds them
  to the current view angles and writes the result into both the outgoing
  `CUserCmd.viewangles` and the engine view, so the action domain matches the
  Python `UserCmd` (mouse-style deltas), not absolute angles.
- `forwardmove` / `sidemove` map directly to `CUserCmd.forwardmove/sidemove`.
- `buttons` is the Python `Buttons` bitfield (JUMP=1, DUCK=2, ATTACK=4,
  USE=8); the plugin translates it to the engine's `IN_*` flags.

If no reply arrives within the timeout the plugin applies a zero command for
that tick (no freeze) and continues.

## Reset

To start a new run the harness sends:

```json
{"type":"reset"}
```

The plugin runs its configured reset console command (e.g. a teleport to the
course start; requires a server the user controls) and then resumes sending
`state` frames. The first `state` after a reset carries the run's opening
tick.
```
