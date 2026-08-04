#include "createmove_hook.h"

#include "game_offsets.h"
#include "sdk_types.h"
#include "session.h"
#include "vmt_hook.h"

// Calling convention of ClientModeShared::CreateMove. On Linux the Source
// client is compiled with the SysV cdecl 'this' passed as the first stack
// argument; on MSVC it is __thiscall. We normalize by taking `thisptr`
// explicitly and tagging the detour so the compiler emits a matching frame.
#if defined(_WIN32)
#define SURF_THISCALL __fastcall
#else
#define SURF_THISCALL
#endif

namespace surf {
namespace {

VmtHook g_hook;

using CreateMoveFn = bool(SURF_THISCALL*)(void* thisptr,
#if defined(_WIN32)
                                          void* edx,
#endif
                                          float sample_time, CUserCmd* cmd);

bool SURF_THISCALL DetourCreateMove(void* thisptr,
#if defined(_WIN32)
                                    void* edx,
#endif
                                    float sample_time, CUserCmd* cmd) {
    // Let the engine build its usual command first...
    CreateMoveFn original = g_hook.Original<CreateMoveFn>();
    bool result = original(thisptr,
#if defined(_WIN32)
                           edx,
#endif
                           sample_time, cmd);
    // ...then hand it to the session, which may overwrite it with the
    // model's action for this tick.
    GlobalSession().OnCreateMove(cmd);
    return result;
}

}  // namespace

bool InstallCreateMoveHook(void* client_mode) {
    return g_hook.Install(client_mode, kCreateMoveVtIndex,
                          reinterpret_cast<void*>(&DetourCreateMove));
}

void RemoveCreateMoveHook() { g_hook.Remove(); }

}  // namespace surf
