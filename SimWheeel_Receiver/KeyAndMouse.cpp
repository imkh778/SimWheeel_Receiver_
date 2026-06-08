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

    // -------------------------------
    // LETTERS (A–Z)
    // Custom: 200–225
    // VK_A = 0x41
    // -------------------------------
    if (code >= 200 && code <= 225) {
        return WORD(0x41 + (code - 200));  // 200→A, 201→B, etc.
    }

    // -------------------------------
    // NUMBERS 0–9
    // Custom: 300–309
    // VK_0 = 0x30, VK_1 = 0x31, etc
    // -------------------------------
    if (code >= 300 && code <= 309) {
        if (code == 300) return 0X30;
        return WORD(0x30 + (code - 300));  // 301→1, 302→2, etc.
    }

    // -------------------------------
    // FUNCTION KEYS F1–F12
    // Custom: 400–411
    // VK_F1 = 0x70
    // -------------------------------
    if (code >= 400 && code <= 411) {
        return WORD(VK_F1 + (code - 400));
    }

    // -------------------------------
    // SPECIAL KEYS
    // -------------------------------

    if (code == 230) return VK_SPACE;
    if (code == 231) return VK_RETURN;     // Enter
    if (code == 232) return VK_BACK;       // Backspace
    if (code == 233) return VK_TAB;        // Tab
    if (code == 234) return VK_SHIFT;      // Shift
    if (code == 235) return VK_CONTROL;    // Ctrl
    if (code == 236) return VK_MENU;       // Alt
    if (code == 237) return VK_LWIN;       // Left Windows key
    if (code == 238) return VK_ESCAPE;     // ESC
    if (code == 239) return VK_CAPITAL;    // Caps Lock

    // -------------------------------
    // SYMBOL KEYS
    // -------------------------------
    if (code == 250) return VK_OEM_MINUS;     // '-'
    if (code == 251) return VK_OEM_PLUS;      // '='
    if (code == 252) return VK_OEM_4;         // '['
    if (code == 253) return VK_OEM_6;         // ']'
    if (code == 254) return VK_OEM_5;         // '\'
    if (code == 255) return VK_OEM_1;         // ';'
    if (code == 256) return VK_OEM_7;         // '''
    if (code == 257) return VK_OEM_COMMA;     // ','
    if (code == 258) return VK_OEM_PERIOD;    // '.'
    if (code == 259) return VK_OEM_2;         // '/'

    // -------------------------------
    // ARROWS
    // Custom: 350–353
    // -------------------------------
    if (code == 350) return VK_LEFT;
    if (code == 351) return VK_RIGHT;
    if (code == 352) return VK_UP;
    if (code == 353) return VK_DOWN;

    // -------------------------------
    // PAGE KEYS
    // -------------------------------
    if (code == 360) return VK_HOME;
    if (code == 361) return VK_END;
    if (code == 362) return VK_PRIOR; // Page Up
    if (code == 363) return VK_NEXT;  // Page Down

    // -------------------------------
    // DELETE / INSERT
    // -------------------------------
    if (code == 370) return VK_DELETE;
    if (code == 371) return VK_INSERT;

    //Mouse
    //if (code == 372)return VK_LBUTTON;   // Left mouse button
    //if (code == 373)return VK_RBUTTON;  // Right mouse button
    //if (code == 374)return VK_MBUTTON; // Middle mouse button (wheel click)

    // Unknown key
    return 0;
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
