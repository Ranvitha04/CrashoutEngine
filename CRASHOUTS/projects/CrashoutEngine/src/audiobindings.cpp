/*!
@file		audiobindings.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       19/11/2025
@brief		Definition of audio bindings functions

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/
//__________________________________________________________________________*/

#include "audiobindings.h"
#include "audioSystem.h"
#include "audiocomponent.h"
#include "entitymanager.h"
#include "activecomponent.h"
#include "transformcomponent.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <fmod_common.h> // For FMOD_DSP_TYPE

using namespace CrashoutsEngine;
using namespace Component;
using CrashoutsEngine::Audio::AudioSystem;

namespace
{
    // Helpers: convert MonoString* to std::string safely
    static std::string MonoToStdString(MonoString* s)
    {
        if (!s) return {};
        char* cstr = mono_string_to_utf8(s);
        std::string out = cstr ? cstr : "";
        mono_free(cstr);
        return out;
    }

    // --------- Internal call implementations ----------

    static void ICALL_Audio_PlayMusicFileTagged(MonoString* key, MonoString* path, mono_bool loop)
    {
        auto& audio = AudioSystem::instance();
        audio.playMusicFileTagged(
            MonoToStdString(key),
            MonoToStdString(path),
            loop != 0
        );
    }

    static void ICALL_Audio_PlaySFXFileTagged(MonoString* key, MonoString* path, mono_bool loop)
    {
        auto& audio = AudioSystem::instance();
        audio.playSFXFileTagged(
            MonoToStdString(key),
            MonoToStdString(path),
            loop != 0
        );
    }

    static void ICALL_Audio_StopTagged(MonoString* key, float fadeDuration)
    {
        auto& audio = AudioSystem::instance();
        audio.stopTagged(MonoToStdString(key), fadeDuration);
    }

    static void ICALL_Audio_StopMusic()
    {
        auto& audio = AudioSystem::instance();
        audio.stopMusicGroup();
    }

    static void ICALL_Audio_StopComponent(uint32_t entityID, float fadeDuration)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();

