/**
 * wifi.cpp  –  Linux port
 *
 * Original used Windows iphlpapi GetAdaptersAddresses.
 * Replaced with POSIX getifaddrs() which is available on all Linux distros.
 */

#include "Lib_Simwheel.h"

void ShowLocalIP()
{
    struct ifaddrs* ifaddr = nullptr;

    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "getifaddrs() failed: " << strerror(errno) << "\n";
        return;
    }

    std::cout << "Use this/those IP(s) in the SimWheel app to connect:\n";

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        // Only IPv4, skip loopback, skip interfaces that are down
        if (ifa->ifa_addr->sa_family != AF_INET)            continue;
        if (ifa->ifa_flags & IFF_LOOPBACK)                  continue;
        if (!(ifa->ifa_flags & IFF_UP))                     continue;

        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) == nullptr) continue;

        std::cout << "  Local IPv4 [" << ifa->ifa_name << "]: " << ip << "\n";
    }

    freeifaddrs(ifaddr);
}
