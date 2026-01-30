///*!
//@file       aibindings.cpp
//@author     m.biniswandi@digipen.edu
//@date       29/11/2025
//@brief      aibindings.cpp file contains the function definitions for registering AI-related bindings in the CrashoutsEngine
//
//All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
//
////__________________________________________________________________________*/
//
//#include "aimanager.h"
//#include "entitymanager.h"
//
//#include <mono/jit/jit.h>
//#include <mono/metadata/object.h>
//#include <mono/metadata/assembly.h>
//
//using namespace CrashoutsEngine;
//
//namespace
//{
//    std::string MonoStringToString(MonoString* str)
//    {
//        if (!str) return "";
//        char* cStr = mono_string_to_utf8(str);
//        std::string result(cStr);
//        mono_free(cStr); // Important: Free the raw Mono buffer
//        return result;
//    }
//
//    // --- AI UTILITY WRAPPERS ---
//
//    static bool Internal_IsPlatformInFront(uint32_t entityID, float direction, MonoString* tag)
//    {
//        // Convert Mono types to C++ types
//        return AIUtils::IsPlatformInFront((Entity)entityID, direction, MonoStringToString(tag));
//    }
//
//    static bool Internal_IsObstacleInFront(uint32_t entityID, float direction, MonoString* tag)
//    {
//        return AIUtils::IsObstacleInFront((Entity)entityID, direction, MonoStringToString(tag));
//    }
//
//    static bool Internal_IsObstacleIntersecting(uint32_t entityID, MonoString* tag)
//    {
//        return AIUtils::IsObstacleIntersecting((Entity)entityID, MonoStringToString(tag));
//    }
//
//    static bool Internal_IsIntersecting(uint32_t entity1, uint32_t entity2)
//    {
//        return AIUtils::IsIntersecting((Entity)entity1, (Entity)entity2);
//    }
//
//    static bool Internal_EntityInRange(uint32_t entity1, uint32_t entity2, float tolerance)
//    {
//        return AIUtils::EntityInRange((Entity)entity1, (Entity)entity2, tolerance);
//    }
//
//}
//namespace CrashoutsEngine::Bindings
//{
//    void RegisterAIBindings()
//    {
//        mono_add_internal_call("CrashoutsEngine.AI::IsPlatformInFront", reinterpret_cast<const void*>(Internal_IsPlatformInFront));
//        mono_add_internal_call("CrashoutsEngine.AI::IsObstacleInFront", reinterpret_cast<const void*>(Internal_IsObstacleInFront));
//        mono_add_internal_call("CrashoutsEngine.AI::IsObstacleIntersecting", reinterpret_cast<const void*>(Internal_IsObstacleIntersecting));
//        mono_add_internal_call("CrashoutsEngine.AI::IsIntersecting", reinterpret_cast<const void*>(Internal_IsIntersecting));
//        mono_add_internal_call("CrashoutsEngine.AI::EntityInRange", reinterpret_cast<const void*>(Internal_EntityInRange));
//    }
//}
//
