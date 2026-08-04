// Minimal Source SDK type surface the plugin touches.
//
// When building against the real HL2SDK, define SURF_USE_HL2SDK and include
// the SDK headers instead; the layouts below mirror the public SDK so the
// hook logic compiles and reads correctly in either mode. They are the
// stable, publicly documented structures (Vector, QAngle, CUserCmd) — not
// anything reverse-engineered or anti-cheat-sensitive.
#ifndef SURF_SDK_TYPES_H
#define SURF_SDK_TYPES_H

#ifdef SURF_USE_HL2SDK
#include "cdll_int.h"
#include "icliententitylist.h"
#include "in_buttons.h"
#include "mathlib/vector.h"
#include "usercmd.h"
#else

struct Vector {
    float x = 0, y = 0, z = 0;
};

struct QAngle {
    float x = 0, y = 0, z = 0;  // pitch, yaw, roll
};

// Public CUserCmd layout (Source SDK usercmd.h). Only the fields the plugin
// reads/writes are annotated; the rest preserve offsets.
struct CUserCmd {
    void* vtable;
    int command_number;
    int tick_count;
    QAngle viewangles;   // absolute view angles this tick
    Vector aimdirection;
    float forwardmove;
    float sidemove;
    float upmove;
    int buttons;         // IN_* bitfield
    unsigned char impulse;
    int weaponselect;
    int weaponsubtype;
    int random_seed;
    short mousedx;
    short mousedy;
    bool hasbeenpredicted;
    // ... remaining fields unused by the plugin
};

// The subset of the client engine interface (IVEngineClient) we call.
class IVEngineClient {
public:
    virtual ~IVEngineClient() {}
    virtual int GetLocalPlayer() = 0;
    virtual void GetViewAngles(QAngle& va) = 0;
    virtual void SetViewAngles(QAngle& va) = 0;
    virtual bool IsInGame() = 0;
    virtual void ClientCmd_Unrestricted(const char* cmd) = 0;
};

// Opaque client entity; real accessors live in the game adapter, which reads
// netvars via build-specific offsets (see game_offsets.h).
class IClientEntity;

class IClientEntityList {
public:
    virtual IClientEntity* GetClientEntity(int index) = 0;
};

#endif  // SURF_USE_HL2SDK

#endif  // SURF_SDK_TYPES_H
