/**
@file       settings.cpp
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Implementation of the Settings class

Stores and manages screen resolution settings
*/

#include "settings.h"
#include <iostream> 

//TEMPORARY USE TO BE REMOVED AS IT DOESNT ADHERE TO ECS - Rain
/// Default screen width before loading from file
unsigned int Settings::screenWidth = 1280;

/// Default screen height before loading from file
unsigned int Settings::screenHeight = 720;

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
}

/**
 * @brief Save settings to a file
 * @param path Path to the settings file
 *
 * For future implementation to save JSON configuration
 */
void Settings::SaveToFile(const char* path) {
    //for future me to write to JSON
}
