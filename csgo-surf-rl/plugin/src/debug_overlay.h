// In-world debug visualization built on the engine's IVDebugOverlay. Unlike
// the ImGui menu it needs no graphics device hook — the engine draws these
// overlays itself — so it works the moment the interface resolves.
#ifndef SURF_DEBUG_OVERLAY_H
#define SURF_DEBUG_OVERLAY_H

#include "debug_state.h"
#include "ipc_client.h"  // StateFrame
#include "sdk_types.h"

namespace surf {

class DebugOverlay {
public:
    // Resolve IVDebugOverlay from the engine factory. Returns false (and the
    // overlay stays inert) if unavailable.
    bool Initialize();
    bool IsReady() const { return overlay_ != nullptr; }

    // Draw the enabled overlays for this tick's state. Cheap; safe to call
    // every CreateMove.
    void Draw(const StateFrame& state, const DebugState& controls);

private:
    IVDebugOverlay* overlay_ = nullptr;
};

}  // namespace surf

#endif  // SURF_DEBUG_OVERLAY_H
