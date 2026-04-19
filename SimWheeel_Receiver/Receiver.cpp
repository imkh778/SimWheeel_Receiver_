
// pc_udp_receiver_with_vjoy.cpp
// UDP receiver with vJoy integration: listens on port 4567, parses controls,
// and feeds them into a vJoy virtual joystick.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include "vJoyInterface.h"
#include <nlohmann/json.hpp>
#include <iphlpapi.h>
#include <fstream>
#include <vector>
#include <utility>
#include <unordered_set>


#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "vJoyInterface.lib")

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

void EnableVirtualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

using json = nlohmann::json;

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}


// For convenience, use the nlohmann::json namespace.
using json = nlohmann::json;

bool IsRunAsAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&SIDAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup)) {
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        FreeSid(pAdministratorsGroup);
    }
    return fIsRunAsAdmin;
}

static void MouseClick(int button, bool isPressed) {
    INPUT in = {}; // Zero-initializes everything. dx, dy, time are now 0.
    in.type = INPUT_MOUSE;

    if (button == 500) {
        in.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }
    else if (button == 501) {
        in.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    }
    else if (button == 503) {
        in.mi.dwFlags = isPressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    }

    SendInput(1, &in, sizeof(INPUT));
}

WORD customKeyToVK(int code) {

    // -------------------------------
    // LETTERS (A–Z)
    // Custom: 200–225
    // VK_A = 0x41
    // -------------------------------
    if (code >= 200 && code <= 225) {
        return WORD(0x41 + (code - 200));  // 200→A, 201→B, etc.
    }

    // -------------------------------
    // NUMBERS 0–9
    // Custom: 300–309
    // VK_0 = 0x30, VK_1 = 0x31, etc
    // -------------------------------
    if (code >= 300 && code <= 309) {
        if (code == 300) return 0X30;
        return WORD(0x30 + (code - 300));  // 301→1, 302→2, etc.
    }

    // -------------------------------
    // FUNCTION KEYS F1–F12
    // Custom: 400–411
    // VK_F1 = 0x70
    // -------------------------------
    if (code >= 400 && code <= 411) {
        return WORD(VK_F1 + (code - 400));
    }

    // -------------------------------
    // SPECIAL KEYS
    // -------------------------------

    if (code == 230) return VK_SPACE;
    if (code == 231) return VK_RETURN;     // Enter
    if (code == 232) return VK_BACK;       // Backspace
    if (code == 233) return VK_TAB;        // Tab
    if (code == 234) return VK_SHIFT;      // Shift
    if (code == 235) return VK_CONTROL;    // Ctrl
    if (code == 236) return VK_MENU;       // Alt
    if (code == 237) return VK_LWIN;       // Left Windows key
    if (code == 238) return VK_ESCAPE;     // ESC
    if (code == 239) return VK_CAPITAL;    // Caps Lock

    // -------------------------------
    // SYMBOL KEYS
    // -------------------------------
    if (code == 250) return VK_OEM_MINUS;     // '-'
    if (code == 251) return VK_OEM_PLUS;      // '='
    if (code == 252) return VK_OEM_4;         // '['
    if (code == 253) return VK_OEM_6;         // ']'
    if (code == 254) return VK_OEM_5;         // '\'
    if (code == 255) return VK_OEM_1;         // ';'
    if (code == 256) return VK_OEM_7;         // '''
    if (code == 257) return VK_OEM_COMMA;     // ','
    if (code == 258) return VK_OEM_PERIOD;    // '.'
    if (code == 259) return VK_OEM_2;         // '/'

    // -------------------------------
    // ARROWS
    // Custom: 350–353
    // -------------------------------
    if (code == 350) return VK_LEFT;
    if (code == 351) return VK_RIGHT;
    if (code == 352) return VK_UP;
    if (code == 353) return VK_DOWN;

    // -------------------------------
    // PAGE KEYS
    // -------------------------------
    if (code == 360) return VK_HOME;
    if (code == 361) return VK_END;
    if (code == 362) return VK_PRIOR; // Page Up
    if (code == 363) return VK_NEXT;  // Page Down

    // -------------------------------
    // DELETE / INSERT
    // -------------------------------
    if (code == 370) return VK_DELETE;
    if (code == 371) return VK_INSERT;

    //Mouse
    //if (code == 372)return VK_LBUTTON;   // Left mouse button
    //if (code == 373)return VK_RBUTTON;  // Right mouse button
    //if (code == 374)return VK_MBUTTON; // Middle mouse button (wheel click)

    // Unknown key
    return 0;
}

