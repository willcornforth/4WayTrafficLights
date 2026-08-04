// ImGui render backend: owns the ImGui context and the graphics-device hook
// that draws the menu each frame. Resolving the device and installing the
// present/paint hook is engine/platform specific and is the integration point
// a target build supplies (see ResolveRenderDevice in the .cpp). Until that is
// wired — and unless SURF_USE_IMGUI is defined — the backend stays inert and
// the in-world IVDebugOverlay is the working debug path.
#ifndef SURF_IMGUI_BACKEND_H
#define SURF_IMGUI_BACKEND_H

namespace surf {

class ImGuiBackend {
public:
    // Create the ImGui context, resolve the render device, and install the
    // present hook. Returns false (and stays inert) if imgui is not compiled
    // in or the device cannot be resolved.
    bool Initialize();
    void Shutdown();
    bool active() const { return active_; }

    // Build and submit one menu frame. The present hook calls this; exposed
    // so the integration point can invoke it from wherever it hooks.
    void RenderFrame();

private:
    bool active_ = false;
};

ImGuiBackend& GlobalImGuiBackend();

}  // namespace surf

#endif  // SURF_IMGUI_BACKEND_H
