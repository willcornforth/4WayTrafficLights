#include "debug_overlay.h"

#include "game_offsets.h"

#include <cmath>
#include <cstdio>

namespace surf {

// Provided by plugin.cpp (engine interface factory).
extern void* g_engine_factory_query(const char* name);

bool DebugOverlay::Initialize() {
    overlay_ = reinterpret_cast<IVDebugOverlay*>(
        g_engine_factory_query(kDebugOverlayIface));
    if (!overlay_) {
        std::printf("[surf] debug overlay interface unavailable\n");
        return false;
    }
    return true;
}

void DebugOverlay::Draw(const StateFrame& s, const DebugState& d) {
    if (!overlay_) return;

    Vector origin{s.origin[0], s.origin[1], s.origin[2]};
    const float duration = 0.1f;  // redrawn every tick; kept short

    if (d.overlay_velocity) {
        // Scale velocity down so the vector is a readable length in-world.
        Vector tip{origin.x + s.velocity[0] * 0.15f,
                   origin.y + s.velocity[1] * 0.15f,
                   origin.z + s.velocity[2] * 0.15f};
        overlay_->AddLineOverlay(origin, tip, 0, 255, 0, true, duration);
    }
    if (d.overlay_surface_normal) {
        Vector tip{origin.x + s.surface_normal[0] * 64.0f,
                   origin.y + s.surface_normal[1] * 64.0f,
                   origin.z + s.surface_normal[2] * 64.0f};
        overlay_->AddLineOverlay(origin, tip, 0, 128, 255, true, duration);
    }
    if (d.overlay_text) {
        float speed_2d = std::sqrt(s.velocity[0] * s.velocity[0] +
                                   s.velocity[1] * s.velocity[1]);
        overlay_->AddTextOverlay(origin, 0, duration, "tick %lld", s.tick);
        overlay_->AddTextOverlay(origin, 1, duration, "speed %.0f", speed_2d);
        overlay_->AddTextOverlay(origin, 2, duration, "%s",
                                 s.on_ground ? "ground" : "air");
    }
}

}  // namespace surf
