#include "SimWheelVJoy.h"
#include <iostream>
#include "vJoyInterface.h"

#pragma comment(lib, "vJoyInterface.lib")


// Map a normalized [-1,1] value to vJoy axis range [0,0x8000]
LONG SimWheelVJoy::MapToVJoyAxis(double norm) {
    if (norm < -1.0) norm = -1.0;
    if (norm > 1.0) norm = 1.0;
    return static_cast<LONG>(16384 + norm * 16384);
}

void SimWheelVJoy::SetVJoyButton(UINT btnNumber, bool pressed)
{
    // btnNumber starts at 1
    SetBtn(pressed, vJoyId, static_cast<UCHAR>(btnNumber));
}

void SimWheelVJoy::checkVJoyOwnership() {
    VjdStat status = GetVJDStatus(vJoyId);

    switch (status) {
    case VJD_STAT_FREE:
        std::cout << "Device " << vJoyId << " is FREE.\n";
        break;

    case VJD_STAT_OWN:
        std::cout << "Device " << vJoyId << " is OWNED by this process.\n";
        break;

    case VJD_STAT_BUSY:
        std::cout << "Device " << vJoyId << " is BUSY (owned by another process).\n";
        break;

    case VJD_STAT_MISS:
        std::cout << "Device " << vJoyId << " is not configured in vJoyConf.\n";
        break;

    default:
        std::cout << "Unknown device status.\n";
        break;
    }
}

int SimWheelVJoy::vjStatus() {
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

        return 1;
    }
    VjdStat status = GetVJDStatus(vJoyId);
    if (status == VJD_STAT_OWN || status == VJD_STAT_FREE) {
        if (!AcquireVJD(vJoyId)) {
            std::cerr << "Failed to acquire vJoy device #" << vJoyId << "\n";
            checkVJoyOwnership();
            return 1;
        }
    }
    else {
        std::cerr << "vJoy device #" << vJoyId << " not available (status=" << status << ")\n";
        checkVJoyOwnership();
        return 1;
    }
    std::cout << "vJoy device #" << vJoyId << " acquired\n";
	return 0;
}