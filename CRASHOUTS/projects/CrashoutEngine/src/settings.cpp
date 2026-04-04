/**
@file       settings.cpp
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Implementation of the Settings class

Stores and manages screen resolution settings

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "settings.h"
#include <iostream>
#include <fstream>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"

#include "imgui.h" 
#include "assetmanager.h"
#include "frameratecontroller.h"
#include "audioSystem.h"
#include "engine.h"

/// Default screen width before loading from file
unsigned int Settings::screenWidth = 1280;

/// Default screen height before loading from file
unsigned int Settings::screenHeight = 720;

float Settings::MusicVolume = 1.0f;
float Settings::SFXVolume = 1.0f;

const float Settings::TARGET_ASPECT_RATIO = 16.0f / 9.0f;

/** @brief SettingsLoader */
struct SettingsLoader {
    SettingsLoader() {
        // We no longer load here because GetGameRoot() is empty at static-init time.
        // Initialization will happen manually in engine.cpp after SetGameRoot().
    }
};

static SettingsLoader SLoader;

void Settings::Load() {
    std::string path = GetGameRoot() + "/assets/settings.json";
    LoadFromFile(path.c_str());
    SetScreenSize(GetScreenWidth(), GetScreenHeight());
}

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

    if (json.HasMember("musicVolume") && json["musicVolume"].IsFloat()) {
        float vol = json["musicVolume"].GetFloat();
        Settings::MusicVolume = vol;
        if (CrashoutsEngine::Audio::AudioSystem::instance().isInitialized()) {
            CrashoutsEngine::Audio::AudioSystem::instance().setMusicGroupVolume(vol);
        }
    }

    if (json.HasMember("sfxVolume") && json["sfxVolume"].IsFloat()) {
        float vol = json["sfxVolume"].GetFloat();
        Settings::SFXVolume = vol;
        if (CrashoutsEngine::Audio::AudioSystem::instance().isInitialized()) {
            CrashoutsEngine::Audio::AudioSystem::instance().setSFXGroupVolume(vol);
        }
    }
}

/**
 * @brief Save settings to a file
 * @param path Path to the settings file
 */
void Settings::SaveToFile(const char* path) {
    rapidjson::Document json;
    
    // Read existing file first to preserve other settings
    std::ifstream ifs(path);
    if (ifs.is_open()) {
        rapidjson::IStreamWrapper isw(ifs);
        json.ParseStream(isw);
        ifs.close();
    }
    
    // Ensure it's an object in case the file was missing or invalid
    if (!json.IsObject()) {
        json.SetObject();
    }

    rapidjson::Document::AllocatorType& allocator = json.GetAllocator();

    // Helper macro to update or add a member
    #define UPDATE_OR_ADD(key, val) \
        if (json.HasMember(key)) json[key] = val; \
        else json.AddMember(key, val, allocator)

    UPDATE_OR_ADD("width", screenWidth);
    UPDATE_OR_ADD("height", screenHeight);
    UPDATE_OR_ADD("musicVolume", MusicVolume);
    UPDATE_OR_ADD("sfxVolume", SFXVolume);

    #undef UPDATE_OR_ADD

    std::ofstream ofs(path);
    if (!ofs.is_open()) return;

    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);
}

void Settings::Save() {
    std::string path = GetGameRoot() + "/assets/settings.json";
    SaveToFile(path.c_str());
}

