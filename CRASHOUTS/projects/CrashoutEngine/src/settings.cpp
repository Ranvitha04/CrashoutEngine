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
    json.SetObject();
    rapidjson::Document::AllocatorType& allocator = json.GetAllocator();

    json.AddMember("width", screenWidth, allocator);
    json.AddMember("height", screenHeight, allocator);

    json.AddMember("musicVolume", MusicVolume, allocator);
    json.AddMember("sfxVolume", SFXVolume, allocator);

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
    rapidjson::Document json;
    json.SetObject();
    rapidjson::Document::AllocatorType& allocator = json.GetAllocator();

    // --- 1. Boolean Toggles ---
    json.AddMember("lockLayout", uiState.lockLayout, allocator);
    json.AddMember("showPerf", uiState.showPerf, allocator);
    json.AddMember("showConsole", uiState.showConsole, allocator);
    json.AddMember("showInspector", uiState.showInspector, allocator);
    json.AddMember("showECS", uiState.showECS, allocator);
    json.AddMember("showAssets", uiState.showAssets, allocator);
    json.AddMember("showGameState", uiState.showGameState, allocator);
    json.AddMember("showGameScene", uiState.showGameScene, allocator);
    json.AddMember("showPhysics", uiState.showPhysics, allocator);
    json.AddMember("showAudio", uiState.showAudio, allocator);
    json.AddMember("drawCollOverlay", uiState.drawCollOverlay, allocator);
    json.AddMember("showEngineControl", uiState.showEngineControl, allocator);
    json.AddMember("drawQuadTree", uiState.drawQuadTree, allocator);
    json.AddMember("freezeOnCollision", uiState.freezeOnCollision, allocator);
    json.AddMember("showAnimation", uiState.showAnimation, allocator);
    json.AddMember("showSpriteSheet", uiState.showSpriteSheet, allocator);
    json.AddMember("drawAllColliders", uiState.drawAllColliders, allocator);
    json.AddMember("drawVelocities", uiState.drawVelocities, allocator);

    // --- 2. Floating Point Values ---
    json.AddMember("overlayThickness", uiState.overlayThickness, allocator);
    json.AddMember("fontScale", uiState.fontScale, allocator); 
    json.AddMember("fpsCap", FrameRateController::GetInstance().CAP(), allocator);

	// --- 3. Audio Volumes ---
    float musicVol = 1.0f;
    float sfxVol = 1.0f;
    auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
    if (audio.isInitialized()) {
        musicVol = audio.GetMusicVolume();
        sfxVol = audio.GetSFXVolume();
    }
    else {
        // Attempt to read previously saved values so we don't overwrite them with 0.
        std::string existingPath = GetGameRoot() + "/assets/settings.json";
        std::ifstream ifs(existingPath);
        if (ifs.is_open()) {
            rapidjson::IStreamWrapper isw(ifs);
            rapidjson::Document prev;
            prev.ParseStream(isw);
            if (!prev.HasParseError()) {
                if (prev.HasMember("musicVolume") && prev["musicVolume"].IsFloat())
                    musicVol = prev["musicVolume"].GetFloat();
                if (prev.HasMember("sfxVolume") && prev["sfxVolume"].IsFloat())
                    sfxVol = prev["sfxVolume"].GetFloat();
            }
        }
    }

    json.AddMember("musicVolume", musicVol, allocator);
    json.AddMember("sfxVolume", sfxVol, allocator);

    // 3. Save Colors
    // We create an array of arrays: [[r,g,b,a], [r,g,b,a], ...]
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

    json.AddMember("colors", colorArray, allocator);

    // 4. Write to File
    std::string fullPath = GetGameRoot() + "/assets/settings.json";
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
    LOAD_BOOL(showGameState);
    LOAD_BOOL(showGameScene);
    LOAD_BOOL(showPhysics);
    LOAD_BOOL(showAudio);
    LOAD_BOOL(drawCollOverlay);
    LOAD_BOOL(showEngineControl);
    LOAD_BOOL(drawQuadTree);
    LOAD_BOOL(freezeOnCollision);
    LOAD_BOOL(showAnimation);
    LOAD_BOOL(showSpriteSheet);
    LOAD_BOOL(drawAllColliders);
    LOAD_BOOL(drawVelocities);

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
