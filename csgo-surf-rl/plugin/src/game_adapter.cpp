#include "game_adapter.h"

#include "game_offsets.h"
#include "surf_protocol.h"

#include <cmath>
#include <cstdio>

// CreateInterface factory access. With the real SDK this comes from
// tier1/interface.h + the module handles the plugin receives at load. Here
// we declare the two factories the plugin is handed in Load().
namespace surf {

// Provided by plugin.cpp from the interface factories passed to Load().
extern void* g_engine_factory_query(const char* name);
extern void* g_client_factory_query(const char* name);

namespace {

template <typename T>
T ReadOffset(const void* base, int offset) {
    return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(base) + offset);
}

bool OffsetsConfigured() {
    return netvar::m_vecOrigin != 0 && netvar::m_vecVelocity != 0 &&
           netvar::m_fFlags != 0;
}

float NormalizeAngle(float a) {
    while (a > 180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
    return a;
}

}  // namespace

bool GameAdapter::Initialize() {
    engine_ = reinterpret_cast<IVEngineClient*>(
        g_engine_factory_query(kEngineClientIface));
    entities_ = reinterpret_cast<IClientEntityList*>(
        g_client_factory_query(kClientEntityListIface));
    if (!engine_ || !entities_) {
        std::printf("[surf] missing engine/entity interface\n");
        return false;
    }
    if (!OffsetsConfigured()) {
        // Refuse to read garbage: the integrator must fill game_offsets.h.
        std::printf(
            "[surf] netvar offsets not configured in game_offsets.h; "
            "state capture disabled\n");
        return false;
    }
    ready_ = true;
    return true;
}

bool GameAdapter::IsInGame() const {
    return engine_ && engine_->IsInGame();
}

IClientEntity* GameAdapter::LocalPlayer() const {
    if (!engine_ || !entities_) return nullptr;
    int idx = engine_->GetLocalPlayer();
    if (idx <= 0) return nullptr;
    return entities_->GetClientEntity(idx);
}

bool GameAdapter::CaptureState(long long tick, StateFrame* out) const {
    IClientEntity* player = LocalPlayer();
    if (!player) return false;

    Vector origin = ReadOffset<Vector>(player, netvar::m_vecOrigin);
    Vector vel = ReadOffset<Vector>(player, netvar::m_vecVelocity);
    int flags = ReadOffset<int>(player, netvar::m_fFlags);

    QAngle va;
    engine_->GetViewAngles(va);

    out->tick = tick;
    out->origin[0] = origin.x;
    out->origin[1] = origin.y;
    out->origin[2] = origin.z;
    out->velocity[0] = vel.x;
    out->velocity[1] = vel.y;
    out->velocity[2] = vel.z;
    out->view_yaw = va.y;
    out->view_pitch = va.x;
    out->on_ground = (flags & FL_ONGROUND) != 0;
    // surface_normal defaults to up; a client-side trace can fill this in.
    out->surface_normal[0] = 0.0f;
    out->surface_normal[1] = 0.0f;
    out->surface_normal[2] = 1.0f;
    // failed/completed are populated by trigger-event wiring, not here.
    return true;
}

void GameAdapter::ApplyCommand(const CommandFrame& cmd, CUserCmd* usercmd) {
    if (!usercmd) return;

    QAngle va;
    engine_->GetViewAngles(va);
    va.x = NormalizeAngle(va.x + cmd.pitch_delta);
    if (va.x > 89.0f) va.x = 89.0f;
    if (va.x < -89.0f) va.x = -89.0f;
    va.y = NormalizeAngle(va.y + cmd.yaw_delta);

    // Write the resulting absolute angles into the outgoing command and keep
    // the engine view consistent so the model's deltas are what the server
    // actually receives.
    usercmd->viewangles = va;
    engine_->SetViewAngles(va);

    usercmd->forwardmove = cmd.forwardmove;
    usercmd->sidemove = cmd.sidemove;
    usercmd->buttons = ToEngineButtons(cmd.buttons);
}

void GameAdapter::RunConsoleCommand(const char* command) {
    if (engine_) engine_->ClientCmd_Unrestricted(command);
}

}  // namespace surf
