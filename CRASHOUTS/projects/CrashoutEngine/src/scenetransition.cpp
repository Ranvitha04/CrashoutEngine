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
    float SceneTransition::s_duration = 1.0f;
    bool SceneTransition::s_active = false;
    bool SceneTransition::s_fadingOut = false;
    float SceneTransition::s_alpha = 0.0f;

    void SceneTransition::Start(const std::string& nextSceneName, float fadeDuration) {
        if (s_active) return; // Already running
        s_nextScene = nextSceneName;
        s_duration = fadeDuration;
        if (s_duration <= 0.0f) s_duration = 0.01f;
        s_timer = 0.0f;
        s_active = true;
        s_fadingOut = true;
        s_alpha = 0.0f;
        std::cout << "[SceneTransition] Starting fade to: " << nextSceneName << std::endl;
    }

    void SceneTransition::Update(float dt) {
        if (!s_active) return;

        s_timer += dt;
        float progress = s_timer / s_duration;
        if (progress > 1.0f) progress = 1.0f;

        // Apply SmoothStep easing: t * t * (3 - 2 * t)
        float smoothed = progress * progress * (3.0f - 2.0f * progress);

        if (s_fadingOut) {
            s_alpha = smoothed;
            if (progress >= 1.0f) {
                // Fade out complete -> Switch Scene
                s_fadingOut = false;
                s_timer = 0.0f;
                s_alpha = 1.0f; // Keep black during load
                
                // Trigger Load
                g_PendingSceneName = s_nextScene;
                g_IsSceneLoadPending = true;
            }
        } else {
            // Fade In (from black)
            s_alpha = 1.0f - smoothed;
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