void Settings::SaveEditorSettings(const UI::UIState& uiState) {
    std::string fullPath = GetGameRoot() + "/assets/settings.json";
    rapidjson::Document json;

    // Read existing file first to preserve other settings (like width and height)
    std::ifstream ifs(fullPath);
    if (ifs.is_open()) {
        rapidjson::IStreamWrapper isw(ifs);
        json.ParseStream(isw);
        ifs.close();
    }
    
    if (!json.IsObject()) {
        json.SetObject();
    }

    rapidjson::Document::AllocatorType& allocator = json.GetAllocator();

    // Helper macro to update or add a member
    #define UPDATE_OR_ADD(key, val) \
        if (json.HasMember(key)) { json[key] = val; } \
        else { json.AddMember(rapidjson::StringRef(key), val, allocator); }

    // --- 1. Boolean Toggles ---
    UPDATE_OR_ADD("lockLayout", uiState.lockLayout);
    UPDATE_OR_ADD("showPerf", uiState.showPerf);
    UPDATE_OR_ADD("showConsole", uiState.showConsole);
    UPDATE_OR_ADD("showInspector", uiState.showInspector);
    UPDATE_OR_ADD("showECS", uiState.showECS);
    UPDATE_OR_ADD("showAssets", uiState.showAssets);
    UPDATE_OR_ADD("showGameScene", uiState.showGameScene);
    UPDATE_OR_ADD("showPhysics", uiState.showPhysics);
    UPDATE_OR_ADD("showAudio", uiState.showAudio);
    UPDATE_OR_ADD("showBuildPanel", uiState.showBuildPanel);
    UPDATE_OR_ADD("drawCollOverlay", uiState.drawCollOverlay);
    UPDATE_OR_ADD("showEngineControl", uiState.showEngineControl);
    UPDATE_OR_ADD("drawQuadTree", uiState.drawQuadTree);
    UPDATE_OR_ADD("freezeOnCollision", uiState.freezeOnCollision);
    UPDATE_OR_ADD("showAnimation", uiState.showAnimation);
    UPDATE_OR_ADD("showSpriteSheet", uiState.showSpriteSheet);
    UPDATE_OR_ADD("drawAllColliders", uiState.drawAllColliders);
    UPDATE_OR_ADD("drawVelocities", uiState.drawVelocities);
    UPDATE_OR_ADD("showProjectManager", uiState.showProjectManager);
    UPDATE_OR_ADD("showVisualScripting", uiState.showVisualScripting);

    // --- 2. Floating Point & Int Values ---
    UPDATE_OR_ADD("overlayThickness", uiState.overlayThickness);
    UPDATE_OR_ADD("fontScale", uiState.fontScale); 
    UPDATE_OR_ADD("fpsCap", FrameRateController::GetInstance().CAP());

	// --- 3. Audio Volumes ---
    float musicVol = 1.0f;
    float sfxVol = 1.0f;
    auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
    if (audio.isInitialized()) {
        musicVol = audio.GetMusicVolume();
        sfxVol = audio.GetSFXVolume();
    }
    else {
        // We already loaded the previous json, so if they exist, use them
        if (json.HasMember("musicVolume") && json["musicVolume"].IsFloat()) {
            musicVol = json["musicVolume"].GetFloat();
        }
        if (json.HasMember("sfxVolume") && json["sfxVolume"].IsFloat()) {
            sfxVol = json["sfxVolume"].GetFloat();
        }
    }

    UPDATE_OR_ADD("musicVolume", musicVol);
    UPDATE_OR_ADD("sfxVolume", sfxVol);

    // 4. Save Colors
    rapidjson::Value colorArray(rapidjson::kArrayType);
    ImGuiStyle& style = ImGui::GetStyle();

    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        ImVec4& col = style.Colors[i];
        rapidjson::Value rgba(rapidjson::kArrayType);
        rgba.PushBack(col.x, allocator);
        rgba.PushBack(col.y, allocator);
        rgba.PushBack(col.z, allocator);
        rgba.PushBack(col.w, allocator);
        colorArray.PushBack(rgba, allocator);
    }

    if (json.HasMember("colors")) {
        json["colors"] = colorArray;
    } else {
        json.AddMember("colors", colorArray, allocator);
    }

    #undef UPDATE_OR_ADD

    // 5. Write to File
    std::ofstream ofs(fullPath);
    if (!ofs.is_open()) {
        std::cerr << "Failed to save editor settings to: " << fullPath << std::endl;
        return;
    }

    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    json.Accept(writer);

    std::cout << "Editor settings saved to: " << fullPath << std::endl;
}

