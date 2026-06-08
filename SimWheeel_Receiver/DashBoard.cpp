#include "DashBoard.h"
#include <iostream>


// Console Dashboard State

std::string DashBoard::DrawBar(double value, double minVal, double maxVal, int width) {
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;
    double range = maxVal - minVal;
    double pct = (range == 0) ? 0 : (value - minVal) / range;

    int fillCount = static_cast<int>(pct * width);
    std::string bar = "[";
    for (int i = 0; i < width; i++) {
        if (i < fillCount) bar += "=";
        else if (i == fillCount) bar += "|";
        else bar += " ";
    }
    bar += "]";

    char buf[64];
    if (minVal < 0) {
        snprintf(buf, sizeof(buf), " (%5.1f deg)", value);
    }
    else {
        snprintf(buf, sizeof(buf), " (%3.0f%%)", pct * 100);
    }
    return bar + buf;
}

void DashBoard::UpdateDashboard() {
    int lines = 7;
    if (g_Dash.hasClutch) lines++;
    if (g_Dash.hasZAxis) lines++;

    // Re-reserve space if number of lines increases
    static int lastLinesCount = lines;
    if (lines > lastLinesCount) {
        g_Dash.initialized = false;
        lastLinesCount = lines;
    }

    if (!g_Dash.initialized) {
        for (int i = 0; i < lines; i++) std::cout << "\n";
        g_Dash.initialized = true;
    }

    std::cout << "\033[" << lines << "A";
    std::cout << "\033[K" << "\033[36m>> STATUS: \033[0m" << g_Dash.lastLog << "\n";
    std::cout << "\033[K" << "------------------------------------------------\n";
    std::cout << "\033[K" << " Connection : " << g_Dash.deviceName << " [" << g_Dash.connection << "]\n";

    std::cout << "\033[K" << " Steering   : " << DrawBar(g_Dash.steering, -g_Dash.userRange, g_Dash.userRange, 20) << "\n";
    std::cout << "\033[K" << " Throttle   : " << DrawBar(g_Dash.throttle, 0.0, 1.0, 20) << "\n";
    std::cout << "\033[K" << " Brake      : " << DrawBar(g_Dash.brake, 0.0, 1.0, 20) << "\n";

    if (g_Dash.hasClutch) {
        std::cout << "\033[K" << " Clutch     : " << DrawBar(g_Dash.clutch, 0.0, 1.0, 20) << "\n";
    }
    if (g_Dash.hasZAxis) {
        std::cout << "\033[K" << " Z-Axis     : " << DrawBar(g_Dash.zaxis, 0.0, 1.0, 20) << "\n";
    }

    std::cout << "\033[K" << "------------------------------------------------\n";
    std::flush(std::cout);
}

//void DashBoard::DashLog(const std::string& msg) {
//    if (g_Dash.initialized) {
//        int lines = 7;
//        if (g_Dash.hasClutch) lines++;
//        if (g_Dash.hasZAxis) lines++;
//        std::cout << "\033[" << lines << "A"; // move cursor to top of dashboard
//        std::cout << "\033[J";  // clear from cursor to end of screen
//        std::cout << msg << "\n";
//        g_Dash.initialized = false;
//        UpdateDashboard();
//    }
//    else {
//        std::cout << msg << "\n";
//    }
//}