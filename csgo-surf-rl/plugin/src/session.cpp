#include "session.h"

#include <cstdio>

namespace surf {

Session& GlobalSession() {
    static Session s;
    return s;
}

bool Session::Initialize(const SessionConfig& cfg) {
    cfg_ = cfg;
    if (!adapter_.Initialize()) {
        // Adapter logs the specific reason (missing iface / unset offsets).
        return false;
    }
    initialized_ = true;
    tick_ = 0;
    ticks_since_connect_attempt_ = cfg_.reconnect_every_ticks;  // try soon
    std::printf("[surf] session initialized; harness at %s:%d\n",
                cfg_.host.c_str(), cfg_.port);
    return true;
}

void Session::Shutdown() {
    ipc_.Disconnect();
    initialized_ = false;
}

void Session::EnsureConnected() {
    if (ipc_.IsConnected()) return;
    if (++ticks_since_connect_attempt_ < cfg_.reconnect_every_ticks) return;
    ticks_since_connect_attempt_ = 0;
    // tick_interval reported to the harness in the hello handshake.
    ipc_.Connect(cfg_.host.c_str(), cfg_.port, 1.0 / 64.0);
}

void Session::OnCreateMove(CUserCmd* usercmd) {
    if (!initialized_ || !usercmd) return;
    if (!adapter_.IsInGame()) return;

    EnsureConnected();
    if (!ipc_.IsConnected()) return;  // harness not up; leave input untouched

    StateFrame state;
    if (!adapter_.CaptureState(tick_, &state)) return;

    CommandFrame cmd = ipc_.Exchange(state, cfg_.response_timeout_ms);
    if (!cmd.valid) return;  // timeout: keep the engine's own command

    if (cmd.is_reset) {
        // A per-reset override command takes precedence over the configured one.
        const std::string& rc =
            !cmd.console_command.empty() ? cmd.console_command : cfg_.reset_command;
        if (!rc.empty()) adapter_.RunConsoleCommand(rc.c_str());
        tick_ = 0;
        return;
    }

    // A console command rides along whether or not movement is set, so the
    // harness can push inputs to the client on any tick.
    if (!cmd.console_command.empty()) {
        adapter_.RunConsoleCommand(cmd.console_command.c_str());
    }
    if (cmd.has_movement) {
        adapter_.ApplyCommand(cmd, usercmd);
        ++tick_;
    }
}

}  // namespace surf
