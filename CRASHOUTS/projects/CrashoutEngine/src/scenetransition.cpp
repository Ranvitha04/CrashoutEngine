/*!
@file       scenetransition.cpp
@author     m.biniswandi@digipen.edu
@date       31/01/2026
@brief      Handles scene transition effects like fade in/out.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#include "scenetransition.h"
#include "shapes.h"
#include "rendering.h"
#include "engine.h" // for g_PendingSceneName
#include "CEColor.h"
#include "glapp.h" // for width/height
#include <iostream>

// Access global variables in engine.cpp
extern std::string g_PendingSceneName;
extern bool g_IsSceneLoadPending;

namespace CrashoutsEngine {

    std::string SceneTransition::s_nextScene = "";
    float SceneTransition::s_timer = 0.0f;

    float SceneTransition::s_durationOut = 1.0f; // Time to turn black
    float SceneTransition::s_durationIn = 1.0f;  // Time to reveal scene

    float SceneTransition::s_holdDuration = 0.25f;

    float SceneTransition::s_duration = 1.0f;
    bool SceneTransition::s_active = false;
    bool SceneTransition::s_fadingOut = false;
    bool SceneTransition::s_isHolding = false;

    float SceneTransition::s_alpha = 0.0f;

    // Starts fade without a target scene
    void SceneTransition::Start(const std::string& nextSceneName, float fadeOutTime, float fadeInTime) {
        if (s_active) return;
        s_nextScene = nextSceneName;

        // Safety check to prevent division by zero
        s_durationOut = fadeOutTime > 0.0f ? fadeOutTime : 0.01f;
        s_durationIn = fadeInTime > 0.0f ? fadeInTime : 0.01f;

        s_timer = 0.0f;
        s_active = true;
        s_fadingOut = true;
        s_isHolding = false;
        s_alpha = 0.0f;
        std::cout << "[SceneTransition] Fade to " << nextSceneName << " (Out: " << s_durationOut << "s, In: " << s_durationIn << "s)" << std::endl;
    }

    void SceneTransition::StartFade(float fadeOutTime, float fadeInTime) {
        if (s_active) return;

        s_nextScene = ""; // No load, just effect

        s_durationOut = fadeOutTime > 0.0f ? fadeOutTime : 0.01f;
        s_durationIn = fadeInTime > 0.0f ? fadeInTime : 0.01f;

        s_timer = 0.0f;
        s_active = true;
        s_fadingOut = true;
        s_alpha = 0.0f;
        std::cout << "[SceneTransition] Effect started." << std::endl;
    }

    void SceneTransition::Update(float dt) {
        if (!s_active) return;

        // --- STATE 1: FADING OUT (To Black) ---
        if (s_fadingOut) {
            s_timer += dt;
            float progress = s_timer / s_durationOut;
            if (progress > 1.0f) progress = 1.0f;

            // Use Linear instead of SmoothStep for alpha - usually feels more consistent
            s_alpha = progress;

            if (progress >= 1.0f) {
                // Done fading out. Switch to HOLD state.
                s_fadingOut = false;
                s_isHolding = true;
                s_timer = 0.0f; // Reset timer for the hold phase
                s_alpha = 1.0f;
            }
        }
        // --- STATE 2: HOLDING (Stay Black) ---
        else if (s_isHolding) {
            s_timer += dt;

            // Keep screen black
            s_alpha = 1.0f;

            // Wait until hold duration is over
            if (s_timer >= s_holdDuration) {
                s_isHolding = false;
                s_timer = 0.0f; // Reset timer for fade in

                // TRIGGER LOAD NOW (in the middle of the blackness)
                if (!s_nextScene.empty()) {
                    g_PendingSceneName = s_nextScene;
                    g_IsSceneLoadPending = true;
                    // Note: If your engine loads synchronously, the game will freeze here 
                    // on a black screen, which is perfect.
                }
            }
        }
        // --- STATE 3: FADING IN (To Transparent) ---
        else {
            s_timer += dt;
            float progress = s_timer / s_durationIn;
            if (progress > 1.0f) progress = 1.0f;

            s_alpha = 1.0f - progress; // Linear fade in

            if (progress >= 1.0f) {
                s_active = false;
                s_alpha = 0.0f;
            }
        }
    }

    void SceneTransition::Render() {
        if (!s_active && s_alpha <= 0.0f) return;

        using Component::Rendering;
        Rendering& renderer = Rendering::Get();
        renderer.shdr_pgm->Use();
        
        // Use Identity Projection/View for Fullscreen NDC draw
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", glm::mat4(1.0f));
        
        GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw Rectangle covering -1 to 1 (Size 2.0)
        // isWorldSpace=true prevents Shapes from applying its own View/Proj override logic,
        // allowing us to use the Identity ones we just set.
        Component::Shapes::CreateRectangle(
            CEVec2(2.0f, 2.0f), 
            CEVec3(0.0f, 0.0f, 0.0f), 
            0.0f, 
            CEColor(0.0f, 0.0f, 0.0f, s_alpha), 
            true 
        );
        
        if (depthEnabled) glEnable(GL_DEPTH_TEST);
    }
    
    bool SceneTransition::IsTransitioning() {
        return s_active;
    }
}

