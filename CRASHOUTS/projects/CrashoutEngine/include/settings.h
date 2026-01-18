/*!
@file       settings.h
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Declaration of the Settings class

Stores global configuration values like screen width and height

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#pragma once
#include "debugmenu.h"

/*!
@class Settings
@brief Provides static methods for managing screen size and configuration
*/
class Settings {
public:
    static const float TARGET_ASPECT_RATIO;

    /*!
    @brief Set the screen size
    @param width Screen width
    @param height Screen height
    */
    static void SetScreenSize(unsigned int width, unsigned int height);

    /*!
    @brief Get the current screen width
    @return Screen width
    */
    static unsigned int GetScreenWidth();

    /*!
    @brief Get the current screen height
    @return Screen height
    */
    static unsigned int GetScreenHeight();

    /*!
    @brief Load settings from a file
    @param path Path to the settings file
    
    Placeholder for future JSON support
    */
    static void LoadFromFile(const char* path);

    /*!
    @brief Save settings to a file
    @param path Path to the settings file
    
    Placeholder for future JSON support
    */
    static void SaveToFile(const char* path);

    // --- EDITOR SETTINGS ---
    /*!
    @brief Saves Zoom, Colors, and Docking state
    @param uiState UIState to reference
    */
    static void SaveEditorSettings(const UI::UIState& uiState);
    /*!
    @brief Loads Zoom, Colors, and applies them to ImGui immediately
    @param uiState UIState to reference
    */
    static void LoadEditorSettings(UI::UIState& uiState);

private:
    //TEMPORARY USE TO BE REMOVED AS IT DOESNT ADHERE TO ECS - Rain
    /// Current screen width
    static unsigned int screenWidth;

    /// Current screen height
    static unsigned int screenHeight;

    // --- GAME SETTINGS ---
    static float MusicVolume;
    static float SFXVolume;

};
