// Shared debug state between the session (tick thread) and the debug UI
// (render thread). Telemetry fields are written by the session each tick and
// read by the menu/overlay; control fields are written by the menu and read
// by the session. Plain fields with benign cross-thread races — this is a
// debug tool, not a synchronization-critical path.
#ifndef SURF_DEBUG_STATE_H
#define SURF_DEBUG_STATE_H

namespace surf {

struct DebugState {
    // --- live telemetry (written by the session) -----------------------
    bool connected = false;   // harness socket up
    bool in_game = false;
    long long tick = 0;
    long long timeouts = 0;   // ticks the harness missed its deadline
    float origin[3] = {0, 0, 0};
    float velocity[3] = {0, 0, 0};
    float speed_2d = 0.0f;
    float view_yaw = 0.0f;
    float view_pitch = 0.0f;
    bool on_ground = false;
    bool run_failed = false;
    bool run_completed = false;
    // Last command actually applied to the outgoing CUserCmd.
    float last_yaw_delta = 0.0f;
    float last_forwardmove = 0.0f;
    float last_sidemove = 0.0f;
    int last_buttons = 0;

    // --- controls (written by the menu) --------------------------------
    bool menu_visible = true;
    bool model_input_enabled = true;  // apply model movement to the CUserCmd
    bool paused = false;              // stop exchanging with the harness
    bool draw_overlay = true;         // draw the in-world debug overlay
    bool overlay_velocity = true;
    bool overlay_surface_normal = true;
    bool overlay_text = true;

    // --- one-shot requests from the menu (cleared by the session) ------
    bool run_command_requested = false;  // run command_buffer once
    bool request_reset = false;          // run the configured reset command
    char command_buffer[128] = {0};
};

// The single debug-state instance shared by the session and the UI.
inline DebugState& GlobalDebugState() {
    static DebugState state;
    return state;
}

}  // namespace surf

#endif  // SURF_DEBUG_STATE_H
