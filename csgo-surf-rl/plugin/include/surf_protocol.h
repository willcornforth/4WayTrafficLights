// Shared wire-protocol constants for the surf telemetry plugin.
// Keep these in lockstep with plugin/PROTOCOL.md and the Python
// csgo_surf_rl.client.usercmd.Buttons flags.
#ifndef SURF_PROTOCOL_H
#define SURF_PROTOCOL_H

namespace surf {

// Protocol revision; bump on any incompatible wire change.
constexpr int kProtocolVersion = 1;

// Default loopback endpoint the harness listens on.
constexpr const char* kDefaultHost = "127.0.0.1";
constexpr int kDefaultPort = 42813;

// Python Buttons bitfield (csgo_surf_rl.client.usercmd.Buttons).
enum SurfButton {
    SURF_BTN_JUMP = 1 << 0,
    SURF_BTN_DUCK = 1 << 1,
    SURF_BTN_ATTACK = 1 << 2,
    SURF_BTN_USE = 1 << 3,
};

// Source engine IN_* button flags we translate to (bitflags in CUserCmd).
// These match the public Source SDK in_buttons.h values.
constexpr int IN_ATTACK = (1 << 0);
constexpr int IN_JUMP = (1 << 1);
constexpr int IN_DUCK = (1 << 2);
constexpr int IN_USE = (1 << 5);

// Translate a Python Buttons bitfield into engine IN_* flags.
inline int ToEngineButtons(int surf_buttons) {
    int out = 0;
    if (surf_buttons & SURF_BTN_JUMP) out |= IN_JUMP;
    if (surf_buttons & SURF_BTN_DUCK) out |= IN_DUCK;
    if (surf_buttons & SURF_BTN_ATTACK) out |= IN_ATTACK;
    if (surf_buttons & SURF_BTN_USE) out |= IN_USE;
    return out;
}

}  // namespace surf

#endif  // SURF_PROTOCOL_H
