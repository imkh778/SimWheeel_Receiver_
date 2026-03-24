# SimWheel Receiver – Linux Port

Linux port of [SimWheeel_Receiver](https://github.com/imkh778/SimWheeel_Receiver_) by imkh778.

The Android app connects over Wi-Fi UDP and sends steering/throttle/brake data as JSON.  
This receiver decodes it and forwards it to a virtual gamepad that Linux games can use.

---

## What changed from the Windows version

| Windows                        | Linux replacement                     |
|-------------------------------|---------------------------------------|
| `Winsock2` / `ws2tcpip`       | POSIX `sys/socket.h` / `netinet/in.h` |
| `iphlpapi` GetAdaptersAddresses | `getifaddrs()` from `ifaddrs.h`     |
| vJoy driver + `vJoyInterface.h` | Linux kernel **uinput** (`/dev/uinput`) |
| `SOCKET`, `LONG`, `UINT`, etc. | Standard C++ types + aliases         |
| `InetNtopW` (wide char)        | `inet_ntop` (standard narrow char)   |
| Visual Studio `.sln` / `.vcxproj` | **Makefile**                       |
| `vjoy_functions.cpp`           | `uinput_functions.cpp`               |

All game logic, JSON parsing, axis math, and button mapping are **unchanged**.

---

## Dependencies

```bash
# Ubuntu / Debian
sudo apt install build-essential nlohmann-json3-dev

# Fedora / RHEL
sudo dnf install gcc-c++ json-devel

# Arch Linux
sudo pacman -S base-devel nlohmann-json
```

The uinput kernel module (ships with all major distros, just needs loading):

```bash
sudo modprobe uinput
```

To avoid `sudo` every time, add yourself to the `input` group and re-login:

```bash
sudo usermod -aG input $USER
# then log out and back in
```

---

## Build

```bash
make
```

---

## Run

```bash
sudo ./simwheel_receiver      # or without sudo if you set up the input group
```

The program will:
1. Print your local IP address(es) — enter one of these in the SimWheel Android app
2. Ask for the steering range (or read it from `settings.json`)
3. Create a virtual gamepad visible as `/dev/input/eventX`
4. Listen on UDP port **4567** for data from the app

---

## Settings file

`settings.json` is created automatically on first run:

```json
{
  "steering_range_default": 0,   // 0 = prompt on startup; 90–2520 to hard-code
  "is_log": true                 // print each received packet to stdout
}
```

---

## Verify the virtual controller

```bash
# List input devices — look for "SimWheel Virtual Controller"
cat /proc/bus/input/devices

# Or with evtest (sudo apt install evtest)
sudo evtest
```

In a sim game, configure the new "SimWheel Virtual Controller" as your wheel just like any other joystick.

---

## Axis mapping

| Data field | uinput axis | Notes                          |
|-----------|-------------|-------------------------------|
| `steering` | `ABS_X`    | degrees → normalised → [0, 32768] |
| `throttle` | `ABS_Y`    | 0–1 → [0, 32768]               |
| `brake`    | `ABS_Z`    | 0–1 → [0, 32768]               |
| `zaxis`    | `ABS_RZ`   | 0–1 → [0, 32768]               |
| `horn`     | Button 1   |                                |
| `"N"` keys | Button N   | numeric string keys            |

---

## Troubleshooting

**`Cannot open /dev/uinput`**
```bash
sudo modprobe uinput
sudo chmod 0660 /dev/uinput   # temporary
# OR add yourself to the input group (permanent):
sudo usermod -aG input $USER
```

**nlohmann/json not found**
```bash
sudo apt install nlohmann-json3-dev
```

**Port 4567 already in use**
```bash
sudo ss -ulpn | grep 4567
```
