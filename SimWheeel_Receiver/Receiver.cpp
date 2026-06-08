
// pc_udp_receiver_with_vjoy.cpp
// UDP receiver with vJoy integration: listens on port 4567, parses controls,
// and feeds them into a vJoy virtual joystick.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include "vJoyInterface.h"
#include <nlohmann/json.hpp>
#include <unordered_set>
#include "KeyAndMouse.h"
#include "Networking.h"
#include "DashBoard.h"
#include "SimWheelVJoy.h"

//UINT vJoyId = 1;


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


int main() {
	KeyAndMouse km; // Instantiate to avoid linker issues with static methods
	Networking net;
    DashBoard dash;
    SimWheelVJoy vj;

    EnableVirtualTerminal();

    std::cout << "\033[36m===============================================\033[0m" << std::endl;
    std::cout << "\033[36m           SIMWHEEL PC SERVER v3.0            \033[0m" << std::endl;
    std::cout << "\033[36m===============================================\033[0m\n" << std::endl;

    if(vj.vjStatus()==1){
        WSACleanup();
        pressEnterToExit();
        return 1;
	}

    if (!IsRunAsAdmin()) {
        // Warning message remains here
        std::cout << "\033[93m[WARNING] NOT RUNNING AS ADMINISTRATOR. MOUSE AND KEYBOARD FUNCTIONS WILL NOT WORK IN GAMES!!!\nIGNORE IF YOU ARE NOT USING THEM.\033[0m\n";
    }
    else {
        std::cout << ">> Running as Administrator.\n";
    }

    net.ShowLocalIP(); // Print local IP address for debugging


    // 1. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        pressEnterToExit();
        return 1;
    }

    // 2. Initialize vJoy
    //UINT vJoyId = 1;
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
    dash.g_Dash.userRange = userRange;
    std::cout << "\n";
    std::cout << "\033[96m>>> Application Ready. Waiting for app connection... <<<\033[0m\n\n";

    // 5. Receive loop
    const int bufSize = 512;
    char buffer[bufSize];
    sockaddr_in client;
    int clientLen = sizeof(client);

  /*  JOYSTICK_POSITION iReport{};
    BYTE id = 1;
    iReport.bDevice = id;*/

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
                    dash.g_Dash.initialized = false;
                }
                else {
                    std::cout << "\033[91mDevice blocked.\033[0m\n";
                    dash.g_Dash.initialized = false;
                    blockedIPs.insert(currentIPStr);
                    dash.UpdateDashboard();
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
                reply["connection"] = net.GetConnectionType(client);
                std::string replyStr = reply.dump();

                sendto(sock, replyStr.c_str(), static_cast<int>(replyStr.length()), 0, reinterpret_cast<sockaddr*>(&client), clientLen);

                dash.g_Dash.deviceName = phoneName;
                dash.g_Dash.connection = reply["connection"].get<std::string>();
                dash.g_Dash.lastLog = "Handshake with " + phoneName + " OK.";
                //DashLog("\033[92m[+] Found the phone [" + phoneName + "]! via " + dash.g_Dash.connection + "\033[0m");
                continue;
            }

            if (j.contains("steering")) {
                double steering = j.at("steering").get<double>();  // e.g. 450.0
                double throttle = j.at("throttle").get<double>();  // 0.75
                double brake = j.at("brake").get<double>();     // 0.25

                dash.g_Dash.steering = steering;
                dash.g_Dash.throttle = throttle;
                dash.g_Dash.brake = brake;
                if (dash.g_Dash.deviceName == "Searching...") {
                    // Fallback if connected manually without discovery
                    dash.g_Dash.deviceName = "Connected manually";
                    dash.g_Dash.connection = net.GetConnectionType(client);
                }

                if (j.contains("clutch ")) {
                    double clutch = j.at("clutch ").get<double>();
                    dash.g_Dash.hasClutch = true;
                    dash.g_Dash.clutch = clutch;
                    SetAxis(vj.MapToVJoyAxis(clutch * 2 - 1), vj.vJoyId, HID_USAGE_RX);
                    j.erase("clutch ");
                }

                j.erase("steering");
                j.erase("throttle");
                j.erase("brake");

                // 6. Feed to vJoy axes
                double normSteer = steering / userRange;
                LONG vJoyValue = clamp(vj.MapToVJoyAxis(normSteer), static_cast<LONG>(0), static_cast<LONG>(32768));

                SetAxis(vJoyValue, vj.vJoyId, HID_USAGE_X);
                SetAxis(vj.MapToVJoyAxis(throttle * 2 - 1), vj.vJoyId, HID_USAGE_Y);
                SetAxis(vj.MapToVJoyAxis(brake * 2 - 1), vj.vJoyId, HID_USAGE_Z);
            }

            /*   iReport.wAxisX = vJoyValue;
               iReport.wAxisY = MapToVJoyAxis(throttle * 2 - 1);
               iReport.wAxisZ = MapToVJoyAxis(brake * 2 - 1);*/


               /* if (!UpdateVJD(id, &iReport)) {
                    printf("Failed to update vJoy device\n");
                }*/

            if (j.contains("zaxis")) {
                double zaxis = j.at("zaxis").get<double>();
                dash.g_Dash.hasZAxis = true;
                dash.g_Dash.zaxis = zaxis;
                SetAxis(vj.MapToVJoyAxis(zaxis * 2 - 1), vj.vJoyId, HID_USAGE_RZ);
                j.erase("zaxis");
            }
            else {
                double dx = j.at("dx").get<double>(); // Mouse relative movement
                double dy = j.at("dy").get<double>();
                km.moveMouse((int)dx, (int)dy);
                j.erase("dx");
                j.erase("dy");
            }


            std::string activeButtons = "";

            if (j.contains("horn")) {
                SetBtn(j["horn"], vj.vJoyId, static_cast<UCHAR>(1));
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
                        km.MouseClick(buttonId, status);
                    else if (buttonId >= 200)
                        km.keyBoardEvents(buttonId, status);
                    else
                        SetBtn(status, vj.vJoyId, static_cast<UCHAR>(buttonId));

                    it = j.erase(it);
                }
            }
            catch (...) {

            }

            if (!activeButtons.empty()) {
                dash.g_Dash.lastLog = "Active inputs: [ " + activeButtons + "]";
            }
            else if (dash.g_Dash.lastLog.find("Active inputs:") == 0) {
                dash.g_Dash.lastLog = "Streaming data...";
            }

           dash.UpdateDashboard();
        }
        catch (json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
        }

    }


    // 7. Cleanup
    RelinquishVJD(vj.vJoyId);
    closesocket(sock);
    WSACleanup();
    return 0;
}
