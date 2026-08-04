// Installs/removes the CreateMove VMT hook on the client mode interface.
#ifndef SURF_CREATEMOVE_HOOK_H
#define SURF_CREATEMOVE_HOOK_H

namespace surf {

// `client_mode` is the ClientModeShared instance whose CreateMove(float,
// CUserCmd*) we intercept (resolved at plugin load). Returns false if the
// vtable slot could not be hooked.
bool InstallCreateMoveHook(void* client_mode);
void RemoveCreateMoveHook();

}  // namespace surf

#endif  // SURF_CREATEMOVE_HOOK_H
