// Build-specific integration points, isolated so a new game build only
// touches this file. These are the numbers that drift between engine
// versions; the hook logic never hard-codes them.
#ifndef SURF_GAME_OFFSETS_H
#define SURF_GAME_OFFSETS_H

namespace surf {

// Interface version strings exported by the client/engine modules, resolved
// through the SDK's CreateInterface at plugin load.
constexpr const char* kEngineClientIface = "VEngineClient014";
constexpr const char* kClientEntityListIface = "VClientEntityList003";
constexpr const char* kBaseClientIface = "VClient018";
constexpr const char* kDebugOverlayIface = "VDebugOverlay004";

// VMT index of the function we hook. On the CS:GO-era client this is the
// client mode's CreateMove(float, CUserCmd*). Confirm against the target
// build before use; wrong index = wrong hook, so this is deliberately a
// single named constant.
constexpr int kCreateMoveVtIndex = 24;

// Netvar byte offsets into C_BasePlayer for the fields we read. Populate
// from the target build's netvar dump. Left at 0 they must be filled in
// before the adapter can read entity state (the plugin logs and refuses to
// run with unset offsets rather than reading garbage).
namespace netvar {
constexpr int m_vecOrigin = 0;    // Vector: predicted origin
constexpr int m_vecVelocity = 0;  // Vector: predicted velocity
constexpr int m_fFlags = 0;       // int: FL_ONGROUND lives here (1<<0)
}  // namespace netvar

constexpr int FL_ONGROUND = (1 << 0);

}  // namespace surf

#endif  // SURF_GAME_OFFSETS_H
