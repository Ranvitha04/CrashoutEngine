/*!
@file		replaybindings.cpp
@author     lee.r@digipen.edu
@brief		Definition of replay bindings functions
*/

#include "replaymanager.h"
#include "monomanager.h"

namespace CrashoutsEngine::Bindings
{
    // --- WRAPPER FUNCTIONS ---
    static void Internal_ReplayStartRecording() { 
        ReplayManager::Replay& r = ReplayManager::Replay::GetInstance();
        r.StartRecording();
    }
    
    static void Internal_ReplayStop() {
        ReplayManager::Replay& r = ReplayManager::Replay::GetInstance();
        r.Stop();
    }

    static void Internal_ReplayStartPlayback() {
        ReplayManager::Replay& r = ReplayManager::Replay::GetInstance();
        r.StartPlayback();
    }

    static void Internal_ReplaySaveRecording(MonoString* monoFilename) {
        char* cstr = mono_string_to_utf8(monoFilename);
        ReplayManager::Replay& r = ReplayManager::Replay::GetInstance();
        r.SaveRecording(cstr);
        mono_free(cstr); // Free the memory Mono allocated for the string!
    }

    static void Internal_ReplayLoadRecording(MonoString* monoFilename) {
        char* cstr = mono_string_to_utf8(monoFilename);
        ReplayManager::Replay& r = ReplayManager::Replay::GetInstance();
        r.LoadRecording(cstr);
        mono_free(cstr);
    }

    // --- REGISTRATION ---
    // Add these to your main bindings registration function
    void RegisterReplayBindings() {
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStartRecording", reinterpret_cast<const void*>(Internal_ReplayStartRecording));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStop", reinterpret_cast<const void*>(Internal_ReplayStop));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStartPlayback", reinterpret_cast<const void*>(Internal_ReplayStartPlayback));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalSaveRecording", reinterpret_cast<const void*>(Internal_ReplaySaveRecording));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalLoadRecording", reinterpret_cast<const void*>(Internal_ReplayLoadRecording));
    }
}
