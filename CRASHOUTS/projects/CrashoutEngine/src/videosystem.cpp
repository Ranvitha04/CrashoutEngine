/*!
@file		videosystem.cpp
@author     m.biniswandi@digipen.edu
@date       01/02/2026
@brief		Definition of Video System to update Video Components.

All content ? 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "videosystem.h"
#include "videocomponent.h"
#include "videoloader.h"
#include "shapescomponent.h"
#include "transformcomponent.h"
#include "entitymanager.h"
#include "timebindings.h"
#include "glapp.h"
#include <iostream>

namespace CrashoutsEngine {

    /**
     * @brief Updates all VideoComponents in the ECS.
     * 
     * This method handles both FFmpeg-based video playback (via VideoLoader) 
     * and image sequence playback. It also synchronizes the video frame 
     * with the ShapeComponent's texture and optionally enforces aspect ratio 
     * on the TransformComponent.
     * 
     * @param dt Delta time since the last frame.
     */
    void VideoSystem::Update(float dt) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return;

        // Note: ECS doesn't expose component iterators directly easily unless we access the array.
        // We can use GetComponentArray<VideoComponent>().
        
        // Check if VideoComponent array exists (it might not if no video components created yet)
        // ECS::GetComponentArray creates it if missing.
        auto& videoArray = ecs->GetComponentArray<Component::VideoComponent>();
        
        for (size_t i = 0; i < videoArray.dense.size(); ++i) {
            Component::VideoComponent* vc = videoArray.dense[i];
            Entity entityID = videoArray.indexToEntity[i];

            // Failsafe: Try to initialize loader if it's missing but we have an MP4 path
            if (!vc->loader && !vc->videoPath.empty()) {
                vc->loader = std::make_shared<VideoLoader>(vc->videoPath);
                if (!vc->loader->Load()) {
                    vc->loader.reset();
                }
            }

            // Priority: FFmpeg Loader  Image Sequence
            if (vc->loader && vc->loader->IsValid()) {
                if (vc->isPlaying) {
                    vc->loader->Play();
                    vc->loader->SetLoop(vc->loop);
                    vc->loader->SetVolume(vc->volume);
                    vc->loader->Update(dt);
                } else {
                    vc->loader->Pause();
                }
                
                // Update Shape Texture
                if (ecs->HasComponent<Component::ShapeComponent>(entityID)) {
                    auto* sc = ecs->GetComponent<Component::ShapeComponent>(entityID);
                    sc->textureHandle = vc->loader->GetTextureHandle();
                    sc->enableTexture = true;
                    // Reset UVs to full for video
                    sc->uvSize = CEVec2(1.0f, 1.0f); 
                }

                // Enforce Aspect Ratio
                if (vc->matchAspectRatio && vc->loader->GetHeight() > 0) {
                    if (ecs->HasComponent<Component::TransformComponent>(entityID)) {
                        auto* tc = ecs->GetComponent<Component::TransformComponent>(entityID);
                        float videoRatio = (float)vc->loader->GetWidth() / (float)vc->loader->GetHeight();
                        // Maintain width, adjust height
                        if (abs(tc->size.y - (tc->size.x / videoRatio)) > 0.001f) {
                             tc->size.y = tc->size.x / videoRatio;
                        }
                    }
                }

                continue; // Skip legacy sequence update
            }

            if (!vc->isPlaying || vc->frames.empty()) continue;

            vc->timer += dt;
            if (vc->timer >= (1.0f / vc->frameRate)) {
                vc->timer = 0.0f;
                vc->currentFrameIndex++;

                if (vc->currentFrameIndex >= vc->frames.size()) {
                    if (vc->loop) {
                        vc->currentFrameIndex = 0;
                    } else {
                        vc->currentFrameIndex = (int)vc->frames.size() - 1;
                        vc->isPlaying = false;
                        // OnFinished callback?
                    }
                }
            }

            // Apply to ShapeComponent
            if (ecs->HasComponent<Component::ShapeComponent>(entityID)) {
                auto* sc = ecs->GetComponent<Component::ShapeComponent>(entityID);
                sc->textureHandle = vc->frames[vc->currentFrameIndex];
                sc->enableTexture = true;
            }
        }
    }
}
