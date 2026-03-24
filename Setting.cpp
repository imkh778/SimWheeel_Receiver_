/**
 * Setting.cpp  –  Linux port
 *
 * Logic is identical to the original Windows version.
 * No Windows-specific code was present here.
 */

#include "Lib_Simwheel.h"

using json = nlohmann::json;

Settings getSettings() {
    const std::string filename = "settings.json";

    // Create default settings file if it doesn't exist
    std::ifstream check_file(filename);
    if (!check_file.is_open()) {
        std::cout << "File '" << filename << "' not found. Creating with default settings.\n";
        std::ofstream create_file(filename);
        create_file << R"({
  // IF YOU MESSED UP THE SETTINGS FILE, DELETE IT AND RUN THE PROGRAM AGAIN

  // IMPORTANT: If you change the steering range, you must also change it
  // in both the GAME and APP settings to match the new range,
  // otherwise the steering will not work properly.

  // Default steering range (minimum: 90, maximum: 2520). Set 0 to prompt on startup.
  "steering_range_default": 0,

  // Enable or disable logging of received control data
  "is_log": true
})";
        create_file.close();
    }

    try {
        std::ifstream settings_file(filename);
        if (!settings_file.is_open()) {
            std::cerr << "Error: Could not open settings.json for reading.\n";
            return { 0, true };
        }

        // nlohmann/json supports C-style // comments with allow_comments=true
        json settings_json = json::parse(settings_file, nullptr, true, true);

        double steering = settings_json.value("steering_range_default", 0.0);
        bool   log      = settings_json.value("is_log", true);

        return { steering, log };
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error in " << filename << ": " << e.what() << "\n"
                  << "  byte position: " << e.byte << "\n";
    }
    catch (json::exception& e) {
        std::cerr << "JSON exception: " << e.what() << "\n";
    }

    return { 900.0, true };
}

double userSteering() {
    double userRange = 900.0;
    std::string input;

    std::cout << "Enter steering range (min 90, max 2520), or press Enter for default (900): ";
    std::getline(std::cin, input);

    if (!input.empty()) {
        try {
            double tempRange = std::stod(input);
            if (tempRange >= 90.0 && tempRange <= 2520.0) {
                userRange = tempRange;
            } else {
                std::cerr << "Range out of bounds. Using default (900).\n";
            }
        } catch (...) {
            std::cerr << "Invalid input. Using default (900).\n";
        }
    }

    std::cout << "Using steering range: " << userRange << "\n";
    return userRange;
}
