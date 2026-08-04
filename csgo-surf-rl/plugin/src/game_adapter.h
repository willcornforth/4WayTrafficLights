// Bridges the engine's client-side objects to the transport-neutral
// StateFrame / CommandFrame structs. This is the one place that reads the
// local player's predicted state and writes the outgoing command, so the
// "client data domain" boundary is enforced in a single file.
#ifndef SURF_GAME_ADAPTER_H
#define SURF_GAME_ADAPTER_H

#include "ipc_client.h"
#include "sdk_types.h"

namespace surf {

class GameAdapter {
public:
    // Resolve engine interfaces via the SDK's CreateInterface. Returns false
    // if a required interface or a required netvar offset is missing.
    bool Initialize();

    bool IsReady() const { return ready_; }
    bool IsInGame() const;

    // Capture the local player's client-predicted state for this tick.
    // Returns false if there is no valid local player right now.
    bool CaptureState(long long tick, StateFrame* out) const;

    // Apply the harness's command to the outgoing CUserCmd, converting view
    // deltas into absolute angles and syncing the engine view.
    void ApplyCommand(const CommandFrame& cmd, CUserCmd* usercmd);

    // Run a console command (used for run resets, e.g. a teleport trigger).
    void RunConsoleCommand(const char* cmd);

private:
    IClientEntity* LocalPlayer() const;

    IVEngineClient* engine_ = nullptr;
    IClientEntityList* entities_ = nullptr;
    bool ready_ = false;
};

}  // namespace surf

#endif  // SURF_GAME_ADAPTER_H
