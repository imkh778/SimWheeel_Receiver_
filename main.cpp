/**
 * main.cpp  –  Linux port of SimWheel Receiver
 *
 * Changes from Windows version:
 *   - WSAStartup / WSACleanup removed  (not needed on Linux)
 *   - SOCKET / INVALID_SOCKET / SOCKET_ERROR  → int / -1
 *   - closesocket()  → close()
 *   - InetNtopW (wide-char)  → inet_ntop (narrow char, standard)
 *   - socklen_t used instead of int for recvfrom length arg
 *   - RelinquishVJD called on exit via atexit / signal handler
 */

#include "Lib_Simwheel.h"
#include <cstdio>
#include <cerrno>

static UINT vJoyId = 1;

// ─── Graceful shutdown on Ctrl-C ─────────────────────────────────────────────
static int g_sock = -1;

static void signal_handler(int /*sig*/)
{
    std::cout << "\nShutting down...\n";
    RelinquishVJD(vJoyId);
    if (g_sock >= 0) close(g_sock);
    std::exit(0);
}

// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    ShowLocalIP();

    Settings usersetting = getSettings();

    std::cout << "\n **Settings**\n"
              << "1. Default steering range enabled: "
              << (usersetting.steering_range == 0 ? "No" : "Yes") << "\n"
              << "2. Logging enabled: "
              << (usersetting.is_log ? "Yes" : "No") << "\n\n";

    // ── Steering range ────────────────────────────────────────────────────────
    double userRange;
    if (usersetting.steering_range == 0) {
        userRange = userSteering();
    } else {
        userRange = usersetting.steering_range;
        if (userRange < 90.0 || userRange > 2520.0) {
            std::cerr << "Steering range out of bounds (90-2520). "
                         "Using default (900). Please fix settings.json.\n";
            userRange = 900.0;
        }
        std::cout << "Using steering range: " << userRange << "\n";
    }

    // ── Initialise uinput virtual gamepad (replaces vJoy init) ───────────────
    if (vjoyeorrs(vJoyId)) {
        return 1;
    }
    std::cout << "uinput virtual controller acquired (device slot #" << vJoyId << ")\n";

    // ── Create UDP socket ─────────────────────────────────────────────────────
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << "\n";
        RelinquishVJD(vJoyId);
        return 1;
    }
    g_sock = sock;

    // Allow quick reuse of the port after restart
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    // ── Bind to port 4567 (same as Windows version) ───────────────────────────
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(4567);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed on port 4567: " << strerror(errno) << "\n";
        close(sock);
        RelinquishVJD(vJoyId);
        return 1;
    }
    std::cout << "Listening for UDP on port 4567...\n";

    // ── Receive loop ─────────────────────────────────────────────────────────
    const int  bufSize = 512;
    char       buffer[bufSize];
    sockaddr_in client{};
    socklen_t  clientLen = sizeof(client);

    while (true) {
        int bytes = static_cast<int>(
            recvfrom(sock, buffer, bufSize - 1, 0,
                     reinterpret_cast<sockaddr*>(&client), &clientLen)
        );

        if (bytes < 0) {
            std::cerr << "recvfrom() error: " << strerror(errno) << "\n";
            break;
        }

        buffer[bytes] = '\0';
        std::string msg(buffer);

        // Print sender IP (replaces InetNtopW wide-char version)
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ipStr, sizeof(ipStr));

        if (usersetting.is_log) {
            std::cout << "Received [" << ipStr << "] -> " << msg << "\n";
        }

        // Core function: parse JSON and forward to virtual controller
        processInput(buffer, static_cast<int>(vJoyId), userRange);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    RelinquishVJD(vJoyId);
    close(sock);
    return 0;
}
