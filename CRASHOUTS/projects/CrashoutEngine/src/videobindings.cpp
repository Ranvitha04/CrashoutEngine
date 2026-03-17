/*!
@file		videobindings.cpp
@author     Shahrin Idzuan
@date       31/01/2026
@brief		Exposes Video System functionality to C# scripts via Mono internal calls.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#include "videobindings.h"
#include "videocomponent.h"
#include "entitymanager.h"
#include "textureasset.h"
#include <mono/jit/jit.h>
#include <string>

#include "videoloader.h"
#include <memory>

namespace CrashoutsEngine {
    
    /**
     * @brief Internal call to add a VideoComponent to an entity.
     * @param id The ID of the entity.
     */
    static void InternalAddVideo(uint32_t id) {
        ECS::getInstance()->AddComponent<Component::VideoComponent>(id);
    }

    /**
     * @brief Internal call to check if an entity has a VideoComponent.
     * @param id The ID of the entity.
     * @return True if the entity has a VideoComponent, false otherwise.
     */
    static bool InternalHasVideo(uint32_t id) {
         return ECS::getInstance()->HasComponent<Component::VideoComponent>(id);
    }

    /**
     * @brief Internal call to remove a VideoComponent from an entity.
     * @param id The ID of the entity.
     */
    static void InternalRemoveVideo(uint32_t id) {
         ECS::getInstance()->RemoveComponent<Component::VideoComponent>(id);
    }

    /**
     * @brief Internal call to start playing the video on an entity.
     * @param id The ID of the entity.
     */
    static void InternalVideoPlay(uint32_t id) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(vc) vc->isPlaying = true;
    }

    /**
     * @brief Internal call to pause the video on an entity.
     * @param id The ID of the entity.
     */
    static void InternalVideoPause(uint32_t id) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(vc) vc->isPlaying = false;
    }

    /**
     * @brief Internal call to stop the video on an entity, resetting its state.
     * @param id The ID of the entity.
     */
    static void InternalVideoStop(uint32_t id) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(vc) {
             vc->isPlaying = false;
             vc->currentFrameIndex = 0;
             vc->timer = 0;
             if(vc->loader) vc->loader->Stop();
        }
    }

    /**
     * @brief Internal call to set whether the video on an entity should loop.
     * @param id The ID of the entity.
     * @param loop True to loop, false otherwise.
     */
    static void InternalVideoSetLoop(uint32_t id, bool loop) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(vc) vc->loop = loop;
    }

    /**
     * @brief Internal call to add a frame to the image sequence of a VideoComponent.
     * @param id The ID of the entity.
     * @param pathStr The path to the texture file for the frame.
     */
    static void InternalVideoAddFrame(uint32_t id, MonoString* pathStr) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(!vc) return;

        char* pathCStr = mono_string_to_utf8(pathStr);
        std::string path(pathCStr);
        mono_free(pathCStr);
        
        // Ensure texture is loaded
        Component::TextureRenderer::PreloadTexture(path);
        
        // Get handle. Note: PreloadTexture might register it under the path or asset name.
        // Assuming path acts as key.
        GLuint handle = Component::TextureRenderer::GetTextureHandle(path);
        
        // If handle is 0, Preload might have failed or the key is different.
        // Assuming success for now.
        if(handle != 0) {
            vc->frames.push_back(handle);
        }
    }
    
    /**
     * @brief Internal call to set the frame rate for the image sequence of a VideoComponent.
     * @param id The ID of the entity.
     * @param fps The frames per second.
     */
    static void InternalVideoSetFPS(uint32_t id, float fps) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(vc) vc->frameRate = fps;
    }
    
    /**
     * @brief Internal call to clear all frames from the image sequence of a VideoComponent.
     * @param id The ID of the entity.
     */
    static void InternalVideoClearFrames(uint32_t id) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if(vc) vc->frames.clear();
    }

    /**
     * @brief Internal call to load a video file into a VideoComponent.
     * @param id The ID of the entity.
     * @param pathStr The path to the video file.
     */
    static void InternalVideoLoad(uint32_t id, MonoString* pathStr) {
        auto* vc = ECS::getInstance()->GetComponent<Component::VideoComponent>(id);
        if (!vc) return;

        char* pathCStr = mono_string_to_utf8(pathStr);
        std::string path(pathCStr);
        mono_free(pathCStr);

        // Create Loader
        vc->loader = std::make_shared<VideoLoader>(path);
        if (!vc->loader->Load()) {
            // Fallback or error logging
            vc->loader.reset();
        }
    }

    /**
     * @brief Registers all internal video-related calls to Mono.
     */
    void Bindings::RegisterVideoBindings() {
        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddVideo", (const void*)InternalAddVideo);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveVideo", (const void*)InternalRemoveVideo);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasVideo", (const void*)InternalHasVideo);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_Play", (const void*)InternalVideoPlay);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_Pause", (const void*)InternalVideoPause);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_Stop", (const void*)InternalVideoStop);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_SetLoop", (const void*)InternalVideoSetLoop);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_AddFrame", (const void*)InternalVideoAddFrame);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_SetFPS", (const void*)InternalVideoSetFPS);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_ClearFrames", (const void*)InternalVideoClearFrames);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Video_Load", (const void*)InternalVideoLoad);
    }
}
