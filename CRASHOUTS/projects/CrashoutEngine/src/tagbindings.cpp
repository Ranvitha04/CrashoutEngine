/*!
@file        tagbindings.cpp
@author      lee.r@digipen.edu
@date        26/11/2025
@brief       Definition of button bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "tagcomponent.h" 
#include "tagbindings.h"
#include "entitymanager.h"
#include "mono/metadata/loader.h"
#include "mono/metadata/object.h"
#include <string>

using namespace CrashoutsEngine;
using namespace Component;

namespace {
	// --- Instance Logic ---
	static uint32_t InternalGetTag(uint32_t entityID) {
		auto* ecs = ECS::getInstance();
		if (ecs->HasComponent<TagComponent>(entityID)) {
			return ecs->GetComponent<TagComponent>(entityID)->tagMask;
		}
		return 0; // Default
	}

	static void InternalSetTag(uint32_t entityID, uint32_t newLayer) {
		auto* ecs = ECS::getInstance();
		if (ecs->HasComponent<TagComponent>(entityID)) {
			ecs->GetComponent<TagComponent>(entityID)->tagMask = newLayer;
		}
	}

	// --- Static Helper: Get ID from Name ---
	// This lets C# scripts say tag.Set("Enemy") instead of tag.Set(2)
	static uint32_t InternalGetTagByName(MonoString* name) {
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
	static void InternalAddTag(uint32_t id) {
		ECS::getInstance()->AddComponent<TagComponent>(id);
	}

	static void InternalRemoveTag(uint32_t id) {
		ECS::getInstance()->RemoveComponent<TagComponent>(id);
	}

	static bool InternalEntityHasTag(uint32_t id) {
		return ECS::getInstance()->HasComponent<TagComponent>(id);
	}
}

// Register function (Call this in your main MonoManager Init)
void CrashoutsEngine::Bindings::RegisterTagBindings() {

	// 1. Logic (Attached to Component Class)
	mono_add_internal_call("CrashoutsEngine.TagComponent::InternalGetTag", (const void*)InternalGetTag);
	mono_add_internal_call("CrashoutsEngine.TagComponent::InternalSetTag", (const void*)InternalSetTag);
	mono_add_internal_call("CrashoutsEngine.TagComponent::InternalGetTagByName", (const void*)InternalGetTagByName);

	// 2. Lifecycle (Attached to InternalCalls)
	mono_add_internal_call("CrashoutsEngine.InternalCalls::AddTag", (const void*)InternalAddTag);
	mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveTag", (const void*)InternalRemoveTag);
	mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasTag", (const void*)InternalEntityHasTag);
}
