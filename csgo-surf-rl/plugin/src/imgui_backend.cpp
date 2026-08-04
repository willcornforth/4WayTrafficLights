#include "imgui_backend.h"

#include "debug_menu.h"
#include "imgui_support.h"

#include <cstdio>

namespace surf {

ImGuiBackend& GlobalImGuiBackend() {
    static ImGuiBackend backend;
    return backend;
}

namespace {

// Integration point: return the graphics device/swapchain to draw on (the
// D3D9 device on the CS:GO-era Windows client, or the GL context on Linux),
// typically resolved from the material system. Returns nullptr until a target
// build wires it up, which keeps the ImGui backend disabled without crashing.
void* ResolveRenderDevice() {
    // TODO(integration): resolve the render device for the target build and
    // install a present/paint hook that calls GlobalImGuiBackend().RenderFrame().
    return nullptr;
}

}  // namespace

bool ImGuiBackend::Initialize() {
#ifndef SURF_USE_IMGUI
    std::printf(
        "[surf] imgui not compiled in (build with -DIMGUI_ROOT=...); "
        "menu disabled, in-world overlay still active\n");
    return false;
#else
    void* device = ResolveRenderDevice();
    if (!device) {
        std::printf("[surf] render device unresolved; imgui menu disabled\n");
        return false;
    }
    ImGui::CreateContext();
    // TODO(integration): ImGui platform+renderer backend init against `device`,
    // and install the present hook that calls RenderFrame().
    active_ = true;
    std::printf("[surf] imgui backend active\n");
    return true;
#endif
}

void ImGuiBackend::Shutdown() {
    if (!active_) return;
    // TODO(integration): shut down the platform+renderer backends first.
    ImGui::DestroyContext();
    active_ = false;
}

void ImGuiBackend::RenderFrame() {
    if (!active_) return;
    // TODO(integration): platform+renderer NewFrame() calls belong here.
    ImGui::NewFrame();
    DrawDebugMenu();
    ImGui::Render();
    // TODO(integration): submit ImGui::GetDrawData() to the renderer here.
}

}  // namespace surf
