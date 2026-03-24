/**
 * core.cpp  –  Linux port
 *
 * Logic is identical to the original Windows version.
 * No Windows-specific code was present here; only the header include changes.
 */

#include "Lib_Simwheel.h"

using json = nlohmann::json;

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

void processInput(const std::string& buffer, int vJoyId, double userRange)
{
    try {
        auto j = json::parse(buffer);

        double steering = j.at("steering").get<double>();   // e.g. 450.0
        double throttle = j.at("throttle").get<double>();   // 0.0 – 1.0
        double brake    = j.at("brake").get<double>();       // 0.0 – 1.0
        double zaxis    = j.at("zaxis").get<double>();

        j.erase("steering");
        j.erase("throttle");
        j.erase("brake");
        j.erase("zaxis");

        // Map steering degrees → normalised [-1, 1] → vJoy axis [0, 32768]
        double normSteer = steering / userRange;
        LONG vJoyValue = clamp(MapToVJoyAxis(normSteer),
                               static_cast<LONG>(0),
                               static_cast<LONG>(32768));

        SetAxis(vJoyValue,                          static_cast<UINT>(vJoyId), HID_USAGE_X);
        SetAxis(MapToVJoyAxis(throttle * 2 - 1),    static_cast<UINT>(vJoyId), HID_USAGE_Y);
        SetAxis(MapToVJoyAxis(brake    * 2 - 1),    static_cast<UINT>(vJoyId), HID_USAGE_Z);
        SetAxis(MapToVJoyAxis(zaxis    * 2 - 1),    static_cast<UINT>(vJoyId), HID_USAGE_RZ);

        SetVJoyButton(1, j.value("horn", false), static_cast<UINT>(vJoyId));
        j.erase("horn");

        // Remaining keys are numeric button IDs
        for (auto it = j.begin(); it != j.end(); ++it) {
            try {
                int buttonId = std::stoi(it.key());
                bool value   = it.value().get<bool>();
                SetVJoyButton(static_cast<UINT>(buttonId), value,
                              static_cast<UINT>(vJoyId));
            } catch (...) {
                // Skip non-integer keys
            }
        }
    }
    catch (json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}
