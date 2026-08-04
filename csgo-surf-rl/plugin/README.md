# Surf telemetry plugin

The in-game half of the `GameClientBridge` boundary: a Source-engine plugin
that runs **inline in the game process**, reads the client's own predicted
state each tick, ships it to the Python harness, and writes the model's
returned `UserCmd` back into the engine's outgoing command for that same
tick. Because the read (`ClientState`) and the write (`UserCmd`) both happen
at the client's `CreateMove`, the data the model trains on and the data it
acts through are the same domain — the whole point of the harness.

```
  ClientModeShared::CreateMove(dt, CUserCmd*)      ← hooked, inline in-engine
        │  original() builds the tick's command
        ▼
  GameAdapter.CaptureState() ──► StateFrame ──┐
                                              │  loopback TCP (JSON lines)
                          CommandFrame ◄───── ┤  ↕  Python SocketClientBridge
        │                                     │
  GameAdapter.ApplyCommand(cmd, CUserCmd*) ◄──┘  overwrites viewangles/moves/buttons
```

## Scope and intended use

This is research tooling for **surf** — a solo/cooperative movement
discipline — for **local training on a server you control** (a listen server
or your own dedicated server). It is not built to evade anti-cheat, is not
obfuscated, and is not intended for use on servers you do not own or in
competitive play. Use it where injecting your own model's movement is
something you're authorized to do.

## Files

| File | Role |
|------|------|
| `src/plugin.cpp` | `IServerPluginCallbacks` lifecycle + `CreateInterface` export |
| `src/createmove_hook.*` | VMT hook of `ClientModeShared::CreateMove` |
| `src/session.*` | Per-run state: transport + adapter + tick counter |
| `src/game_adapter.*` | Reads local-player predicted state, applies commands |
| `src/ipc_client.*` | Loopback TCP client, newline-framed JSON |
| `src/vmt_hook.h` | Generic vtable hook helper |
| `src/sdk_types.h` | SDK type surface (or include the real HL2SDK) |
| `src/game_offsets.h` | **Build-specific** interface versions, VMT index, netvars |
| `src/debug_state.h` | Shared telemetry + control state for the debug UI |
| `src/debug_overlay.*` | In-world debug drawing via engine `IVDebugOverlay` |
| `src/debug_menu.*` | ImGui debug menu contents (backend-agnostic) |
| `src/imgui_backend.*` | ImGui context + render-device/present hook (integration point) |
| `src/imgui_support.h` | Real ImGui include, or a no-op shim when it is absent |
| `include/surf_protocol.h` | Wire constants + button translation (shared) |
| `PROTOCOL.md` | The wire protocol, matching the Python dicts |

## Building

```bash
cd plugin
cmake -S . -B build            # add -DHL2SDK_ROOT=/path/to/hl2sdk for the real SDK
                               # add -DIMGUI_ROOT=/path/to/imgui for the real menu UI
cmake --build build
```

Without `HL2SDK_ROOT` it compiles against the self-contained shims in
`sdk_types.h` / `server_plugin.h` — enough to type-check the transport and
hook logic. For a loadable plugin, build with a real HL2SDK checkout. Without
`IMGUI_ROOT` the ImGui menu compiles against a no-op shim (the in-world
overlay still works); add Dear ImGui to get the real menu.

## Integration points to fill in for a target build

These are isolated on purpose; the logic never hard-codes them:

1. **`game_offsets.h`** — interface version strings, the `CreateMove` VMT
   index, and the `C_BasePlayer` netvar offsets (`m_vecOrigin`,
   `m_vecVelocity`, `m_fFlags`). The adapter refuses to read state until the
   offsets are set, rather than reading garbage.
2. **`ResolveClientMode()` in `plugin.cpp`** — return the `ClientModeShared`
   instance to hook (typically a sig-scan for `g_pClientMode`). Until it
   returns non-null the plugin loads but does not install the hook.
3. **Reset command** — set `SURF_RESET_COMMAND` to whatever teleports the
   player to the course start on your map/server (requires the appropriate
   server privileges).
4. **`ResolveRenderDevice()` in `imgui_backend.cpp`** — *only needed for the
   ImGui menu*. Return the graphics device and install a present/paint hook
   that calls `GlobalImGuiBackend().RenderFrame()`. Until then the menu stays
   disabled; the in-world overlay is independent of this and needs nothing.

## Debug menu & overlay

The plugin ships an in-client debug UI with two independent halves:

- **In-world overlay** (`debug_overlay.*`) — draws the player's velocity
  vector, surface normal, and an origin readout (tick / speed / ground-air)
  using the engine's `IVDebugOverlay`. It needs no graphics hook, so it works
  as soon as the interface resolves — this is the "if required" debug drawing.
- **ImGui menu** (`debug_menu.*` + `imgui_backend.*`) — a window showing live
  telemetry (connection, tick, timeouts, origin/velocity/speed/view, run
  state, last applied command) with toggles that feed straight back into the
  tick loop:
  - **Apply model input** — when off, the model's command is still received
    and shown but not applied, so a human can drive while you watch what the
    model *would* have sent.
  - **Pause exchange** — stop talking to the harness (input untouched).
  - **Draw world overlay** + per-element toggles.
  - **Manual console input** — a text box + *Run command* button, and a
    *Reset run* button, so you can push inputs to the client straight from
    the menu (routed through the same `ClientCmd_Unrestricted` path).

Both halves read and write one shared `DebugState` (`debug_state.h`); the
session publishes telemetry into it each tick and honors its control flags.

## Installing

Copy the built module and `surf_telemetry.vdf` into `<game>/addons/`, then
either autoload it or `plugin_load addons/surf_telemetry` from the console.

## Configuration (environment variables)

| Variable | Default | Meaning |
|----------|---------|---------|
| `SURF_HARNESS_HOST` | `127.0.0.1` | harness listen address |
| `SURF_HARNESS_PORT` | `42813` | harness listen port |
| `SURF_RESPONSE_TIMEOUT_MS` | `20` | per-tick wait for a command before falling back |
| `SURF_RESET_COMMAND` | *(empty)* | console command run on a harness `reset` |

## Driving it from Python

Start the harness first (it listens), then launch the game:

```python
from csgo_surf_rl.client.socket_bridge import SocketClientBridge
from csgo_surf_rl.env.surf_env import SurfEnv

with SocketClientBridge(port=42813) as bridge:  # blocks until the plugin connects
    env = SurfEnv(bridge)
    obs, info = env.reset()
    ...
```

`SocketClientBridge` is a drop-in `GameClientBridge`, so the env, recorder,
and training loops are unchanged from the mock-bridge setup.

## Sending inputs to the client

Beyond the per-tick `UserCmd` the model emits, the harness can push inputs to
the client directly:

```python
# A movement input plus a console input on the same tick:
bridge.step(UserCmd(sidemove=450.0, upmove=200.0, impulse=101),
            command="say training")

# A console input only (setup/control between runs — no model movement):
bridge.send_command("sv_cheats 1; noclip")

# Reset with a one-off teleport override:
bridge.reset(command="ent_fire !self AddOutput \"origin 0 0 64\"")
```

`UserCmd` now carries the full client input structure — `upmove` (jump/crouch
analog axis) and `impulse` (one-shot inputs like `impulse 101`) in addition
to the move/view/button axes. They are applied to the outgoing `CUserCmd` but
are deliberately not part of the learned action vector (see `usercmd.py`).
