/**
 * utils.cpp  –  Linux port
 *
 * MapToVJoyAxis logic is identical to the original.
 * pressEnterToExit() uses standard cin (same as original).
 * moveMouseRelative() (commented out in original) is left out entirely
 * on Linux – use xdotool or libinput if needed later.
 */

#include "Lib_Simwheel.h"

// Map a normalised [-1, 1] value to vJoy/uinput axis range [0, 32768]
LONG MapToVJoyAxis(double norm) {
    if (norm < -1.0) norm = -1.0;
    if (norm >  1.0) norm =  1.0;
    return static_cast<LONG>(16384 + norm * 16384);
}

void pressEnterToExit() {
    std::cout << "Press Enter to exit...";
    std::cin.ignore(10000, '\n');
    std::cin.get();
}
