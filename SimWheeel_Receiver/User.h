#pragma once

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif  
class User
{
public:
	void EnableVirtualTerminal();
	bool IsRunAsAdmin();
	double userSteering();
	void pressEnterToExit();
};

