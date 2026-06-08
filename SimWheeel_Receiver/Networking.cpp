#include "Networking.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <iphlpapi.h>
#include <algorithm> // Add this include to use std::transform
// Ensure this include is added at the top of your file
#include <cctype> // For ::tolower
// Skip VirtualBox noise
std::string Networking::WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

void Networking::ShowLocalIP() {
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

std::string Networking::GetConnectionType(const sockaddr_in& clientAddr) {
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