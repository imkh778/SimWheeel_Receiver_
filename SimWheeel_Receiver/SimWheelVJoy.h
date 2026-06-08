#pragma once
#include <windows.h>
class SimWheelVJoy
{
	public:
	UINT vJoyId = 1;
	LONG MapToVJoyAxis(double norm);
	void SetVJoyButton(UINT btnNumber, bool pressed);
	int vjStatus();
	private:
		void checkVJoyOwnership();
};

