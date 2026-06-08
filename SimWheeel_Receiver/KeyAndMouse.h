#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class KeyAndMouse
{
public:
	static void MouseClick(int button, bool isPressed);
	static void keyBoardEvents(int code, bool isPressed);
	void moveMouse(int dx, int dy);
private:
	static WORD customKeyToVK(int code);

};

