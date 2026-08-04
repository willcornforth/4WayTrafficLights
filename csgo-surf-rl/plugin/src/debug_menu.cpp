#include "debug_menu.h"

#include "debug_state.h"
#include "imgui_support.h"

namespace surf {

void DrawDebugMenu() {
    DebugState& d = GlobalDebugState();
    if (!d.menu_visible) return;

    if (!ImGui::Begin("Surf RL Debug", &d.menu_visible)) {
        ImGui::End();
        return;
    }

    // --- live telemetry -------------------------------------------------
    ImGui::Text("Harness: %s", d.connected ? "connected" : "disconnected");
    ImGui::Text("In game: %s", d.in_game ? "yes" : "no");
    ImGui::Text("Tick: %lld   Timeouts: %lld", d.tick, d.timeouts);
    ImGui::Separator();
    ImGui::Text("Origin:   %8.1f %8.1f %8.1f", d.origin[0], d.origin[1], d.origin[2]);
    ImGui::Text("Velocity: %8.1f %8.1f %8.1f", d.velocity[0], d.velocity[1], d.velocity[2]);
    ImGui::Text("Speed 2D: %.1f u/s", d.speed_2d);
    ImGui::Text("View:     yaw %.1f  pitch %.1f  %s", d.view_yaw, d.view_pitch,
                d.on_ground ? "[ground]" : "[air]");
    ImGui::Text("Run:      %s", d.run_completed ? "COMPLETE"
                                : d.run_failed  ? "FAILED"
                                                : "in progress");
    ImGui::Separator();
    ImGui::Text("Last applied cmd:");
    ImGui::Text("  yaw d %.2f   fwd %.0f   side %.0f   btn %d", d.last_yaw_delta,
                d.last_forwardmove, d.last_sidemove, d.last_buttons);
    ImGui::Separator();

    // --- controls -------------------------------------------------------
    ImGui::Checkbox("Apply model input", &d.model_input_enabled);
    ImGui::Checkbox("Pause exchange", &d.paused);
    ImGui::Spacing();
    ImGui::Checkbox("Draw world overlay", &d.draw_overlay);
    ImGui::Checkbox("  velocity vector", &d.overlay_velocity);
    ImGui::Checkbox("  surface normal", &d.overlay_surface_normal);
    ImGui::Checkbox("  origin text", &d.overlay_text);
    ImGui::Separator();

    // --- manual input ---------------------------------------------------
    ImGui::Text("Manual console input:");
    ImGui::InputText("##cmd", d.command_buffer, sizeof(d.command_buffer));
    if (ImGui::Button("Run command")) d.run_command_requested = true;
    ImGui::SameLine();
    if (ImGui::Button("Reset run")) d.request_reset = true;

    ImGui::End();
}

}  // namespace surf
