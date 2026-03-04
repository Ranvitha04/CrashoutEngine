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
    
    static void Internal_ReplayStopRecording() { ReplayManager::Replay::GetInstance().StopRecording(); }
    static void Internal_ReplayStopPlayback() { ReplayManager::Replay::GetInstance().StopPlayback(); }

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

    static bool Internal_ReplayLoadRecording(MonoString* monoFilename) {
        char* cstr = mono_string_to_utf8(monoFilename);
        ReplayManager::Replay& r = ReplayManager::Replay::GetInstance();
        bool success = r.LoadRecording(cstr);
        mono_free(cstr);
        return success;
    }
    
    static void Internal_ReplayRecordTransform(float x, float y, float z, float rot) {
        ReplayManager::Replay::GetInstance().RecordCurrentTransform(x, y, z, rot);
    }

    // Pointers allow us to return multiple values back to C#
    static bool Internal_ReplayGetGhostTransform(float* x, float* y, float* z, float* rot) {
        return ReplayManager::Replay::GetInstance().GetGhostTransform(*x, *y, *z, *rot);
    }

    // --- REGISTRATION ---
    // Add these to your main bindings registration function
    void RegisterReplayBindings() {
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStartRecording", reinterpret_cast<const void*>(Internal_ReplayStartRecording));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStopRecording", reinterpret_cast<const void*>(Internal_ReplayStopRecording));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStopPlayback", reinterpret_cast<const void*>(Internal_ReplayStopPlayback));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalStartPlayback", reinterpret_cast<const void*>(Internal_ReplayStartPlayback));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalSaveRecording", reinterpret_cast<const void*>(Internal_ReplaySaveRecording));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalLoadRecording", reinterpret_cast<const void*>(Internal_ReplayLoadRecording));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalRecordTransform", reinterpret_cast<const void*>(Internal_ReplayRecordTransform));
        mono_add_internal_call("CrashoutsEngine.ReplayManager::InternalGetGhostTransform", reinterpret_cast<const void*>(Internal_ReplayGetGhostTransform));
    }
}