static void keyBoardEvents(int code, bool isPressed) {
    WORD vk = customKeyToVK(code);
    if (!vk) return;


    // 1. Map the Virtual Key to a Hardware Scan Code
    // ETS2 reads this, not the wVk
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0; // Set to 0 because we are using wScan
    in.ki.wScan = (WORD)scanCode;
    in.ki.time = 0;
    in.ki.dwExtraInfo = 0;

    // 2. Set the Base Flag to indicate we are sending a Scan Code
    in.ki.dwFlags = KEYEVENTF_SCANCODE;

    // 3. Handle Extended Keys (Arrow keys, Insert, Delete, Home, End, etc.)
    // Without this, arrow keys might be interpreted as Numpad keys by the game.
    switch (vk) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: // PageUp, PageDown
    case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE:
    case VK_DIVIDE: case VK_NUMLOCK:
        in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        break;

    }

    // 4. Handle Key Release
    if (!isPressed) {
        in.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    SendInput(1, &in, sizeof(INPUT));
}

void moveMouse(int dx, int dy) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    // MOUSEEVENTF_MOVE without MOUSEEVENTF_ABSOLUTE = Relative movement (Delta)
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    input.mi.dx = dx * 2; // Sensitivity multiplier
    input.mi.dy = dy * 2;

    // This function returns the number of events inserted. 
    // If it returns 0, it was blocked. 
    if (SendInput(1, &input, sizeof(INPUT)) == 0) {
        std::cerr << "\033[91mInput blocked! Run as Admin.\033[0m\n";
    }
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

void ShowLocalIP() {
    ULONG bufLen = 15000;
    IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, addresses, &bufLen) == NO_ERROR) {

        bool found = false;
        std::cout << "\033[93m--- AVAILABLE IPs FOR MANUAL APP CONNECTION ---\033[0m\n";
        for (IP_ADAPTER_ADDRESSES* addr = addresses; addr != NULL; addr = addr->Next) {
            if (addr->OperStatus != IfOperStatusUp) continue;

            std::string desc = WStringToString(addr->Description);
            std::string lowerDesc = desc;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);

            // Skip VirtualBox noise
            if (lowerDesc.find("virtualbox") != std::string::npos) continue;

            IP_ADAPTER_UNICAST_ADDRESS* ua = addr->FirstUnicastAddress;
            while (ua) {
                sockaddr_in* sa = (sockaddr_in*)ua->Address.lpSockaddr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
                std::string ipStr = ip;

                if (ipStr != "127.0.0.1") {
                    if (addr->IfType == IF_TYPE_IEEE80211) {
                        std::cout << "\033[95mWiFi IP: " << ipStr << " <- (Make sure phone and PC are on the same WiFi)\033[0m \n" << std::endl;
                        found = true;
                    }
                    else if (lowerDesc.find("usb") != std::string::npos || lowerDesc.find("ndis") != std::string::npos) {
                        std::cout << "\n\033[96mUSB IP : " << ipStr << " <- Best for low latency\033[0m \n" << std::endl;
                        found = true;
                    }
                }
                ua = ua->Next;
            }
        }

    }
    free(addresses);
}