        for (auto& clip : ac->clips) {
            if (clip.inst) {
                clip.inst->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                clip.inst->release();
                clip.inst = nullptr;
            }
            if (clip.channel) {
                audio.stopChannel(clip.channel, fadeDuration);
                clip.channel = nullptr;
            }
        }
    }

    static void ICALL_Audio_SetMusicVolume(float volume)
    {
        auto& audio = AudioSystem::instance();
        audio.setMusicGroupVolume(volume);
    }

    static void ICALL_Audio_SetSFXVolume(float volume)
    {
        auto& audio = AudioSystem::instance();
        audio.setSFXGroupVolume(volume);
    }

    static void ICALL_Audio_SetTaggedVolume(MonoString* key, float vol)
    {
        auto& audio = AudioSystem::instance();
        audio.setTaggedVolume(MonoToStdString(key), vol);
    }

    static void ICALL_Audio_SetMasterVolume(float volume)
    {
        auto& audio = AudioSystem::instance();
        audio.setMasterVolume(volume);
    }

    static float ICALL_Audio_GetMusicVolume() {
        return AudioSystem::instance().GetMusicVolume();
    }

    static float ICALL_Audio_GetSFXVolume() {
        return AudioSystem::instance().GetSFXVolume();
	}

    static float ICALL_Audio_GetMasterVolume() {
        return AudioSystem::instance().GetMasterVolume();
    }

    static void ICALL_Audio_SetListenerAttributes(CEVec3* pos, CEVec3* forward, CEVec3* up, CEVec3* velocity)
    {
        if (!pos || !forward || !up || !velocity) return;
        AudioSystem::instance().SetListenerAttributes(*pos, *forward, *up, *velocity);
    }

    static void ICALL_Audio_GetComponentData(uint32_t entityID, MonoString** outName, MonoString** outPath, float* outVol, float* outPitch, mono_bool* outLoop, mono_bool* outIsMusic, mono_bool* outSpatial3D, float* outMinDist, float* outMaxDist)
    {
        // Initialize defaults
        *outName = nullptr;
        *outPath = nullptr;
        *outVol = 1.0f; *outPitch = 1.0f; *outLoop = false; *outIsMusic = false;
        *outSpatial3D = false; *outMinDist = 1.0f; *outMaxDist = 100.0f;

        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);

        // Get data from the FIRST clip in the component
        if (!ac->clips.empty()) {
            const auto& c = ac->clips[0];

            *outName = mono_string_new(mono_domain_get(), c.name.c_str());

            // Return the Path
            std::string finalPath = c.path;
            // Ensure path has assets/audio prefix if missing
            if (finalPath.rfind("assets/", 0) != 0) finalPath = "assets/audio/" + finalPath;

            *outPath = mono_string_new(mono_domain_get(), finalPath.c_str());
            // --------------------------------

            *outVol = c.volume;
            *outPitch = c.pitch;
            *outLoop = c.looping;
            *outIsMusic = c.isMusic;
            *outSpatial3D = c.spatial3D;
            *outMinDist = c.minDistance;
            *outMaxDist = c.maxDistance;
        }
    }

    static void ICALL_Audio_SetClip3DDistances(uint32_t entityID, MonoString* clipNameStr, float minDist, float maxDist)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        std::string targetClipName = MonoToStdString(clipNameStr);

        for (auto& clip : ac->clips) {
            if (clip.name == targetClipName) {
                clip.minDistance = minDist;
                clip.maxDistance = maxDist;
                clip.spatial3D = true; // Auto-enable 3D if distances are being set
                // Position and distance sync is handled by engine.cpp every frame
                return;
            }
        }
    }

    static void ICALL_Audio_PlayClipByName(uint32_t entityID, MonoString* clipNameStr)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string target = MonoToStdString(clipNameStr);

        // Search the vector
        for (auto& clip : ac->clips) {
            if (clip.name == target) {
                // Found it -> Play it
                std::string path = clip.path;
                if (path.rfind("assets/", 0) != 0) path = "assets/audio/" + path;

                if (clip.channel) clip.channel->stop(); // Stop previous overlap

                FMOD::Channel* ch = nullptr;
                if (clip.isMusic) ch = audio.playMusicFile(path, clip.looping);
                else ch = audio.playSFXFile(path, clip.spatial3D, clip.minDistance, clip.maxDistance);

                if (ch) {
                    ch->setVolume(clip.volume);
                    ch->setPitch(clip.pitch);
                    ch->setMode(clip.looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
                    clip.channel = ch;

                    // Immediately sync 3D attributes if spatial
                    if (clip.spatial3D) {
                        Component::TransformComponent* transform = ecs->GetComponent<Component::TransformComponent>(entityID);
                        if (transform) {
                            audio.SetChannel3DAttributes(ch, transform->position, clip.minDistance, clip.maxDistance);
                        }
                    }

                    if (clip.dspType != -1) {
                        audio.AddChannelDSP(ch, (FMOD_DSP_TYPE)clip.dspType);
                    }
                }
                return;
            }
        }
    }

    static void ICALL_Audio_PlayPathOnEntity(uint32_t entityID, MonoString* pathStr, float vol, float pitch, mono_bool loop, mono_bool isMusic, mono_bool spatial3D, float minDistance, float maxDistance)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string path = MonoToStdString(pathStr);

        if (path.rfind("assets/", 0) != 0) path = "assets/audio/" + path;

        FMOD::Channel* ch = nullptr;
        if (isMusic) ch = audio.playMusicFile(path, loop);
        else ch = audio.playSFXFile(path, spatial3D, minDistance, maxDistance);

        if (ch) {
            ch->setVolume(vol);
            ch->setPitch(pitch);
            if (!isMusic && !spatial3D) ch->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        }
    }

    static void ICALL_Audio_PlayComponent(uint32_t entityID)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;
        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);

        if (!ac->clips.empty()) {
            ICALL_Audio_PlayClipByName(entityID, mono_string_new(mono_domain_get(), ac->clips[0].name.c_str()));
        }
    }

    static void ICALL_Audio_SetClipVolume(uint32_t entityID, MonoString* clipNameStr, float volume)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string targetClipName = MonoToStdString(clipNameStr);

        for (auto& clip : ac->clips) {
            if (clip.name == targetClipName) {
                if (clip.channel) {
                    audio.SetChannelVolume(clip.channel, volume);
                }
                return;
            }
        }
    }

    static void ICALL_Audio_SetClipPosition(uint32_t entityID, MonoString* clipNameStr, float x, float y, float z)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string targetClipName = MonoToStdString(clipNameStr);

        for (auto& clip : ac->clips) {
            if (clip.name == targetClipName) {
                if (clip.channel && clip.spatial3D) {
                    audio.SetChannel3DAttributes(clip.channel, {x, y, z}, clip.minDistance, clip.maxDistance);
                }
                return;
            }
        }
    }

    static void ICALL_Audio_AddClipDSP(uint32_t entityID, MonoString* clipNameStr, FMOD_DSP_TYPE dspType)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string targetClipName = MonoToStdString(clipNameStr);

        for (auto& clip : ac->clips) {
            if (clip.name == targetClipName) {
                if (clip.channel) {
                    audio.AddChannelDSP(clip.channel, dspType);
                }
                return;
            }
        }
    }

    static void ICALL_Audio_RemoveClipDSP(uint32_t entityID, MonoString* clipNameStr, FMOD_DSP_TYPE dspType)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string targetClipName = MonoToStdString(clipNameStr);

        for (auto& clip : ac->clips) {
            if (clip.name == targetClipName) {
                if (clip.channel) {
                    audio.RemoveChannelDSP(clip.channel, dspType);
                }
                return;
            }
        }
    }

    static void ICALL_Audio_SetClipDSPParameter(uint32_t entityID, MonoString* clipNameStr, FMOD_DSP_TYPE dspType, int index, float value)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID)) return;

        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        std::string targetClipName = MonoToStdString(clipNameStr);

        for (auto& clip : ac->clips) {
            if (clip.name == targetClipName) {
                if (clip.channel) {
                    audio.SetChannelDSPParameter(clip.channel, dspType, index, value);
                }
                return;
            }
        }
    }

    static void ICALL_Audio_AddGroupDSP(mono_bool isMusic, FMOD_DSP_TYPE dspType)
    {
        AudioSystem::instance().AddGroupDSP(isMusic != 0, dspType);
    }

    static void ICALL_Audio_RemoveGroupDSP(mono_bool isMusic, FMOD_DSP_TYPE dspType)
    {
        AudioSystem::instance().RemoveGroupDSP(isMusic != 0, dspType);
    }

    static void ICALL_Audio_SetGroupDSPParameter(mono_bool isMusic, FMOD_DSP_TYPE dspType, int index, float value)
    {
        AudioSystem::instance().SetGroupDSPParameter(isMusic != 0, dspType, index, value);
    }

    static void ICALL_Audio_FadeGroupDSPParameter(mono_bool isMusic, FMOD_DSP_TYPE dspType, int index, float targetValue, float duration)
    {
        AudioSystem::instance().FadeGroupDSPParameter(isMusic != 0, dspType, index, targetValue, duration);
    }

    static void ICALL_Audio_FadeGroupVolume(mono_bool isMusic, float targetVolume, float duration)
    {
        AudioSystem::instance().FadeGroupVolume(isMusic != 0, targetVolume, duration);
    }

    static mono_bool ICALL_Audio_IsTaggedPlaying(MonoString* key)
    {
        auto& audio = AudioSystem::instance();
        return audio.isTaggedPlaying(MonoToStdString(key));
    }

}
// ---------- REGISTRATION ----------

