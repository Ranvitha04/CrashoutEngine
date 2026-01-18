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
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>

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

    static void ICALL_Audio_PlayComponent(uint32_t entityID)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();

        if (!ecs->GetComponent<Component::ActiveComponent>(entityID)->isActive) return;

        // Check if Entity has the component
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID))
        {
            return;
        }

        // Get the component data
        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        if (ac->eventPath.empty()) return;

        auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();

        // Play based on type (FMOD Event vs Raw File)
        if (ac->eventPath.find("event:/") == 0)
        {
            // FMOD Studio Event
            // Check if we already have an instance playing, stop it if so (re-trigger behavior)
            if (ac->inst) {
                ac->inst->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                ac->inst->release();
                ac->inst = nullptr;
            }

            auto* inst = audio.createInstance(ac->eventPath);
            if (inst) {
                inst->setVolume(ac->volume);
                inst->setPitch(ac->pitch);
                // 3D attributes would go here if you have 3D audio implemented
                inst->start();

                // Store it back in the component so the Inspector can see it/stop it
                ac->inst = inst;
            }
        }
        else
        {
            // Raw Audio File (wav/mp3/ogg)
            std::string path = ac->eventPath;
            if (path.rfind("assets/", 0) != 0) path = "assets/audio/" + path;

            // Play and get channel
            FMOD::Channel* ch = audio.playSFXFileEx(path);

            if (ch) {
                ch->setVolume(ac->volume);
                ch->setPitch(ac->pitch);
                ch->setMode(ac->looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

                // Store in component
                ac->channel = ch;
            }
        }
    }

    static void ICALL_Audio_StopComponent(uint32_t entityID, float fadeDuration)
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();

        if (!ecs->GetComponent<Component::ActiveComponent>(entityID)->isActive) return;

        // Check if Entity has the component
        if (!ecs->HasComponent<Component::AudioEmitterComponent>(entityID))
        {
            return;
        }
        // Get the component data
        auto* ac = ecs->GetComponent<Component::AudioEmitterComponent>(entityID);
        //auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
        // Stop based on type
        if (ac->inst)
        {
            ac->inst->stop(fadeDuration > 0.0f ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
            ac->inst->release();
            ac->inst = nullptr;
        }
        if (ac->channel)
        {
            if (fadeDuration <= 0.0f)
            {
                ac->channel->stop();
            }
            else
            {
                // Simple linear fade out over fadeDuration seconds
                float currentVolume = 0.0f;
                ac->channel->getVolume(&currentVolume);
                const int steps = 20;
                float stepTime = fadeDuration / steps;
                float stepVolume = currentVolume / steps;
                for (int i = 0; i < steps; ++i)
                {
                    currentVolume -= stepVolume;
                    if (currentVolume < 0.0f) currentVolume = 0.0f;
                    ac->channel->setVolume(currentVolume);
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepTime * 1000)));
                }
                ac->channel->stop();
            }
            ac->channel = nullptr;
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

}
// ---------- REGISTRATION ----------

void CrashoutsEngine::Bindings::RegisterAudioBindings()
{
    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayMusicFileTagged", (const void*)ICALL_Audio_PlayMusicFileTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlaySFXFileTagged", (const void*)ICALL_Audio_PlaySFXFileTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopTagged", (const void*)ICALL_Audio_StopTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopMusic", (const void*)ICALL_Audio_StopMusic);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayComponent", (const void*)ICALL_Audio_PlayComponent);

	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopComponent", (const void*)ICALL_Audio_StopComponent);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetMusicVolume", (const void*)ICALL_Audio_SetMusicVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetSFXVolume", (const void*)ICALL_Audio_SetSFXVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetMasterVolume", (const void*)ICALL_Audio_SetMasterVolume);
    
	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetMusicVolume", (const void*)ICALL_Audio_GetMusicVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetSFXVolume", (const void*)ICALL_Audio_GetSFXVolume);

	mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_GetMasterVolume", (const void*)ICALL_Audio_GetMasterVolume);
}