std::string GetConnectionType(const sockaddr_in& clientAddr) {
    ULONG bufLen = 15000;
    IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);
    std::string connType = "unknown";

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, addresses, &bufLen) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES* addr = addresses; addr != NULL; addr = addr->Next) {
            if (addr->OperStatus != IfOperStatusUp) continue;

            bool isWifi = (addr->IfType == IF_TYPE_IEEE80211);
            std::string desc = WStringToString(addr->Description);
            std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
            bool isUsb = (desc.find("usb") != std::string::npos || desc.find("ndis") != std::string::npos);

            IP_ADAPTER_UNICAST_ADDRESS* ua = addr->FirstUnicastAddress;
            while (ua) {
                sockaddr_in* sa = (sockaddr_in*)ua->Address.lpSockaddr;
                if (sa->sin_family == AF_INET) {
                    uint32_t pcIp = ntohl(sa->sin_addr.s_addr);
                    uint32_t cIp = ntohl(clientAddr.sin_addr.s_addr);

                    if ((pcIp & 0xFFFFFF00) == (cIp & 0xFFFFFF00)) {
                        if (isWifi) connType = "wifi";
                        else if (isUsb) connType = "usb";
                        else connType = "ethernet";

                        free(addresses);
                        return connType;
                    }
                }
                ua = ua->Next;
            }
        }
    }
    free(addresses);
    return connType;
}

// Console Dashboard State
struct DashboardState {
    std::string deviceName = "Searching...";
    std::string connection = "none";
    double steering = 0.0;
    double throttle = 0.0;
    double brake = 0.0;

    bool hasClutch = false;
    double clutch = 0.0;

    bool hasZAxis = false;
    double zaxis = 0.0;

    std::string lastLog = "Waiting for data...";
    bool initialized = false;
    double userRange = 900.0;
} g_Dash;

std::string DrawBar(double value, double minVal, double maxVal, int width) {
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;
    double range = maxVal - minVal;
    double pct = (range == 0) ? 0 : (value - minVal) / range;

    int fillCount = static_cast<int>(pct * width);
    std::string bar = "[";
    for (int i = 0; i < width; i++) {
        if (i < fillCount) bar += "=";
        else if (i == fillCount) bar += "|";
        else bar += " ";
    }
    bar += "]";

    char buf[64];
    if (minVal < 0) {
        snprintf(buf, sizeof(buf), " (%5.1f deg)", value);
    }
    else {
        snprintf(buf, sizeof(buf), " (%3.0f%%)", pct * 100);
    }
    return bar + buf;
}

void UpdateDashboard() {
    int lines = 7;
    if (g_Dash.hasClutch) lines++;
    if (g_Dash.hasZAxis) lines++;

    // Re-reserve space if number of lines increases
    static int lastLinesCount = lines;
    if (lines > lastLinesCount) {
        g_Dash.initialized = false;
        lastLinesCount = lines;
    }

    if (!g_Dash.initialized) {
        for (int i = 0; i < lines; i++) std::cout << "\n";
        g_Dash.initialized = true;
    }

    std::cout << "\033[" << lines << "A";
    std::cout << "\033[K" << "\033[36m>> STATUS: \033[0m" << g_Dash.lastLog << "\n";
    std::cout << "\033[K" << "------------------------------------------------\n";
    std::cout << "\033[K" << " Connection : " << g_Dash.deviceName << " [" << g_Dash.connection << "]\n";

    std::cout << "\033[K" << " Steering   : " << DrawBar(g_Dash.steering, -g_Dash.userRange, g_Dash.userRange, 20) << "\n";
    std::cout << "\033[K" << " Throttle   : " << DrawBar(g_Dash.throttle, 0.0, 1.0, 20) << "\n";
    std::cout << "\033[K" << " Brake      : " << DrawBar(g_Dash.brake, 0.0, 1.0, 20) << "\n";

    if (g_Dash.hasClutch) {
        std::cout << "\033[K" << " Clutch     : " << DrawBar(g_Dash.clutch, 0.0, 1.0, 20) << "\n";
    }
    if (g_Dash.hasZAxis) {
        std::cout << "\033[K" << " Z-Axis     : " << DrawBar(g_Dash.zaxis, 0.0, 1.0, 20) << "\n";
    }

    std::cout << "\033[K" << "------------------------------------------------\n";
    std::flush(std::cout);
}

void DashLog(const std::string& msg) {
    if (g_Dash.initialized) {
        int lines = 7;
        if (g_Dash.hasClutch) lines++;
        if (g_Dash.hasZAxis) lines++;
        std::cout << "\033[" << lines << "A"; // move cursor to top of dashboard
        std::cout << "\033[J";  // clear from cursor to end of screen
        std::cout << msg << "\n";
        g_Dash.initialized = false;
        UpdateDashboard();
    }
    else {
        std::cout << msg << "\n";
    }
}