void CrashoutsEngine::Bindings::RegisterAudioBindings()
{
    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayMusicFileTagged", (const void*)ICALL_Audio_PlayMusicFileTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlaySFXFileTagged", (const void*)ICALL_Audio_PlaySFXFileTagged);

	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_IsTaggedPlaying", (const void*)ICALL_Audio_IsTaggedPlaying);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopTagged", (const void*)ICALL_Audio_StopTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopMusic", (const void*)ICALL_Audio_StopMusic);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayComponent", (const void*)ICALL_Audio_PlayComponent);

	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopComponent", (const void*)ICALL_Audio_StopComponent);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetMusicVolume", (const void*)ICALL_Audio_SetMusicVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetSFXVolume", (const void*)ICALL_Audio_SetSFXVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetTaggedVolume", (const void*)ICALL_Audio_SetTaggedVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetMasterVolume", (const void*)ICALL_Audio_SetMasterVolume);
    
	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetMusicVolume", (const void*)ICALL_Audio_GetMusicVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetSFXVolume", (const void*)ICALL_Audio_GetSFXVolume);

	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetMasterVolume", (const void*)ICALL_Audio_GetMasterVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetListenerAttributes", (const void*)ICALL_Audio_SetListenerAttributes);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetComponentData", (const void*)ICALL_Audio_GetComponentData);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayPathOnEntity", (const void*)ICALL_Audio_PlayPathOnEntity);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayClipByName", (const void*)ICALL_Audio_PlayClipByName);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetClipVolume", (const void*)ICALL_Audio_SetClipVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetClipPosition", (const void*)ICALL_Audio_SetClipPosition);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetClip3DDistances", (const void*)ICALL_Audio_SetClip3DDistances);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_AddClipDSP", (const void*)ICALL_Audio_AddClipDSP);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_RemoveClipDSP", (const void*)ICALL_Audio_RemoveClipDSP);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetClipDSPParameter", (const void*)ICALL_Audio_SetClipDSPParameter);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_AddGroupDSP", (const void*)ICALL_Audio_AddGroupDSP);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_RemoveGroupDSP", (const void*)ICALL_Audio_RemoveGroupDSP);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetGroupDSPParameter", (const void*)ICALL_Audio_SetGroupDSPParameter);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_FadeGroupDSPParameter", (const void*)ICALL_Audio_FadeGroupDSPParameter);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_FadeGroupVolume", (const void*)ICALL_Audio_FadeGroupVolume);
}