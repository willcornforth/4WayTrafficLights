// The ImGui debug menu contents. Backend-agnostic: it only issues ImGui
// commands against the shared DebugState, so the render backend just calls
// DrawDebugMenu() between NewFrame() and Render().
#ifndef SURF_DEBUG_MENU_H
#define SURF_DEBUG_MENU_H

namespace surf {

void DrawDebugMenu();

}  // namespace surf

#endif  // SURF_DEBUG_MENU_H
