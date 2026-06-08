#pragma once
#include <string>
#include <winsock2.h>

class Networking
{
	public:
		static void ShowLocalIP();
		std::string GetConnectionType(const sockaddr_in& clientAddr);
	private:
		static std::string WStringToString(const std::wstring& wstr);
};

