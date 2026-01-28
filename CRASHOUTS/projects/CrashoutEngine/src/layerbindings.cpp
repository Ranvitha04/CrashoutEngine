/*!
@file        layerbindings.cpp
@author      lee.r@digipen.edu
@date        26/11/2025
@brief       Definition of button bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "tagcomponent.h" 
#include "layerbindings.h"
#include "entitymanager.h"
#include "mono/metadata/loader.h"
#include "mono/metadata/object.h"
#include <string>

using namespace CrashoutsEngine;
using namespace Component;

namespace {
	// --- Instance Logic ---
	static uint32_t InternalGetLayer(uint32_t entityID) {
		auto* ecs = ECS::getInstance();
		if (ecs->HasComponent<TagComponent>(entityID)) {
			return ecs->GetComponent<TagComponent>(entityID)->tagMask;
		}
		return 0; // Default
	}

	static void InternalSetLayer(uint32_t entityID, uint32_t newLayer) {
		auto* ecs = ECS::getInstance();
		if (ecs->HasComponent<TagComponent>(entityID)) {
			ecs->GetComponent<TagComponent>(entityID)->tagMask = newLayer;
		}
	}

	// --- Static Helper: Get ID from Name ---
	// This lets C# scripts say layer.Set("Enemy") instead of layer.Set(2)
	static uint32_t InternalGetLayerByName(MonoString* name) {
		char* str = mono_string_to_utf8(name);
		std::string key(str);
		mono_free(str);

		// Check the static map
		if (TagComponent::tags.find(key) != TagComponent::tags.end()) {
			return TagComponent::tags[key];
		}
		return 0;
	}

	// --- Lifecycle ---
	static void InternalAddLayer(uint32_t id) {
		ECS::getInstance()->AddComponent<TagComponent>(id);
	}

	static void InternalRemoveLayer(uint32_t id) {
		ECS::getInstance()->RemoveComponent<TagComponent>(id);
	}

	static bool InternalEntityHasLayer(uint32_t id) {
		return ECS::getInstance()->HasComponent<TagComponent>(id);
	}
}

// Register function (Call this in your main MonoManager Init)
void CrashoutsEngine::Bindings::RegisterLayerBindings() {

	// 1. Logic (Attached to Component Class)
	mono_add_internal_call("CrashoutsEngine.LayerComponent::InternalGetLayer", (const void*)InternalGetLayer);
	mono_add_internal_call("CrashoutsEngine.LayerComponent::InternalSetLayer", (const void*)InternalSetLayer);
	mono_add_internal_call("CrashoutsEngine.LayerComponent::InternalGetLayerByName", (const void*)InternalGetLayerByName);

	// 2. Lifecycle (Attached to InternalCalls)
	mono_add_internal_call("CrashoutsEngine.InternalCalls::AddLayer", (const void*)InternalAddLayer);
	mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveLayer", (const void*)InternalRemoveLayer);
	mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasLayer", (const void*)InternalEntityHasLayer);
}
