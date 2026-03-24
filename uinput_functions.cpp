/**
 * uinput_functions.cpp
 *
 * Linux port of vjoy_functions.cpp
 *
 * Windows vJoy API → Linux uinput virtual gamepad
 *
 * Axis mapping (identical value range [0, 32768]):
 *   HID_USAGE_X  (0x30)  →  ABS_X   (steering)
 *   HID_USAGE_Y  (0x31)  →  ABS_Y   (throttle)
 *   HID_USAGE_Z  (0x32)  →  ABS_Z   (brake)
 *   HID_USAGE_RZ (0x35)  →  ABS_RZ  (z-axis / clutch)
 *
 * Button mapping:
 *   btnNumber 1..32  →  BTN_TRIGGER + btnNumber - 1
 *
 * Prerequisites:
 *   sudo modprobe uinput          (load kernel module once)
 *   sudo chmod 0660 /dev/uinput   (or add yourself to the 'input' group)
 *   -- OR just run the program with sudo --
 */

#include "Lib_Simwheel.h"

// ─── Global uinput fd shared with main ───────────────────────────────────────
int g_uinput_fd = -1;

// ─── Helper: emit one uinput event ───────────────────────────────────────────
static void uinput_emit(int fd, int type, int code, int val)
{
    struct input_event ev{};
    ev.type  = static_cast<__u16>(type);
    ev.code  = static_cast<__u16>(code);
    ev.value = val;
    if (write(fd, &ev, sizeof(ev)) < 0) {
        std::cerr << "[uinput] write error: " << strerror(errno) << "\n";
    }
}

// ─── Helper: configure one ABS axis ──────────────────────────────────────────
static void uinput_set_abs(int fd, int axis, int min, int max, int fuzz = 0, int flat = 0)
{
    struct uinput_abs_setup abs{};
    abs.code              = static_cast<__u16>(axis);
    abs.absinfo.minimum   = min;
    abs.absinfo.maximum   = max;
    abs.absinfo.fuzz      = fuzz;
    abs.absinfo.flat      = flat;
    abs.absinfo.resolution = 0;
    if (ioctl(fd, UI_ABS_SETUP, &abs) < 0) {
        std::cerr << "[uinput] UI_ABS_SETUP failed for axis " << axis
                  << ": " << strerror(errno) << "\n";
    }
}

// ─── Create virtual gamepad (called once during startup) ─────────────────────
static int uinput_create_device()
{
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "[uinput] Cannot open /dev/uinput: " << strerror(errno) << "\n"
                      << "  → Load the kernel module:  sudo modprobe uinput\n"
                      << "  → Then re-run with:        sudo ./simwheel_receiver\n";
            return -1;
        }
    }

    // ── Enable event types ────────────────────────────────────────────────────
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    // ── Enable ABS axes ───────────────────────────────────────────────────────
    ioctl(fd, UI_SET_ABSBIT, ABS_X);    // steering
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);    // throttle
    ioctl(fd, UI_SET_ABSBIT, ABS_Z);    // brake
    ioctl(fd, UI_SET_ABSBIT, ABS_RZ);   // z-axis

    // ── Enable up to 32 buttons ───────────────────────────────────────────────
    for (int i = 0; i < 32; ++i)
        ioctl(fd, UI_SET_KEYBIT, BTN_TRIGGER + i);

    // ── Configure axis ranges (match vJoy [0, 32768]) ─────────────────────────
    uinput_set_abs(fd, ABS_X,  VJOY_AXIS_MIN, VJOY_AXIS_MAX, 0, 128); // steering: small deadzone
    uinput_set_abs(fd, ABS_Y,  VJOY_AXIS_MIN, VJOY_AXIS_MAX);
    uinput_set_abs(fd, ABS_Z,  VJOY_AXIS_MIN, VJOY_AXIS_MAX);
    uinput_set_abs(fd, ABS_RZ, VJOY_AXIS_MIN, VJOY_AXIS_MAX);

    // ── Device descriptor ─────────────────────────────────────────────────────
    struct uinput_setup usetup{};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1209; // generic open-source vendor
    usetup.id.product = 0x5377; // 'Sw' for SimWheel
    usetup.id.version = 1;
    strncpy(usetup.name, "SimWheel Virtual Controller", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
        std::cerr << "[uinput] UI_DEV_SETUP failed: " << strerror(errno) << "\n";
        close(fd);
        return -1;
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        std::cerr << "[uinput] UI_DEV_CREATE failed: " << strerror(errno) << "\n";
        close(fd);
        return -1;
    }

    std::cout << "[uinput] Virtual controller created: SimWheel Virtual Controller\n";
    return fd;
}

// ─── Public API (same names as original vjoy_functions.cpp) ──────────────────

/**
 * vjoyeorrs  – initialise uinput device.
 * Returns true on error (same semantics as Windows version).
 */
bool vjoyeorrs(UINT /*vJoyId*/)
{
    g_uinput_fd = uinput_create_device();
    if (g_uinput_fd < 0) {
        return true; // error
    }
    return false; // success
}

/**
 * RelinquishVJD – destroy the uinput device on exit.
 */
void RelinquishVJD(UINT /*vJoyId*/)
{
    if (g_uinput_fd >= 0) {
        ioctl(g_uinput_fd, UI_DEV_DESTROY);
        close(g_uinput_fd);
        g_uinput_fd = -1;
        std::cout << "[uinput] Virtual controller destroyed.\n";
    }
}

/**
 * SetAxis – send an absolute axis value to the virtual gamepad.
 *
 * hidUsage → Linux ABS code mapping:
 *   0x30 (HID_USAGE_X)  → ABS_X
 *   0x31 (HID_USAGE_Y)  → ABS_Y
 *   0x32 (HID_USAGE_Z)  → ABS_Z
 *   0x35 (HID_USAGE_RZ) → ABS_RZ
 */
void SetAxis(LONG value, UINT /*vJoyId*/, int hidUsage)
{
    if (g_uinput_fd < 0) return;

    int absCode;
    switch (hidUsage) {
        case HID_USAGE_X:  absCode = ABS_X;  break;
        case HID_USAGE_Y:  absCode = ABS_Y;  break;
        case HID_USAGE_Z:  absCode = ABS_Z;  break;
        case HID_USAGE_RZ: absCode = ABS_RZ; break;
        default:
            std::cerr << "[uinput] Unknown HID usage: 0x" << std::hex << hidUsage << "\n";
            return;
    }

    uinput_emit(g_uinput_fd, EV_ABS, absCode, static_cast<int>(value));
    uinput_emit(g_uinput_fd, EV_SYN, SYN_REPORT, 0);
}

/**
 * SetVJoyButton – press or release a virtual button.
 * btnNumber is 1-based (same as vJoy).
 */
void SetVJoyButton(UINT btnNumber, bool pressed, UINT /*vJoyId*/)
{
    if (g_uinput_fd < 0) return;
    if (btnNumber < 1 || btnNumber > 32) return;

    int keyCode = BTN_TRIGGER + static_cast<int>(btnNumber) - 1;
    uinput_emit(g_uinput_fd, EV_KEY, keyCode, pressed ? 1 : 0);
    uinput_emit(g_uinput_fd, EV_SYN, SYN_REPORT, 0);
}
