#pragma once
#include <string>

class DashBoard
{
public:
    struct DashboardState {
        std::string deviceName = "Searching...";
        std::string connection = "none";
        double steering = 0.0;
        double throttle = 0.0;
        double brake = 0.0;

        bool hasClutch = false;
        double clutch = 0.0;

        bool hasZAxis = false;
        double zaxis = 0.0;

        std::string lastLog = "Waiting for data...";
        bool initialized = false;
        double userRange = 900.0;
    } g_Dash;

	std::string DrawBar(double value, double minVal, double maxVal, int width);
	void UpdateDashboard(); 

};

