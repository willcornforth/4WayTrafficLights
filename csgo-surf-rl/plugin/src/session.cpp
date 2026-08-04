#include "session.h"

#include "debug_state.h"

#include <cmath>
#include <cstdio>

namespace surf {

Session& GlobalSession() {
    static Session s;
    return s;
}

namespace {

// Mirror the tick's captured state into the shared debug view.
void PublishTelemetry(DebugState& d, const StateFrame& s, long long tick,
                      bool connected) {
    d.connected = connected;
    d.tick = tick;
    for (int i = 0; i < 3; ++i) {
        d.origin[i] = s.origin[i];
        d.velocity[i] = s.velocity[i];
    }
    d.speed_2d = std::sqrt(s.velocity[0] * s.velocity[0] +
                           s.velocity[1] * s.velocity[1]);
    d.view_yaw = s.view_yaw;
    d.view_pitch = s.view_pitch;
    d.on_ground = s.on_ground;
    d.run_failed = s.failed;
    d.run_completed = s.completed;
}

}  // namespace

bool Session::Initialize(const SessionConfig& cfg) {
    cfg_ = cfg;
    if (!adapter_.Initialize()) {
        // Adapter logs the specific reason (missing iface / unset offsets).
        return false;
    }
    overlay_.Initialize();  // best-effort; overlay is optional
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
    DebugState& dbg = GlobalDebugState();
    dbg.in_game = adapter_.IsInGame();
    if (!dbg.in_game) return;

    EnsureConnected();
    const bool connected = ipc_.IsConnected();

    StateFrame state;
    if (!adapter_.CaptureState(tick_, &state)) return;
    PublishTelemetry(dbg, state, tick_, connected);

    // Menu-requested one-shot inputs (work regardless of the harness).
    if (dbg.run_command_requested) {
        adapter_.RunConsoleCommand(dbg.command_buffer);
        dbg.run_command_requested = false;
    }
    if (dbg.request_reset) {
        if (!cfg_.reset_command.empty()) {
            adapter_.RunConsoleCommand(cfg_.reset_command.c_str());
        }
        dbg.request_reset = false;
        tick_ = 0;
    }

    // The in-world overlay needs no harness connection.
    if (dbg.draw_overlay) overlay_.Draw(state, dbg);

    // Pausing or a down harness leaves the player's own input untouched.
    if (dbg.paused || !connected) return;

    CommandFrame cmd = ipc_.Exchange(state, cfg_.response_timeout_ms);
    if (!cmd.valid) {  // timeout: keep the engine's own command
        ++dbg.timeouts;
        return;
    }

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
        // The "Apply model input" toggle lets a human drive while still
        // observing what the model would have sent.
        if (dbg.model_input_enabled) {
            adapter_.ApplyCommand(cmd, usercmd);
            dbg.last_yaw_delta = cmd.yaw_delta;
            dbg.last_forwardmove = cmd.forwardmove;
            dbg.last_sidemove = cmd.sidemove;
            dbg.last_buttons = cmd.buttons;
        }
        ++tick_;
    }
}

}  // namespace surf
