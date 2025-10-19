/*!
@file       settings.h
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Declaration of the Settings class

Stores global configuration values like screen width and height
*/

#pragma once

/*!
@class Settings
@brief Provides static methods for managing screen size and configuration
*/
class Settings {
public:
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

private:
    //TEMPORARY USE TO BE REMOVED AS IT DOESNT ADHERE TO ECS - Rain
    /// Current screen width
    static unsigned int screenWidth;

    /// Current screen height
    static unsigned int screenHeight;
};
