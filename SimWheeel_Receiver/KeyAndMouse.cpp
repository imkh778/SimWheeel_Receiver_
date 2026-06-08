#include "KeyAndMouse.h"
#include <iostream>

#pragma comment(lib, "user32.lib")

void KeyAndMouse::MouseClick(int button, bool isPressed) {
    INPUT in = {}; // Zero-initializes everything. dx, dy, time are now 0.
    in.type = INPUT_MOUSE;

    if (button == 500) {
        in.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }
    else if (button == 501) {
        in.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    }
    else if (button == 503) {
        in.mi.dwFlags = isPressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    }

    SendInput(1, &in, sizeof(INPUT));
}

WORD KeyAndMouse::customKeyToVK(int code) {

    if (code >= 200 && code <= 225) return WORD(0x41 + (code - 200));
    if (code >= 300 && code <= 309) return WORD(0x30 + (code - 300));
    if (code >= 400 && code <= 411) return WORD(VK_F1 + (code - 400));

    switch (code) {
        case 230: return VK_SPACE;
        case 231: return VK_RETURN;     // Enter
        case 232: return VK_BACK;       // Backspace
        case 233: return VK_TAB;        // Tab
        case 234: return VK_SHIFT;      // Shift
        case 235: return VK_CONTROL;    // Ctrl
        case 236: return VK_MENU;       // Alt
        case 237: return VK_LWIN;       // Left Windows key
        case 238: return VK_ESCAPE;     // ESC
        case 239: return VK_CAPITAL;    // Caps Lock

        case 250: return VK_OEM_MINUS;  // '-'
        case 251: return VK_OEM_PLUS;   // '='
        case 252: return VK_OEM_4;      // '['
        case 253: return VK_OEM_6;      // ']'
        case 254: return VK_OEM_5;      // '\'
        case 255: return VK_OEM_1;      // ';'
        case 256: return VK_OEM_7;      // '''
        case 257: return VK_OEM_COMMA;  // ','
        case 258: return VK_OEM_PERIOD; // '.'
        case 259: return VK_OEM_2;      // '/'

        case 350: return VK_LEFT;
        case 351: return VK_RIGHT;
        case 352: return VK_UP;
        case 353: return VK_DOWN;

        case 360: return VK_HOME;
        case 361: return VK_END;
        case 362: return VK_PRIOR;      // Page Up
        case 363: return VK_NEXT;       // Page Down

        case 370: return VK_DELETE;
        case 371: return VK_INSERT;

        default: return 0;
    }
}

void KeyAndMouse::keyBoardEvents(int code, bool isPressed) {
    WORD vk = customKeyToVK(code);
    if (!vk) return;


    // 1. Map the Virtual Key to a Hardware Scan Code
    // ETS2 reads this, not the wVk
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0; // Set to 0 because we are using wScan
    in.ki.wScan = (WORD)scanCode;
    in.ki.time = 0;
    in.ki.dwExtraInfo = 0;

    // 2. Set the Base Flag to indicate we are sending a Scan Code
    in.ki.dwFlags = KEYEVENTF_SCANCODE;

    // 3. Handle Extended Keys (Arrow keys, Insert, Delete, Home, End, etc.)
    // Without this, arrow keys might be interpreted as Numpad keys by the game.
    switch (vk) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: // PageUp, PageDown
    case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE:
    case VK_DIVIDE: case VK_NUMLOCK:
        in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        break;

    }

    // 4. Handle Key Release
    if (!isPressed) {
        in.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    SendInput(1, &in, sizeof(INPUT));
}

void KeyAndMouse::moveMouse(int dx, int dy) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    // MOUSEEVENTF_MOVE without MOUSEEVENTF_ABSOLUTE = Relative movement (Delta)
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    input.mi.dx = dx * 2; // Sensitivity multiplier
    input.mi.dy = dy * 2;

    // This function returns the number of events inserted. 
    // If it returns 0, it was blocked. 
    if (SendInput(1, &input, sizeof(INPUT)) == 0) {
        std::cerr << "\033[91mInput blocked! Run as Admin.\033[0m\n";
    }
}
