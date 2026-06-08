#pragma once
#include <string>
#include <winsock2.h>
#include <unordered_map>

class Networking
{
	public:
		static void ShowLocalIP();
		std::string GetConnectionType(const sockaddr_in& clientAddr);
	private:
		static std::string WStringToString(const std::wstring& wstr);
		std::unordered_map<uint32_t, std::string> connectionCache;
};