void Settings::LoadEditorSettings(UI::UIState& uiState) {
    std::string fullPath = GetGameRoot() + "/assets/settings.json";
    std::ifstream ifs(fullPath);

    // If no settings file exists, defaults (set in struct) are used
    if (!ifs.is_open()) return;

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document json;
    json.ParseStream(isw);

    if (json.HasParseError()) return;

    // --- 1. Helper Macro for Booleans ---
    // Checks if key exists and is bool before assigning
#define LOAD_BOOL(name) if (json.HasMember(#name) && json[#name].IsBool()) uiState.name = json[#name].GetBool()

    LOAD_BOOL(lockLayout);
    LOAD_BOOL(showPerf);
    LOAD_BOOL(showConsole);
    LOAD_BOOL(showInspector);
    LOAD_BOOL(showECS);
    LOAD_BOOL(showAssets);
    LOAD_BOOL(showGameScene);
    LOAD_BOOL(showPhysics);
    LOAD_BOOL(showAudio);
    LOAD_BOOL(showBuildPanel);
    LOAD_BOOL(drawCollOverlay);
    LOAD_BOOL(showEngineControl);
    LOAD_BOOL(drawQuadTree);
    LOAD_BOOL(freezeOnCollision);
    LOAD_BOOL(showAnimation);
    LOAD_BOOL(showSpriteSheet);
    LOAD_BOOL(drawAllColliders);
    LOAD_BOOL(drawVelocities);
    LOAD_BOOL(showProjectManager);
    LOAD_BOOL(showVisualScripting);

#undef LOAD_BOOL

    // --- 2. Load Floats ---
    if (json.HasMember("overlayThickness") && json["overlayThickness"].IsFloat()) {
        uiState.overlayThickness = json["overlayThickness"].GetFloat();
    }

    if (json.HasMember("fontScale") && json["fontScale"].IsFloat()) {
        uiState.fontScale = json["fontScale"].GetFloat();
        // Apply zoom immediately
        ImGui::GetIO().FontGlobalScale = uiState.fontScale;
    }

    if (json.HasMember("fpsCap") && json["fpsCap"].IsInt()) {
        FrameRateController::GetInstance().SetTargetFPS(json["fpsCap"].GetInt());
    }

    // In case we use Master Volume later lul
    /*if (json.HasMember("masterVolume") && json["masterVolume"].IsFloat()) {
        float vol = json["masterVolume"].GetFloat();
        CrashoutsEngine::Audio::AudioSystem::instance().setMasterVolume(vol);
	}*/  

    if (json.HasMember("musicVolume") && json["musicVolume"].IsFloat()) {
        float vol = json["musicVolume"].GetFloat();
        Settings::MusicVolume = vol;
        CrashoutsEngine::Audio::AudioSystem::instance().setMusicGroupVolume(vol);
    }

    if (json.HasMember("sfxVolume") && json["sfxVolume"].IsFloat()) {
        float vol = json["sfxVolume"].GetFloat();
        Settings::SFXVolume = vol;
        CrashoutsEngine::Audio::AudioSystem::instance().setSFXGroupVolume(vol);
    }

    // --- 3. Load Colors ---
    if (json.HasMember("colors") && json["colors"].IsArray()) {
        const rapidjson::Value& colorArray = json["colors"];
        ImGuiStyle& style = ImGui::GetStyle();

        for (int i = 0; i < static_cast<int>(colorArray.Size()) && i < ImGuiCol_COUNT; ++i) {
            const rapidjson::Value& rgba = colorArray[i];
            if (rgba.IsArray() && rgba.Size() == 4) {
                style.Colors[i] = ImVec4(
                    rgba[0].GetFloat(),
                    rgba[1].GetFloat(),
                    rgba[2].GetFloat(),
                    rgba[3].GetFloat()
                );
            }
        }
    }
}