// Map a normalized [-1,1] value to vJoy axis range [0,0x8000]
LONG MapToVJoyAxis(double norm) {
    if (norm < -1.0) norm = -1.0;
    if (norm > 1.0) norm = 1.0;
    return static_cast<LONG>(16384 + norm * 16384);
}

double userSteering() {
    double userRange = 900.0; // Default
    std::string input;

    std::cout << "\n\033[92mEnter steering range (min 90, max 2520), or press Enter for default (900) :\033[0m";
    std::getline(std::cin, input);

    if (!input.empty()) {
        try {
            double tempRange = std::stod(input);
            if (tempRange >= 90.0 && tempRange <= 2520.0) {
                userRange = tempRange;
            }
            else {
                std::cerr << "Range too low or high. Using default (900).\n" << std::endl;
            }
        }
        catch (...) {
            std::cerr << "Invalid input. Using default (900)." << std::endl;
        }
    }

    std::cout << "Using steering range: " << userRange << std::endl;

    return userRange;
}

void pressEnterToExit() {
    std::cout << "Press Enter to exit...";
    std::cin.ignore(10000, '\n');
    std::cin.get();
}

UINT vJoyId = 1;

void SetVJoyButton(UINT btnNumber, bool pressed)
{
    // btnNumber starts at 1
    SetBtn(pressed, vJoyId, static_cast<UCHAR>(btnNumber));
}

void checkVJoyOwnership(UINT id) {
    VjdStat status = GetVJDStatus(id);

    switch (status) {
    case VJD_STAT_FREE:
        std::cout << "Device " << id << " is FREE.\n";
        break;

    case VJD_STAT_OWN:
        std::cout << "Device " << id << " is OWNED by this process.\n";
        break;

    case VJD_STAT_BUSY:
        std::cout << "Device " << id << " is BUSY (owned by another process).\n";
        break;

    case VJD_STAT_MISS:
        std::cout << "Device " << id << " is not configured in vJoyConf.\n";
        break;

    default:
        std::cout << "Unknown device status.\n";
        break;
    }
}

