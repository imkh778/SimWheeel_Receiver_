#ifndef Lib_Simwheel_H
#define Lib_Simwheel_H

// ─── Standard library ────────────────────────────────────────────────────────
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <csignal>

// ─── POSIX / Linux networking (replaces winsock2.h / ws2tcpip.h / iphlpapi) ──
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>          // replaces iphlpapi GetAdaptersAddresses
#include <net/if.h>
#include <unistd.h>

// ─── Linux uinput (replaces vJoyInterface.h) ─────────────────────────────────
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <sys/ioctl.h>

// ─── nlohmann/json (same as Windows version – header-only, cross-platform) ───
#include <nlohmann/json.hpp>

// ─── Windows type aliases (keep original code changes minimal) ───────────────
using UINT  = unsigned int;
using UCHAR = unsigned char;
using LONG  = long;
using DWORD = unsigned long;

// ─── vJoy axis range kept identical to Windows version [0, 32768] ────────────
static constexpr int VJOY_AXIS_MIN =     0;
static constexpr int VJOY_AXIS_MAX = 32768;

// ─── Shared uinput file descriptor (set in vjoy_functions.cpp) ───────────────
extern int g_uinput_fd;

// ─── Settings struct ─────────────────────────────────────────────────────────
struct Settings {
    double steering_range;
    bool   is_log;
};

// ─── Function declarations ───────────────────────────────────────────────────
void    ShowLocalIP();
Settings getSettings();
double  userSteering();
LONG    MapToVJoyAxis(double norm);
void    pressEnterToExit();

// vJoy → uinput shim API (same names as original)
void    SetVJoyButton(UINT btnNumber, bool pressed, UINT vJoyId);
bool    vjoyeorrs(UINT vJoyId);          // returns true on error (same semantics)
void    RelinquishVJD(UINT vJoyId);
void    SetAxis(LONG value, UINT vJoyId, int hidUsage);

// HID usage constants (same values used in core.cpp)
static constexpr int HID_USAGE_X  = 0x30;
static constexpr int HID_USAGE_Y  = 0x31;
static constexpr int HID_USAGE_Z  = 0x32;
static constexpr int HID_USAGE_RZ = 0x35;

void    processInput(const std::string& buffer, int vJoyId, double userRange);

#endif // Lib_Simwheel_H
