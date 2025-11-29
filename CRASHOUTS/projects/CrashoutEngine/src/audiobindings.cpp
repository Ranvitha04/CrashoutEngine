#include "audiobindings.h"
#include "audioSystem.h"
#include "audiocomponent.h"
#include "entitymanager.h"
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

    static void ICALL_Audio_StopTagged(MonoString* key)
    {
        auto& audio = AudioSystem::instance();
        audio.stopTagged(MonoToStdString(key));
    }

    static void ICALL_Audio_StopMusic()
    {
        auto& audio = AudioSystem::instance();
        audio.stopMusicGroup();
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
}
// ---------- REGISTRATION ----------

void CrashoutsEngine::Bindings::RegisterAudioBindings()
{
    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlayMusicFileTagged", (const void*)ICALL_Audio_PlayMusicFileTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_PlaySFXFileTagged", (const void*)ICALL_Audio_PlaySFXFileTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopTagged", (const void*)ICALL_Audio_StopTagged);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_StopMusic", (const void*)ICALL_Audio_StopMusic);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetMusicVolume", (const void*)ICALL_Audio_SetMusicVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetSFXVolume", (const void*)ICALL_Audio_SetSFXVolume);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::Audio_SetMasterVolume", (const void*)ICALL_Audio_SetMasterVolume);
}