int main() {
    EnableVirtualTerminal();

    std::cout << "\033[36m===============================================\033[0m" << std::endl;
    std::cout << "\033[36m           SIMWHEEL PC SERVER v3.0            \033[0m" << std::endl;
    std::cout << "\033[36m===============================================\033[0m\n" << std::endl;

    if (!IsRunAsAdmin()) {
        // Warning message remains here
        std::cout << "\033[93m[WARNING] NOT RUNNING AS ADMINISTRATOR. MOUSE AND KEYBOARD FUNCTIONS WILL NOT WORK IN GAMES!!!\nIGNORE IF YOU ARE NOT USING THEM.\033[0m\n";
    }
    else {
        std::cout << ">> Running as Administrator.\n";
    }

    ShowLocalIP(); // Print local IP address for debugging


    // 1. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        pressEnterToExit();
        return 1;
    }

    // 2. Initialize vJoy
    //UINT vJoyId = 1;
    if (!vJoyEnabled()) {
        std::cout << "\n!!  vJoy Device " << vJoyId << " is not available.\n";
        std::cout << " To fix this:\n";
        //std::cout << "watch this on yt:\n";
        //std::cout << "OR:\n";
        std::cout << "1. Press the Windows key and search for \"Configure vJoy or vJoyConf\".\n";
        std::cout << "2. Open the vJoy Configuration Tool.\n";
        std::cout << "3. Check 'Enable vJoy' in bottom corner.\n";
        std::cout << "4. Select Device " << vJoyId << ".\n";
        std::cout << "5. Enable all axes (like X, Y) and set number of buttons or just set 32.\n";
        std::cout << "6. Turn off force feedback for better experience.\n";
        std::cout << "7. Click 'Apply', then close the tool and restart this app.\n\n";
        WSACleanup();
        pressEnterToExit();
        return 1;
    }
    VjdStat status = GetVJDStatus(vJoyId);
    if (status == VJD_STAT_OWN || status == VJD_STAT_FREE) {
        if (!AcquireVJD(vJoyId)) {
            std::cerr << "Failed to acquire vJoy device #" << vJoyId << "\n";
            checkVJoyOwnership(vJoyId);
            WSACleanup();
            pressEnterToExit();
            return 1;
        }
    }
    else {
        std::cerr << "vJoy device #" << vJoyId << " not available (status=" << status << ")\n";
        checkVJoyOwnership(vJoyId);
        WSACleanup();
        pressEnterToExit();
        return 1;
    }
    std::cout << "vJoy device #" << vJoyId << " acquired\n";

    // 3. Create UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // 4. Bind socket to port 4567
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(4567);
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }


    std::cout << "Listening for UDP on port 4567...\n";


    //user steering range

    double userRange = userSteering();
    g_Dash.userRange = userRange;
    std::cout << "\n";
    std::cout << "\033[96m>>> Application Ready. Waiting for app connection... <<<\033[0m\n\n";

    // 5. Receive loop
    const int bufSize = 512;
    char buffer[bufSize];
    sockaddr_in client;
    int clientLen = sizeof(client);

    JOYSTICK_POSITION iReport{};
    BYTE id = 1;
    iReport.bDevice = id;

    std::unordered_set<std::string> allowedIPs;
    std::unordered_set<std::string> blockedIPs;

    while (true) {
        int bytes = recvfrom(sock, buffer, bufSize - 1, 0,
            reinterpret_cast<sockaddr*>(&client), &clientLen);

        if (bytes == SOCKET_ERROR) {
            std::cerr << "recvfrom() error: " << WSAGetLastError() << "\n";
            break;
        }
        buffer[bytes] = '\0';
        std::string msg(buffer);

        char currentIPChars[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client.sin_addr), currentIPChars, INET_ADDRSTRLEN);
        std::string currentIPStr(currentIPChars);

        if (blockedIPs.find(currentIPStr) != blockedIPs.end()) {
            continue;
        }

        std::cout << "\r\033[K [log]Received" << "\033[38;5;208m -> " << msg << "\033[0m" << std::flush;

        try {
            auto j = json::parse(buffer);

            if (allowedIPs.find(currentIPStr) == allowedIPs.end()) {
                std::string phoneName = "Unknown Phone";
                if (j.contains("phoneName") && j["phoneName"].is_string()) {
                    phoneName = j["phoneName"].get<std::string>();
                }

                std::cout << "\n\033[93m[SECURITY] Connection attempt from " << phoneName << " (" << currentIPStr << ").\033[0m\n";
                std::cout << "\033[90m(Until you allow, this PC will not appear on your phone)\033[0m\n";
                std::cout << "\033[93mAllow this device to control your PC? (y/n): \033[0m";
                std::string answer;
                std::getline(std::cin, answer);
                if (answer == "y" || answer == "Y" || answer == "yes" || answer == "Yes") {
                    allowedIPs.insert(currentIPStr);
                    std::cout << "\033[92mDevice allowed.\033[0m\n";
                    g_Dash.initialized = false;
                }
                else {
                    std::cout << "\033[91mDevice blocked.\033[0m\n";
                    g_Dash.initialized = false;
                    blockedIPs.insert(currentIPStr);
                    UpdateDashboard();
                    continue;
                }
            }


            // Respond to discovery broadcast
            if (j.contains("type") && j["type"].is_string() && j["type"].get<std::string>() == "discover") {
                std::string phoneName = "Unknown Phone";
                if (j.contains("phoneName") && j["phoneName"].is_string()) {
                    phoneName = j["phoneName"].get<std::string>();
                }

                char computerName[MAX_COMPUTERNAME_LENGTH + 1];
                DWORD size = sizeof(computerName);
                if (!GetComputerNameA(computerName, &size)) {
                    strcpy_s(computerName, "Unknown PC");
                }

                json reply;
                reply["type"] = "discover_reply";
                reply["name"] = computerName;
                reply["connection"] = GetConnectionType(client);
                std::string replyStr = reply.dump();

                sendto(sock, replyStr.c_str(), replyStr.length(), 0, reinterpret_cast<sockaddr*>(&client), clientLen);

                g_Dash.deviceName = phoneName;
                g_Dash.connection = reply["connection"].get<std::string>();
                g_Dash.lastLog = "Handshake with " + phoneName + " OK.";
                DashLog("\033[92m[+] Found the phone [" + phoneName + "]! via " + g_Dash.connection + "\033[0m");
                continue;
            }

            if (j.contains("steering")) {
                double steering = j.at("steering").get<double>();  // e.g. 450.0
                double throttle = j.at("throttle").get<double>();  // 0.75
                double brake = j.at("brake").get<double>();     // 0.25

                g_Dash.steering = steering;
                g_Dash.throttle = throttle;
                g_Dash.brake = brake;
                if (g_Dash.deviceName == "Searching...") {
                    // Fallback if connected manually without discovery
                    g_Dash.deviceName = "Connected manually";
                    g_Dash.connection = GetConnectionType(client);
                }

                if (j.contains("clatch")) {
                    double clutch = j.at("clatch").get<double>();
                    g_Dash.hasClutch = true;
                    g_Dash.clutch = clutch;
                    SetAxis(MapToVJoyAxis(clutch * 2 - 1), vJoyId, HID_USAGE_RX);
                    j.erase("clatch");
                }

                j.erase("steering");
                j.erase("throttle");
                j.erase("brake");

                // 6. Feed to vJoy axes
                double normSteer = steering / userRange;
                LONG vJoyValue = clamp(MapToVJoyAxis(normSteer), static_cast<LONG>(0), static_cast<LONG>(32768));

                SetAxis(vJoyValue, vJoyId, HID_USAGE_X);
                SetAxis(MapToVJoyAxis(throttle * 2 - 1), vJoyId, HID_USAGE_Y);
                SetAxis(MapToVJoyAxis(brake * 2 - 1), vJoyId, HID_USAGE_Z);
            }

            /*   iReport.wAxisX = vJoyValue;
               iReport.wAxisY = MapToVJoyAxis(throttle * 2 - 1);
               iReport.wAxisZ = MapToVJoyAxis(brake * 2 - 1);*/


               /* if (!UpdateVJD(id, &iReport)) {
                    printf("Failed to update vJoy device\n");
                }*/

            if (j.contains("zaxis")) {
                double zaxis = j.at("zaxis").get<double>();
                g_Dash.hasZAxis = true;
                g_Dash.zaxis = zaxis;
                SetAxis(MapToVJoyAxis(zaxis * 2 - 1), vJoyId, HID_USAGE_RZ);
                j.erase("zaxis");
            }
            else {
                double dx = j.at("dx").get<double>(); // Mouse relative movement
                double dy = j.at("dy").get<double>();
                moveMouse((int)dx, (int)dy);
                j.erase("dx");
                j.erase("dy");
            }


            std::string activeButtons = "";

            if (j.contains("horn")) {
                SetBtn(j["horn"], vJoyId, static_cast<UCHAR>(1));
                if (j["horn"].get<bool>()) activeButtons += "HORN ";
                j.erase("horn");
            }

            try {
                for (auto it = j.begin(); it != j.end(); ) {
                    std::string keyStr = it.key();
                    int buttonId = std::stoi(keyStr);
                    bool status = it.value().get<bool>();

                    if (status) {
                        activeButtons += keyStr + " ";
                    }

                    if (buttonId >= 500)
                        MouseClick(buttonId, status);
                    else if (buttonId >= 200)
                        keyBoardEvents(buttonId, status);
                    else
                        SetBtn(status, vJoyId, static_cast<UCHAR>(buttonId));

                    it = j.erase(it);
                }
            }
            catch (...) {

            }

            if (!activeButtons.empty()) {
                g_Dash.lastLog = "Active inputs: [ " + activeButtons + "]";
            }
            else if (g_Dash.lastLog.find("Active inputs:") == 0) {
                g_Dash.lastLog = "Streaming data...";
            }

            UpdateDashboard();
        }
        catch (json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
        }

    }


    // 7. Cleanup
    RelinquishVJD(vJoyId);
    closesocket(sock);
    WSACleanup();
    return 0;
}
