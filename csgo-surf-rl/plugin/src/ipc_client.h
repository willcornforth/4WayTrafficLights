// Loopback TCP client that carries the surf telemetry protocol.
// Dependency-free: hand-rolled newline-framed JSON so the plugin needs no
// third-party JSON library to link against the Source SDK.
#ifndef SURF_IPC_CLIENT_H
#define SURF_IPC_CLIENT_H

#include <string>

namespace surf {

// The client-domain snapshot the plugin captures each CreateMove.
struct StateFrame {
    long long tick = 0;
    float origin[3] = {0, 0, 0};
    float velocity[3] = {0, 0, 0};
    float view_yaw = 0.0f;
    float view_pitch = 0.0f;
    bool on_ground = false;
    float surface_normal[3] = {0, 0, 1};
    bool failed = false;
    bool completed = false;
};

// The command the harness returns; deltas are degrees for this tick.
struct CommandFrame {
    float yaw_delta = 0.0f;
    float pitch_delta = 0.0f;
    float forwardmove = 0.0f;
    float sidemove = 0.0f;
    int buttons = 0;  // Python Buttons bitfield
    bool is_reset = false;  // harness asked us to reset the run
    bool valid = false;     // false => timeout/no reply this tick
};

class IpcClient {
public:
    IpcClient();
    ~IpcClient();

    // Connect to the harness and send the hello handshake. Returns false on
    // failure; the plugin should retry on a later tick.
    bool Connect(const char* host, int port, double tick_interval);
    void Disconnect();
    bool IsConnected() const { return sock_ >= 0; }

    // Send one state frame, then block up to timeout_ms for a command reply.
    // On timeout returns a CommandFrame with valid=false.
    CommandFrame Exchange(const StateFrame& state, int timeout_ms);

private:
    bool SendLine(const std::string& line);
    bool ReadLine(std::string& out, int timeout_ms);

    int sock_ = -1;      // fd on POSIX, cast SOCKET on Windows
    std::string rxbuf_;  // carries partial lines between reads
};

}  // namespace surf

#endif  // SURF_IPC_CLIENT_H
