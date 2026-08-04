#include "ipc_client.h"

#include "surf_protocol.h"

#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#define SURF_CLOSE closesocket
#define SURF_BAD_SOCKET INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#define SURF_CLOSE ::close
#define SURF_BAD_SOCKET (-1)
#endif

namespace surf {
namespace {

#if defined(_WIN32)
struct WsaInit {
    WsaInit() {
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
    }
    ~WsaInit() { WSACleanup(); }
};
WsaInit g_wsa_init;  // one-time Winsock startup
#endif

// Minimal extractor: find "key":<number> in a flat JSON object. Sufficient
// for the small, well-formed command messages the harness emits. Returns
// false if the key is absent.
bool FindNumber(const std::string& json, const char* key, double* out) {
    std::string needle = std::string("\"") + key + "\"";
    size_t k = json.find(needle);
    if (k == std::string::npos) return false;
    size_t colon = json.find(':', k + needle.size());
    if (colon == std::string::npos) return false;
    *out = std::strtod(json.c_str() + colon + 1, nullptr);
    return true;
}

bool HasType(const std::string& json, const char* type) {
    std::string needle = std::string("\"type\":\"") + type + "\"";
    // tolerate whitespace-free encodings from the harness
    return json.find(needle) != std::string::npos;
}

// Extract "key":"value" from a flat JSON object. Assumes the value contains
// no escaped quotes (true for console commands the harness sends). Returns
// false if the key is absent.
bool FindString(const std::string& json, const char* key, std::string* out) {
    std::string needle = std::string("\"") + key + "\"";
    size_t k = json.find(needle);
    if (k == std::string::npos) return false;
    size_t colon = json.find(':', k + needle.size());
    if (colon == std::string::npos) return false;
    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return false;
    size_t q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return false;
    out->assign(json, q1 + 1, q2 - q1 - 1);
    return true;
}

}  // namespace

IpcClient::IpcClient() = default;
IpcClient::~IpcClient() { Disconnect(); }

bool IpcClient::Connect(const char* host, int port, double tick_interval) {
    Disconnect();
    int s = (int)::socket(AF_INET, SOCK_STREAM, 0);
    if (s == (int)SURF_BAD_SOCKET) return false;

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (::inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        SURF_CLOSE(s);
        return false;
    }
    if (::connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        SURF_CLOSE(s);
        return false;
    }
    int one = 1;
    ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));
    sock_ = s;
    rxbuf_.clear();

    char hello[128];
    std::snprintf(hello, sizeof(hello),
                  "{\"type\":\"hello\",\"tick_interval\":%.9g,\"protocol\":%d}\n",
                  tick_interval, kProtocolVersion);
    return SendLine(hello);
}

void IpcClient::Disconnect() {
    if (sock_ >= 0) {
        SURF_CLOSE(sock_);
        sock_ = -1;
    }
    rxbuf_.clear();
}

bool IpcClient::SendLine(const std::string& line) {
    if (sock_ < 0) return false;
    size_t sent = 0;
    while (sent < line.size()) {
        int n = (int)::send(sock_, line.data() + sent, (int)(line.size() - sent), 0);
        if (n <= 0) {
            Disconnect();
            return false;
        }
        sent += (size_t)n;
    }
    return true;
}

bool IpcClient::ReadLine(std::string& out, int timeout_ms) {
    // Serve a full line already buffered from a previous read.
    for (;;) {
        size_t nl = rxbuf_.find('\n');
        if (nl != std::string::npos) {
            out = rxbuf_.substr(0, nl);
            rxbuf_.erase(0, nl + 1);
            return true;
        }
        if (sock_ < 0) return false;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock_, &rfds);
        timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        int r = ::select(sock_ + 1, &rfds, nullptr, nullptr, &tv);
        if (r == 0) return false;  // timeout
        if (r < 0) {
            Disconnect();
            return false;
        }
        char buf[2048];
        int n = (int)::recv(sock_, buf, sizeof(buf), 0);
        if (n <= 0) {
            Disconnect();
            return false;
        }
        rxbuf_.append(buf, (size_t)n);
    }
}

CommandFrame IpcClient::Exchange(const StateFrame& s, int timeout_ms) {
    CommandFrame cmd;
    if (sock_ < 0) return cmd;

    char line[512];
    std::snprintf(
        line, sizeof(line),
        "{\"type\":\"state\",\"state\":{"
        "\"tick\":%lld,"
        "\"origin\":[%.4f,%.4f,%.4f],"
        "\"velocity\":[%.4f,%.4f,%.4f],"
        "\"view_yaw\":%.4f,\"view_pitch\":%.4f,"
        "\"on_ground\":%s,"
        "\"surface_normal\":[%.5f,%.5f,%.5f],"
        "\"extras\":{}},"
        "\"failed\":%s,\"completed\":%s}\n",
        s.tick, s.origin[0], s.origin[1], s.origin[2], s.velocity[0],
        s.velocity[1], s.velocity[2], s.view_yaw, s.view_pitch,
        s.on_ground ? "true" : "false", s.surface_normal[0],
        s.surface_normal[1], s.surface_normal[2], s.failed ? "true" : "false",
        s.completed ? "true" : "false");
    if (!SendLine(line)) return cmd;

    std::string reply;
    if (!ReadLine(reply, timeout_ms)) return cmd;  // valid stays false

    if (HasType(reply, "reset")) {
        cmd.is_reset = true;
        cmd.valid = true;
        FindString(reply, "command", &cmd.console_command);  // optional override
        return cmd;
    }
    if (HasType(reply, "command")) {
        // Console command only; no movement applied this tick.
        FindString(reply, "command", &cmd.console_command);
        cmd.valid = true;
        return cmd;
    }

    double v = 0.0;
    if (FindNumber(reply, "yaw_delta", &v)) cmd.yaw_delta = (float)v;
    if (FindNumber(reply, "pitch_delta", &v)) cmd.pitch_delta = (float)v;
    if (FindNumber(reply, "forwardmove", &v)) cmd.forwardmove = (float)v;
    if (FindNumber(reply, "sidemove", &v)) cmd.sidemove = (float)v;
    if (FindNumber(reply, "upmove", &v)) cmd.upmove = (float)v;
    if (FindNumber(reply, "buttons", &v)) cmd.buttons = (int)v;
    if (FindNumber(reply, "impulse", &v)) cmd.impulse = (int)v;
    FindString(reply, "command", &cmd.console_command);  // optional co-command
    cmd.has_movement = true;
    cmd.valid = true;
    return cmd;
}

}  // namespace surf
