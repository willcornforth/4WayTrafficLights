// Minimal IServerPluginCallbacks (ISERVERPLUGINCALLBACKS003) surface.
//
// Source loads a plugin listed in addons / via `plugin_load` by calling the
// module's exported CreateInterface for this interface, then Load(). On a
// listen server the plugin runs inside the same process as the client, so
// client-side interfaces (engine, entity list, client mode) are reachable —
// which is what a local surf-training setup uses.
//
// With the real SDK, include engine/iserverplugin.h instead of this shim.
#ifndef SURF_SERVER_PLUGIN_H
#define SURF_SERVER_PLUGIN_H

#ifdef SURF_USE_HL2SDK
#include "engine/iserverplugin.h"
#else

using CreateInterfaceFn = void* (*)(const char* name, int* return_code);

enum PLUGIN_RESULT { PLUGIN_CONTINUE = 0, PLUGIN_OVERRIDE, PLUGIN_STOP };

// Opaque engine types the plugin does not dereference.
using edict_t = void;
class CCommand;

class IServerPluginCallbacks {
public:
    virtual bool Load(CreateInterfaceFn interfaceFactory,
                      CreateInterfaceFn gameServerFactory) = 0;
    virtual void Unload() = 0;
    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual const char* GetPluginDescription() = 0;
    virtual void LevelInit(const char* map_name) = 0;
    virtual void ServerActivate(edict_t* edicts, int edict_count, int client_max) = 0;
    virtual void GameFrame(bool simulating) = 0;
    virtual void LevelShutdown() = 0;
    virtual void ClientActive(edict_t* entity) = 0;
    virtual void ClientDisconnect(edict_t* entity) = 0;
    virtual void ClientPutInServer(edict_t* entity, const char* name) = 0;
    virtual void SetCommandClient(int index) = 0;
    virtual void ClientSettingsChanged(edict_t* edict) = 0;
    virtual PLUGIN_RESULT ClientConnect(bool* allow, edict_t* entity,
                                        const char* name, const char* address,
                                        char* reject, int reject_len) = 0;
    virtual PLUGIN_RESULT ClientCommand(edict_t* entity, const CCommand& args) = 0;
    virtual PLUGIN_RESULT NetworkIDValidated(const char* name, const char* net_id) = 0;
    virtual void OnQueryCvarValueFinished(int cookie, edict_t* ent, int status,
                                          const char* name, const char* value) = 0;
    virtual void OnEdictAllocated(edict_t* edict) = 0;
    virtual void OnEdictFreed(const edict_t* edict) = 0;
};

#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS003"

#endif  // SURF_USE_HL2SDK

#endif  // SURF_SERVER_PLUGIN_H
