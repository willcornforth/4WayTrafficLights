// Plugin entry point: implements IServerPluginCallbacks and, on level init,
// installs the CreateMove hook that drives the surf telemetry session.
#include "createmove_hook.h"
#include "server_plugin.h"
#include "session.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#define SURF_EXPORT extern "C" __declspec(dllexport)
#else
#include <dlfcn.h>
#define SURF_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace surf {

// Factory queries used by game_adapter.cpp. Filled in during Load().
static CreateInterfaceFn g_engine_factory = nullptr;
static CreateInterfaceFn g_client_factory = nullptr;

void* g_engine_factory_query(const char* name) {
    return g_engine_factory ? g_engine_factory(name, nullptr) : nullptr;
}
void* g_client_factory_query(const char* name) {
    return g_client_factory ? g_client_factory(name, nullptr) : nullptr;
}

namespace {

// Resolve client.dll's CreateInterface so we can reach client-side
// interfaces (entity list, client mode) the engine factory doesn't serve.
CreateInterfaceFn LoadClientFactory() {
#if defined(_WIN32)
    HMODULE mod = GetModuleHandleA("client.dll");
    if (!mod) return nullptr;
    return reinterpret_cast<CreateInterfaceFn>(GetProcAddress(mod, "CreateInterface"));
#else
    void* mod = dlopen("client.so", RTLD_NOLOAD | RTLD_NOW);
    if (!mod) return nullptr;
    return reinterpret_cast<CreateInterfaceFn>(dlsym(mod, "CreateInterface"));
#endif
}

// Resolve the ClientModeShared instance whose CreateMove we hook. This is
// build-specific (typically a sig-scan for g_pClientMode in client.dll) and
// is the integration point a target build must supply. Returns nullptr until
// implemented, so the plugin logs and declines to hook rather than crashing.
void* ResolveClientMode() {
    // TODO(integration): sig-scan client.dll for g_pClientMode on the target
    // build and return *g_pClientMode. Left unimplemented on purpose.
    return nullptr;
}

SessionConfig ConfigFromEnv() {
    SessionConfig cfg;
    if (const char* h = std::getenv("SURF_HARNESS_HOST")) cfg.host = h;
    if (const char* p = std::getenv("SURF_HARNESS_PORT")) cfg.port = std::atoi(p);
    if (const char* t = std::getenv("SURF_RESPONSE_TIMEOUT_MS"))
        cfg.response_timeout_ms = std::atoi(t);
    if (const char* r = std::getenv("SURF_RESET_COMMAND")) cfg.reset_command = r;
    return cfg;
}

class SurfPlugin : public IServerPluginCallbacks {
public:
    bool Load(CreateInterfaceFn engineFactory, CreateInterfaceFn) override {
        g_engine_factory = engineFactory;
        g_client_factory = LoadClientFactory();
        if (!g_client_factory) {
            std::printf("[surf] could not resolve client module factory\n");
            return false;
        }
        if (!GlobalSession().Initialize(ConfigFromEnv())) {
            std::printf("[surf] session init failed; plugin idle\n");
            // Return true so the plugin stays loaded and logs remain visible;
            // it simply does nothing until the integration points are set.
            return true;
        }
        std::printf("[surf] loaded\n");
        return true;
    }

    void Unload() override {
        RemoveCreateMoveHook();
        GlobalSession().Shutdown();
    }

    const char* GetPluginDescription() override {
        return "CSGO Surf RL telemetry bridge (client data domain)";
    }

    void LevelInit(const char* map_name) override {
        void* client_mode = ResolveClientMode();
        if (!client_mode) {
            std::printf(
                "[surf] client mode unresolved (see ResolveClientMode); "
                "CreateMove hook not installed on map '%s'\n",
                map_name ? map_name : "?");
            return;
        }
        if (InstallCreateMoveHook(client_mode)) {
            std::printf("[surf] CreateMove hook installed on '%s'\n", map_name);
        } else {
            std::printf("[surf] failed to install CreateMove hook\n");
        }
    }

    void LevelShutdown() override { RemoveCreateMoveHook(); }

    // Unused lifecycle callbacks.
    void Pause() override {}
    void UnPause() override {}
    void ServerActivate(edict_t*, int, int) override {}
    void GameFrame(bool) override {}
    void ClientActive(edict_t*) override {}
    void ClientDisconnect(edict_t*) override {}
    void ClientPutInServer(edict_t*, const char*) override {}
    void SetCommandClient(int) override {}
    void ClientSettingsChanged(edict_t*) override {}
    PLUGIN_RESULT ClientConnect(bool*, edict_t*, const char*, const char*, char*, int) override {
        return PLUGIN_CONTINUE;
    }
    PLUGIN_RESULT ClientCommand(edict_t*, const CCommand&) override {
        return PLUGIN_CONTINUE;
    }
    PLUGIN_RESULT NetworkIDValidated(const char*, const char*) override {
        return PLUGIN_CONTINUE;
    }
    void OnQueryCvarValueFinished(int, edict_t*, int, const char*, const char*) override {}
    void OnEdictAllocated(edict_t*) override {}
    void OnEdictFreed(const edict_t*) override {}
};

SurfPlugin g_plugin;

}  // namespace
}  // namespace surf

// The engine loads the plugin by calling this exported factory.
SURF_EXPORT void* CreateInterface(const char* name, int* return_code) {
    if (name && std::strcmp(name, INTERFACEVERSION_ISERVERPLUGINCALLBACKS) == 0) {
        if (return_code) *return_code = 0;
        return &surf::g_plugin;
    }
    if (return_code) *return_code = 1;
    return nullptr;
}
