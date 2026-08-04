// Owns the per-run state: the transport to the harness, the game adapter,
// the tick counter, and the run configuration. The CreateMove hook forwards
// each tick here so the hook file stays a thin shim.
#ifndef SURF_SESSION_H
#define SURF_SESSION_H

#include "debug_overlay.h"
#include "game_adapter.h"
#include "ipc_client.h"
#include "surf_protocol.h"

#include <string>

namespace surf {

struct SessionConfig {
    std::string host = kDefaultHost;
    int port = kDefaultPort;
    int response_timeout_ms = 20;  // < one 64-tick frame; falls back on miss
    int reconnect_every_ticks = 64;  // retry a dropped connection ~1x/sec
    std::string reset_command = "";  // console cmd run on a harness reset
};

class Session {
public:
    bool Initialize(const SessionConfig& cfg);
    void Shutdown();

    // Called from inside the hooked CreateMove, after the original ran, with
    // the command the engine built for this tick. Captures state, exchanges
    // with the harness, and overwrites the command with the model's action.
    void OnCreateMove(CUserCmd* usercmd);

private:
    void EnsureConnected();

    SessionConfig cfg_;
    GameAdapter adapter_;
    IpcClient ipc_;
    DebugOverlay overlay_;
    long long tick_ = 0;
    int ticks_since_connect_attempt_ = 0;
    bool initialized_ = false;
};

// The single live session the hook talks to (set up at plugin load).
Session& GlobalSession();

}  // namespace surf

#endif  // SURF_SESSION_H
