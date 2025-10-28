/**
@file       settings.cpp
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Implementation of the Settings class

Stores and manages screen resolution settings
*/

#include "settings.h"
#include <iostream>
#include <fstream>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

/// Default screen width before loading from file
unsigned int Settings::screenWidth = 1280;

/// Default screen height before loading from file
unsigned int Settings::screenHeight = 720;

const float Settings::TARGET_ASPECT_RATIO = 16.0f / 9.0f;

struct SettingsLoader {
    SettingsLoader() {
        Settings::LoadFromFile("assets/settings.json");
        Settings::SetScreenSize(Settings::GetScreenWidth(), Settings::GetScreenHeight());
    }
};

static SettingsLoader SLoader;

/**
 * @brief Set the screen size
 * @param width New screen width
 * @param height New screen height
 */
void Settings::SetScreenSize(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;
}


/**
 * @brief Get the current screen width
 * @return Screen width
 */
unsigned int Settings::GetScreenWidth() {
    return screenWidth;
}

/**
 * @brief Get the current screen height
 * @return Screen height
 */
unsigned int Settings::GetScreenHeight() {
    return screenHeight;
}

/**
 * @brief Load settings from a file
 * @param path Path to the settings file
 *
 * For future implementation to load JSON configuration
 */

void Settings::LoadFromFile(const char* path) {
    //for future me to read from JSON
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "Could not open settings file " << path << std::endl;
        return;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document json;
    json.ParseStream(isw);

    if (json.HasParseError()) {
        std::cerr << "JSON parse error at " << path << std::endl;
        return;
    }

    if (json.HasMember("width") && json["width"].IsUint()) {
        screenWidth = json["width"].GetUint();
    }

    if (json.HasMember("height") && json["height"].IsUint()) {
        screenHeight = json["height"].GetUint();
    }
}

/**
 * @brief Save settings to a file
 * @param path Path to the settings file
 *
 * For future implementation to save JSON configuration
 */
void Settings::SaveToFile([[maybe_unused]] const char* path) {
    //for future me to write to JSON
}
