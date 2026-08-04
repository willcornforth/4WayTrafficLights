// ImGui include switch.
//
// Define SURF_USE_IMGUI and add Dear ImGui to the build (see CMake IMGUI_ROOT)
// for the real UI. Otherwise this provides a minimal no-op shim of just the
// ImGui API the debug menu/backend call, so everything compiles and links
// without the library — the menu code is exercised, the calls simply do
// nothing. The shim is header-only inline, so including it in multiple
// translation units is fine.
#ifndef SURF_IMGUI_SUPPORT_H
#define SURF_IMGUI_SUPPORT_H

#ifdef SURF_USE_IMGUI
#include "imgui.h"
#else

#include <cstddef>

struct ImGuiContext;

namespace ImGui {

inline ImGuiContext* CreateContext() { return nullptr; }
inline void DestroyContext(ImGuiContext* = nullptr) {}
inline void NewFrame() {}
inline void Render() {}

inline bool Begin(const char*, bool* = nullptr, int = 0) { return true; }
inline void End() {}
inline void Text(const char*, ...) {}
inline void Separator() {}
inline void Spacing() {}
inline void SameLine(float = 0.0f, float = -1.0f) {}
inline bool Checkbox(const char*, bool*) { return false; }
inline bool Button(const char*) { return false; }
inline bool InputText(const char*, char*, size_t, int = 0) { return false; }
inline bool CollapsingHeader(const char*, int = 0) { return true; }

}  // namespace ImGui

#endif  // SURF_USE_IMGUI

#endif  // SURF_IMGUI_SUPPORT_H
