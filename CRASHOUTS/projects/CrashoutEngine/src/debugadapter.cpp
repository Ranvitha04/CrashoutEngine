/*!
@file       debugadapter.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

Declares and registers built-in Debug Menu adapters & providers:
- ECS component adapters: Name, Transform, RigidBody, Collider, MeshComponent
- GameStateProvider: bridges GameStateManager to the UI
- BuiltinPhysicsProvider: gravity & damping controls via reflection

These adapters make the ECS inspector and physics/game-state panels
runtime-extensible and project-agnostic.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "debugreflection.h"
#include "entitymanager.h"
#include <imgui.h>
#include <algorithm>
#include <filesystem>
//#include <numeric>
#include <fstream>
#include "debugdefaults.h"
#include "debugAssetLoader.h"
#include "changevaluecommand.h"
#include "scriptvalidation.h"
#include "undomanager.h"
#include "hierarchy.h"

/*                                                         component includes
----------------------------------------------------------------------------- */
#include "namecomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "audiocomponent.h"
#include "textcomponent.h"
#include "scriptcomponent.h"
#include "cameracomponent.h"
#include "physicsmanager.h"
#include "ppcomponent.h"
#include "collidercomponent.h"
#include "tilemapcomponent.h"
#include "keyframe.h" 
#include "hierarchycomponent.h"
#include "particleComponent.h"
#include "activecomponent.h"

// FOR GAME GUI
#include "buttoncomponent.h"
#include "guicomponent.h"
#include "textinputcomponent.h"
#include "uilayoutcomponent.h"

#include "collisionmanager.h"
#include "gamestatemanager.h"
#include "assetmanager.h"
#include "monomanager.h"
#include "audioSystem.h"
#include "tilemapasset.h"
#include "imgui_internal.h"
#include "reflectimgui.h"
#include "shapes.h"
#include "layercomponent.h"
#include "transformentitycommand.h"
#include <memory>

// FOR MONO SCRIPTING
#include <mono/metadata/reflection.h>
#include <mono/metadata/attrdefs.h>


using namespace CrashoutsEngine;
using namespace Component;

//---------------------------------------- file-local state & helpers ---------------------------------------- 
/*!
 @brief File-local adapters/providers and registration.

 All adapters/providers live in an anonymous namespace and are registered
 once via RegisterBuiltinComponentAdapters(), which the app calls at startup.
*/
namespace {

	//------------------------------------- UI Helpers for Undo/ Redo logic -------------------------------------- 
	namespace UIHelpers {
		/*
		@brief Generic Undo Helper
		@usage DrawUndoable("Label", myFloat, DrawFunc); DrawFunc should return true if value changed
		*/
		template<typename T, typename Func>
		void DrawUndoableGeneric(const char* label, T& ref, Func drawFunc, std::function<void(const T&)> customSetter = nullptr) {
			static std::unordered_map<ImGuiID, T> startValues;
			ImGuiID id = ImGui::GetID(label);

			// Draw UI
			if (drawFunc()) {
				// Value modified this frame
			}

			// Capture start value
			if (ImGui::IsItemActivated()) {
				startValues[id] = ref;
			}

			// Capture start value
			if(ImGui::IsItemDeactivatedAfterEdit()){
				if (startValues.find(id) != startValues.end()) {
					T start = startValues[id];
					T end = ref;
					if (start != end) {
						// If a custom setter is provided, use it. Otherwise default to writing to the reference.
						auto applyFunc = customSetter ? customSetter : [&ref](const T& v) { ref = v; };

						UndoManager::GetInstance().AddCommand(
							std::make_unique<ChangeValueCommand<T>>(start, end, applyFunc)
						);
					}
					startValues.erase(id);
				}
			}
		}

		// Specializations 
		static void DrawUndoableFloat(const char* label, float& ref, float speed = 0.1f, float min = 0.0f, float max = 0.0f, std::function<void(const float&)> setter = nullptr) {
			DrawUndoableGeneric(label, ref, [&]() { return ImGui::DragFloat(label, &ref, speed, min, max); }, setter);
		}

		static void DrawUndoableVec2(const char* label, CEVec2& ref, float speed = 0.1f, std::function<void(const CEVec2&)> setter = nullptr) {
			DrawUndoableGeneric(label, ref, [&]() { return ImGui::DragFloat2(label, &ref.x, speed); }, setter);
		}

		static void DrawUndoableVec3(const char* label, CEVec3& ref, float speed = 0.1f, std::function<void(const CEVec3&)> setter = nullptr) {
			DrawUndoableGeneric(label, ref, [&]() { return ImGui::DragFloat3(label, &ref.x, speed); }, setter);
		}

		static void DrawUndoableInt(const char* label, int& ref, float speed = 1.0f, int min = 0, int max = 0, std::function<void(const int&)> setter = nullptr) {
			DrawUndoableGeneric(label, ref, [&]() { return ImGui::DragInt(label, &ref, speed, min, max); }, setter);
		}

		/*
		@brief Helper for Bools; Callable to create imgui editable boolean value
		*/
		static void DrawUndoableBool(const char* label, bool& ref, std::function<void(const bool&)> setter = nullptr) {
			bool old = ref;
			if (ImGui::Checkbox(label, &ref)) {
				auto applyFunc = setter ? setter : [&ref](const bool& v) { ref = v; };
				UndoManager::GetInstance().AddCommand(
					std::make_unique<ChangeValueCommand<bool>>(old, ref, applyFunc)
				);
			}
		}
	
		/*
		@brief Helper for std::string; Callable to create imgui editable string value
		*/
		static void DrawUndoableString(const char* label, std::string& ref) {
			static std::unordered_map<ImGuiID, std::string> startValues;
			ImGuiID id = ImGui::GetID(label);

			char buf[256];
			strncpy(buf, ref.c_str(), sizeof(buf));

			if (ImGui::InputText(label, buf, sizeof(buf))) {
				ref = std::string(buf);
			}

			if (ImGui::IsItemActivated()) startValues[id] = ref;

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				if (startValues.count(id)) {
					std::string start = startValues[id];
					if (start != ref) {
						UndoManager::GetInstance().AddCommand(
							std::make_unique<ChangeValueCommand<std::string>>(start, ref, [&ref](const std::string& v) { ref = v; })
						);
					}
					startValues.erase(id);
				}
			}
		}

		/*
		@brief Helper for CEColor; Callable to create imgui editable color value
		*/
		static void DrawUndoableColor(const char* label, CEColor& ref) {
			static std::unordered_map<ImGuiID, CEColor> startValues;
			static bool isEditing = false;

			ImGuiID id = ImGui::GetID(label);
			float col[4] = { ref.x(), ref.y(), ref.z(), ref.w() };

			if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaBar)) {
				if (!isEditing) {
					startValues[id] = ref;
					isEditing = true;
				}
				ref = { col[0], col[1], col[2], col[3] };
			}

			if (ImGui::IsItemDeactivated()) {
				if (isEditing && startValues.count(id)) {
					CEColor start = startValues[id];
					if (start != ref) {
						UndoManager::GetInstance().AddCommand(
							std::make_unique<ChangeValueCommand<CEColor>>(start, ref, [&ref](const CEColor& v) { ref = v; })
						);
					}
					startValues.erase(id);
				}
				isEditing = false;
			}
		}
	}

	f32 max(f32 x1, f32 y2, f32 z3) {
		return std::max(std::max(x1, y2), z3);
	}

	/*!
	@brief Adapter for editing Component::NameComponent (text name).
	@details Shows a single InputText and syncs the value back to the ECS component.
	*/
	struct NameAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "NameComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::NameComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			auto& D = GetDebugDefaults();
			if (!has(ecs, e)) ecs.AddComponent<Component::NameComponent>(e, D.name_default);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::NameComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			
			// For active component (Set gameobject active or not active)
			if (auto* ac = ecs.GetComponent<Component::ActiveComponent>(e)) {
				ImGui::PushID("ActiveToggle");
				UIHelpers::DrawUndoableBool("Enable GameObject##Active", ac->isActive);
				ImGui::PopID();
			}
			// Add active component if not yet created
			else {
				ecs.AddComponent<Component::ActiveComponent>(e, true);
			}

			if (auto* c = ecs.GetComponent<Component::NameComponent>(e)) {
				UIHelpers::DrawUndoableString("Entity Name", c->name);
			}
		}
	};

	/*!
	@brief Shared helper to draw a multi-select layer dropdown.
	@param ecs Reference to ECS (needed to clean up components if a layer is deleted).
	@param mask Reference to the bitmask integer to edit.
	*/
	static void DrawLayerMultiSelect(ECS& ecs, unsigned int& mask) {
		// Get the static map from LayerComponent
		std::map<std::string, unsigned int>& layerMap = Component::LayerComponent::layers;

		// --- 1. Build the Preview String ---
		std::string preview = "";
		int selectedCount = 0;

		for (auto const& [name, bit] : layerMap) {
			if (mask & bit) {
				if (selectedCount > 0) preview += ", ";
				preview += name;
				selectedCount++;
			}
		}

		if (selectedCount == 0) preview = "None";
		else if (selectedCount > 2) preview = "Mixed...";

		// --- 2. Draw the Multi-Select Dropdown ---
		if (ImGui::BeginCombo("Layer Mask", preview.c_str())) {

			std::string layerToRemove = "";

			// --- 3. Draw Checkboxes ---
			for (auto const& [name, bit] : layerMap) {
				bool isSelected = (mask & bit) != 0;

				// Use Selectable so we can detect clicks easily
				if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups)) {
					unsigned int oldMask = mask;
					unsigned int newMask = mask ^ bit; // Toggle

					UndoManager::GetInstance().AddCommand(
						std::make_unique<ChangeValueCommand<unsigned int>>(
							oldMask, newMask, [&mask](unsigned int v) { mask = v; }
						)
					);
					mask = newMask;
				}

				// Remove Button
				if (name != "Default") {
					ImGui::SameLine();
					if (ImGui::SmallButton(("X##" + name).c_str())) layerToRemove = name;
				}
			}

			// --- 4. Removal Logic (Global removal) ---
			if (!layerToRemove.empty()) {
				// Complex undo: Removing a layer affects ALL entities. 
				// For simplicity, just undo the removal from the map, not the entities.
				unsigned int bit = layerMap[layerToRemove];
				layerMap.erase(layerToRemove);

				// Clear from current mask with undo
				unsigned int oldMask = mask;
				mask &= ~bit;
				if (mask != oldMask) {
					UndoManager::GetInstance().AddCommand(
						std::make_unique<ChangeValueCommand<unsigned int>>(oldMask, mask, [&mask](unsigned int v) { mask = v; })
					);
				}

				// Clean up all entities immediately (Not undoable currently for simplicity)
				for (LayerComponent& lc : ecs.GetComponentArray<LayerComponent>().dense) {
					lc.currentLayer &= ~bit;
				}
			}

			// --- 5. Add New Layer Logic ---
			ImGui::Separator();
			static char newLayerBuf[64] = "";
			bool add = ImGui::InputText("New Layer", newLayerBuf, 64, ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if (ImGui::Button("Add") || add) {
				if (strlen(newLayerBuf) > 0 && layerMap.find(newLayerBuf) == layerMap.end()) {
					unsigned int nextBit = 1;
					if (!layerMap.empty()) {
						auto maxEl = std::max_element(layerMap.begin(), layerMap.end(),
							[](const auto& a, const auto& b) { return a.second < b.second; });
						nextBit = maxEl->second << 1;
					}
					// TODO: Add AddLayerCommand for undoing layer creation
					layerMap.emplace(newLayerBuf, nextBit);
					newLayerBuf[0] = '\0';
				}
			}

			ImGui::EndCombo();
		}
	}

	/*!
	@brief Adapter for editing Component::LayerComponent.
	@details Shows a drop down of layers to choose from and create
	*/
	struct LayerAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "LayerComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::LayerComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			//auto& D = GetDebugDefaults();
			if (!has(ecs, e)) ecs.AddComponent<Component::LayerComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::LayerComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			if (auto* c = ecs.GetComponent<Component::LayerComponent>(e)) {
				// Use the helper function!
				DrawLayerMultiSelect(ecs, c->currentLayer);
			}
		}
	};
	static void ApplyLocalToWorldNow(ECS& ecs, Entity child)
	{
		if (!ecs.HasComponent<Component::ParentComponent>(child)) return;

		auto* pc = ecs.GetComponent<Component::ParentComponent>(child);
		if (!pc || pc->parent == 0) return;

		if (!ecs.HasComponent<Component::TransformComponent>(child)) return;
		if (!ecs.HasComponent<Component::TransformComponent>(pc->parent)) return;

		auto* childT = ecs.GetComponent<Component::TransformComponent>(child);
		auto* parentT = ecs.GetComponent<Component::TransformComponent>(pc->parent);

		const float DEG_TO_RAD = 0.0174532925f;
		float parentRad = parentT->rotation * DEG_TO_RAD;
		float s = std::sin(parentRad);
		float c = std::cos(parentRad);

		float pX = (parentT->size.x == 0.f) ? 1.f : parentT->size.x;
		float pY = (parentT->size.y == 0.f) ? 1.f : parentT->size.y;

		// WORLD SCALE
		childT->size.x = parentT->size.x * pc->localScale.x;
		childT->size.y = parentT->size.y * pc->localScale.y;

		// WORLD ROT
		childT->rotation = parentT->rotation + pc->localRot;

		// WORLD POS
		float scaledOffsetX = pc->localPos.x * pX;
		float scaledOffsetY = pc->localPos.y * pY;

		float rotatedX = (c * scaledOffsetX) - (s * scaledOffsetY);
		float rotatedY = (s * scaledOffsetX) + (c * scaledOffsetY);

		childT->position.x = parentT->position.x + rotatedX;
		childT->position.y = parentT->position.y + rotatedY;
		childT->position.z = parentT->position.z + pc->localPos.z;
	}

	/*!
	@brief Adapter for editing Component::TransformComponent (position/scale/rotation).
	@details Uses ImGui drag controls and writes values back to the ECS each frame.
	*/
	struct TransformAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "TransformComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::TransformComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::TransformComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::TransformComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			using CrashoutsEngine::TransformState;
			auto* t = ecs.GetComponent<Component::TransformComponent>(e);
			if (!t) return;

			// --- HIERARCHY AWARENESS FIX ---
			// Check if this entity is a child
			Component::ParentComponent* parentComp = nullptr;
			if (ecs.HasComponent<Component::ParentComponent>(e)) {
				parentComp = ecs.GetComponent<Component::ParentComponent>(e);
				// Ensure parent is valid (id != 0)
				if (parentComp && parentComp->parent == 0) parentComp = nullptr;
			}

			if (parentComp) {
				// ============================================================
				// CHILD MODE: Edit Local Transform (ParentComponent)
				// ============================================================
				ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Child Mode (Local Space)");

				UIHelpers::DrawUndoableVec3("Local Position", parentComp->localPos);
				UIHelpers::DrawUndoableFloat("Local Rotation", parentComp->localRot);
				UIHelpers::DrawUndoableVec2("Local Scale", parentComp->localScale);
				ApplyLocalToWorldNow(ecs, e);

			}
			else {
				//draw_reflected(*t, "Transform");	// <--- how?

				ImGui::SeparatorText("World Transform");

				using CrashoutsEngine::TransformState;

				TransformState before{ t->position, t->rotation, t->size };
				static bool sEditing = false;
				static Entity sEditingEntity = 0;
				static TransformState sStart{};

				UIHelpers::DrawUndoableVec3("Position", t->position);
				UIHelpers::DrawUndoableFloat("Rotation", t->rotation);

				UIHelpers::DrawUndoableVec2("Scale", t->size);
			}
		}
	};

	/*!
	@brief Adapter for editing Component::RigidBodyComponent (velocity / forces).
	@details Displays editable velocity, read-only mass & restitution (with current API), and provides one-shot force utilities for quick testing.
	*/
	struct RigidBodyAdapter : DebugReflection::IComponentAdapter {

		bool simulateDynamics{ true };

		const char* name() const override { return "RigidBody"; }

		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<RigidBodyComponent>(e);
		}

		void add(ECS& ecs, Entity e) override {
			auto& D = GetDebugDefaults();
			ecs.AddComponent<RigidBodyComponent>(e, D.rb_init_velocity, D.rb_mass, D.rb_restitution);
		}

		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<RigidBodyComponent>(e);
		}

		void drawInspector(ECS& ecs, Entity e) override {
			auto* rb = ecs.GetComponent<RigidBodyComponent>(e);
			if (!rb) return;

			ImGui::SeparatorText("Dynamics");

			// Set rigidbody mass
			float mass = rb->getMass();
			UIHelpers::DrawUndoableFloat("Mass", mass, 0.1f, 0.0f, 0.0f, [rb](const float& v) { rb->setMass(v); });
			if (mass != rb->getMass()) rb->setMass(mass);

			// Set rigidbody restitution (bounciness)
			float rest = rb->GetCoeff();
			UIHelpers::DrawUndoableFloat("Restitution", rest, 0.01f, 0.0f, 1.0f, [rb](const float& v) { rb->setRestitution(v); });
			if (rest != rb->GetCoeff()) rb->setRestitution(rest);

			// Set Static / Dynamic friction
			UIHelpers::DrawUndoableFloat("Static Friction", rb->staticFriction, 0.01f, 0.0f, 1.0f);
			UIHelpers::DrawUndoableFloat("Dynamic Friction", rb->dynamicFriction, 0.01f, 0.0f, 1.0f);

			// Set intertia
			float inertia = rb->inertia;
			UIHelpers::DrawUndoableFloat("Inertia", inertia, 1.0f, 0.0f, 0.0f, [rb](const float& v) { rb->setInertia(v); });
			if (inertia != rb->inertia) rb->setInertia(inertia);

			// HELPER to automatically calculate inertia
			if (ecs.HasComponent<ColliderComponent>(e)) {
				if (ImGui::Button("Auto-Calc Inertia")) {
					auto* cc = ecs.GetComponent<ColliderComponent>(e);
					if (cc && cc->collider) rb->setInertia(cc->collider->CalculateInertia(rb->getMass()));
				}
			}

			// Extra force settings to apply externally onto object
			ImGui::SeparatorText("Forces");
			static float f[3]{ 0.f, 0.f, 0.f };
			ImGui::InputFloat3("Force Vector", f);
			if (ImGui::Button("Apply Force")) rb->addForce({ f[0], f[1], f[2] });
			ImGui::SameLine();
			if (ImGui::Button("Clear Forces")) rb->clearForceAccumulator();
		}

	};

	/*!
	@brief Adapter for editing Component::ColliderComponent (Circle / ColliderType::AABB).
	@details Lets users switch collider type and tweak radius/min/max; can snap to Transform.
	*/
	struct ColliderAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "Collider"; }

		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<ColliderComponent>(e);
		}

		void add(ECS& ecs, Entity e) override {
			auto& D = GetDebugDefaults();
			// Default to a 1.0-radius circle at the entity's transform position (if present)
			CEVec3 center{ 0.f,0.f,0.f };
			if (auto* t = ecs.GetComponent<TransformComponent>(e)) center = t->position;

			auto col = std::make_unique<Collision::Circle>(D.sphere_radius, center);
			col->ColliderType = Collision::ColliderType::CIRCLE;
			ecs.AddComponent<ColliderComponent>(e, std::move(col));

			if (!ecs.HasComponent<RigidBodyComponent>(e)) {
				auto& D2 = GetDebugDefaults();
				ecs.AddComponent<RigidBodyComponent>(e, CEVec3{ 0.f,0.f,0.f }, 0.0f, D2.rb_restitution);
			}
		}

		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<ColliderComponent>(e);
		}

		void drawInspector(ECS& ecs, Entity e) override {
			auto* cc = ecs.GetComponent<ColliderComponent>(e);
			if (!cc) return;

			if (cc->collider != nullptr) {
				UIHelpers::DrawUndoableBool("Enabled", cc->collider->enabled);
				UIHelpers::DrawUndoableBool("Is Trigger", cc->collider->isTrigger);
			}

			// Type Switching
			//int type = (cc->collider != nullptr) ? static_cast<int>(cc->collider->ColliderType) : 0;
			//const char* items[] = { "Circle", "AABB", "OBB", "Line", "Triangle", "None" };
			const char* items[] = { "Circle", "AABB", "Line", "Triangle", "None" };				// OBB not really working

			static const Collision::ColliderType typeMap[] = {
				Collision::ColliderType::CIRCLE,   // Index 0 -> Circle
				Collision::ColliderType::AABB,     // Index 1 -> AABB
				Collision::ColliderType::LINE,     // Index 2 -> Line (Skipped OBB)
				Collision::ColliderType::TRIANGLE, // Index 3 -> Triangle
				Collision::ColliderType::NONE      // Index 4 -> None (Skipped Polygon)
			};

			// Find current selection (Enum -> UI Index)
			// We default to "None" (Index 4) if the current type isn't in our allowed list (like OBB)
			int currentItemIndex = 4;

			if (cc->collider) {
				for (int i = 0; i < IM_ARRAYSIZE(typeMap); ++i) {
					if (cc->collider->ColliderType == typeMap[i]) {
						currentItemIndex = i;
						break;
					}
				}
			}

			// Draw Combo & Handle Change
			if (ImGui::Combo("Type", &currentItemIndex, items, IM_ARRAYSIZE(items))) {

				// Convert UI Index back to Real Enum
				Collision::ColliderType selectedType = typeMap[currentItemIndex];

				// Handle "None" explicitly to remove component data
				if (selectedType == Collision::ColliderType::NONE) {
					cc->collider = nullptr;
				}
				else {
					Collision::ColliderFactory factory;

					// Pass the REAL Enum (selectedType), not the UI index!
					auto newCol = factory.CreateCollider(selectedType);

					if (newCol) {
						newCol->ColliderType = selectedType;
						cc->collider = std::move(newCol);
					}
				}
			}

			// Sync with Transform
			CEVec3 transformPos{ 0.f,0.f,0.f };
			if (auto* t = ecs.GetComponent<TransformComponent>(e)) transformPos = t->position;

			using namespace Collision;

			if (cc->collider != nullptr) {

				// --- CIRCLE ---
				if (cc->collider->ColliderType == ColliderType::CIRCLE) {
					auto* s = static_cast<Circle*>(cc->collider.get());

					ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "Circle Collider");

					UIHelpers::DrawUndoableFloat("Radius", s->radius, 0.05f, 0.0f);
					UIHelpers::DrawUndoableVec3("Offset", s->offset);
					s->position = transformPos + s->offset;
				}
				// --- AABB ---
				else if (cc->collider->ColliderType == ColliderType::AABB) {
					auto* a = static_cast<AABBRect*>(cc->collider.get());

					ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "AABB Collider");

					UIHelpers::DrawUndoableVec3("Extents", a->extents);
					UIHelpers::DrawUndoableVec3("Offset", a->offset);

					if (ImGui::Button("Fit to Transform")) {
						if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
							a->extents = { t->size.x * 0.5f, t->size.y * 0.5f, 0.f };
						}
					}
					a->min = transformPos + a->offset - a->extents;
					a->max = transformPos + a->offset + a->extents;
				}
				// --- LINE ---
				else if (cc->collider->ColliderType == ColliderType::LINE) {
					auto* l = static_cast<LineSegment*>(cc->collider.get());
					if (l) {
						ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "Line Segment");
						UIHelpers::DrawUndoableVec3("P1 (Start)", l->p1);
						UIHelpers::DrawUndoableVec3("P2 (End)", l->p2);
						UIHelpers::DrawUndoableVec3("Offset", l->offset);

						if (ImGui::Button("Fit to Transform Width")) {
							if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
								float halfW = t->size.x * 0.5f;
								l->p1 = CEVec3{ -halfW, 0.f, 0.f };
								l->p2 = CEVec3{ halfW, 0.f, 0.f };
							}
						}
						l->UpdateCollider(transformPos);
						if (auto* t = ecs.GetComponent<TransformComponent>(e)) l->UpdateRotation(t->rotation);
					}
				}
				// --- TRIANGLE ---
				else if (cc->collider->ColliderType == ColliderType::TRIANGLE)
				{
					auto* p = static_cast<Collision::Polygon*>(cc->collider.get());
					ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "Triangle Collider");
					bool changed = false;

					// Size (Extents) - requires Rescale logic on change
					CEVec2 currentSize = { p->extents.x, p->extents.y };
					CEVec2 oldSize = currentSize;
					UIHelpers::DrawUndoableVec2("Size", currentSize, 0.05f);
					if (currentSize.x != oldSize.x || currentSize.y != oldSize.y) {
						p->Rescale(CEVec3(currentSize.x, currentSize.y, 0.f));
						changed = true;
					}

					// Rotation
					float oldRot = p->rotation;
					UIHelpers::DrawUndoableFloat("Rotation", p->rotation, 0.05f);
					if (p->rotation != oldRot) changed = true;

					// Offset
					CEVec3 oldOffset = p->offset;
					UIHelpers::DrawUndoableVec3("Offset", p->offset);
					if (p->offset != oldOffset) changed = true;

					// Fit Button
					if (ImGui::Button("Fit to Sprite Size")) {
						if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
							CEVec3 oldExt = p->extents;
							CEVec3 newExt = CEVec3(t->size.x * .5f, t->size.y * .5f, 0.f);
							UndoManager::GetInstance().AddCommand(
								std::make_unique<ChangeValueCommand<CEVec3>>(oldExt, newExt, [p](const CEVec3& v) { p->Rescale(v); })
							);
							p->Rescale(newExt);
							changed = true;
						}
					}

					// Visual Updates
					if (changed) {
						p->UpdatePosition(transformPos);
						p->UpdateRotation(p->rotation);
					}
				}
				// --- OBB/POLYGON INSPECTOR ---
				else if (cc->collider->ColliderType == ColliderType::OBB || cc->collider->ColliderType == ColliderType::POLYGON)
				{
					auto* p = static_cast<Collision::Polygon*>(cc->collider.get());
					ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "OBB Collider");
					bool changed = false;

					UIHelpers::DrawUndoableVec3("Extents", p->extents);

					CEVec3 oldOffset = p->offset;
					UIHelpers::DrawUndoableVec3("Offset", p->offset);
					if (p->offset != oldOffset) changed = true;

					float oldRot = p->rotation;
					UIHelpers::DrawUndoableFloat("Rotation", p->rotation);
					if (p->rotation != oldRot) changed = true;

					if (ImGui::Button("Fit to Sprite")) {
						if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
							p->Rescale(CEVec3(t->size.x * 0.5f, t->size.y * 0.5f, 0.f));
							changed = true;
						}
					}

					// Update Collider Logic
					p->UpdateCollider(transformPos);
					if (changed) {
						if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
							p->UpdateRotation(t->rotation);
						}
						else {
							p->UpdateRotation(p->rotation);
						}
					}
				}
			}
		}
	};

	/*!
	@brief Adapter for editing Component::ShapeComponent (debug primitives).
	@details Edits type-specific parameters and supports a per-entity preview toggle.
	*/
	struct ShapeAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "ShapeComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::ShapeComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::ShapeComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::ShapeComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			auto* sc = ecs.GetComponent<ShapeComponent>(e);
			if (!sc) return;

			ImGui::SeparatorText("Shape Settings");

			// Type
			int t = (int)sc->type;
			if (ImGui::Combo("Type", &t, "Rectangle\0Line\0Triangle\0Circle\0")) {
				UndoManager::GetInstance().AddCommand(
					std::make_unique<ChangeValueCommand<int>>((int)sc->type, t, [sc](int v) { sc->type = (ShapeType)v; })
				);
				sc->type = (ShapeType)t;
			}

			UIHelpers::DrawUndoableBool("World Space", sc->isWorldSpace);
			UIHelpers::DrawUndoableBool("Use Texture", sc->enableTexture);

			// Texture Name (Keep DragDrop logic, simplified here to String Input for brevity)
			UIHelpers::DrawUndoableString("Texture Asset", sc->textureName);

			// Drag & Drop support (ASSET/PAYLOAD)
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
					if (payload->Data && payload->DataSize == sizeof(AssetDragPayload)) {
						const auto* d = static_cast<const AssetDragPayload*>(payload->Data);
						if (d->kind == AssetKind::Texture) {
							std::string oldVal = sc->textureName;
							std::string newVal = std::string(d->relPath);
							sc->textureName = newVal;
							UndoManager::GetInstance().AddCommand(
								std::make_unique<ChangeValueCommand<std::string>>(oldVal, newVal, [sc](const std::string& v) { sc->textureName = v; })
							);
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			// Search / pick popup button
			ImGui::SameLine();
			float btnSize = ImGui::GetFrameHeight();
			if (ImGui::Button("...", ImVec2(btnSize, btnSize))) {
				ImGui::OpenPopup("PickShapeTexturePopup");
			}

			if (ImGui::BeginPopupModal("PickShapeTexturePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				namespace fs = std::filesystem;
				static std::vector<std::string> s_assets;
				static bool s_scanned = false;
				static char s_filter[128] = "";

				if (ImGui::Button("Refresh")) s_scanned = false;
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-1);
				ImGui::InputTextWithHint("##tex_filter", "filter...", s_filter, sizeof(s_filter));

				if (!s_scanned) {
					s_scanned = true;
					s_assets.clear();
					const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "images";
					try {
						if (fs::exists(assetRoot)) {
							for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
								if (!p.is_regular_file()) continue;
								std::string ext = p.path().extension().string();
								std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
								if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
									std::string rel = fs::relative(p.path(), assetRoot).generic_string();
									s_assets.push_back(rel);
								}
							}
							std::sort(s_assets.begin(), s_assets.end());
						}
					}
					catch (...) { /* ignore scan errors */ }
				}

				ImGui::Separator();
				ImGui::BeginChild("tex_list", ImVec2(420, 300), true);
				for (const auto& a : s_assets) {
					if (s_filter[0] && a.find(s_filter) == std::string::npos) continue;
					if (ImGui::Selectable(a.c_str(), sc->textureName == a)) {
						std::string oldVal = sc->textureName;
						sc->textureName = a;
						UndoManager::GetInstance().AddCommand(
							std::make_unique<ChangeValueCommand<std::string>>(oldVal, sc->textureName, [sc](const std::string& v) { sc->textureName = v; })
						);
						ImGui::CloseCurrentPopup();
						break;
					}
				}
				ImGui::EndChild();

				ImGui::Separator();
				if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}

			// UVs
			if (sc->enableTexture) {
				//UIHelpers::DrawUndoableInt("V Wrap", sc->wrapModeV);
				//UIHelpers::DrawUndoableInt("U Wrap", sc->wrapModeU);
				UIHelpers::DrawUndoableVec2("UV Size", sc->uvSize);
				UIHelpers::DrawUndoableVec2("UV Offset", sc->uvOffset);
			}

			UIHelpers::DrawUndoableColor("Color", sc->color);

			if (sc->type == ShapeType::Line) {
				ImGui::SeparatorText("Line Details");
				UIHelpers::DrawUndoableVec3("Start (P1)", sc->p1);
				UIHelpers::DrawUndoableVec3("End (P2)", sc->p2);
				UIHelpers::DrawUndoableFloat("Thickness", sc->thickness);

				if (ImGui::Button("Fit to Transform Width")) {
					if (auto* transform = ecs.GetComponent<TransformComponent>(e)) {
						CEVec3 oldP1 = sc->p1;
						CEVec3 oldP2 = sc->p2;
						float halfW = transform->size.x * 0.5f;
						CEVec3 newP1 = { -halfW, 0.f, 0.f };
						CEVec3 newP2 = { halfW, 0.f, 0.f };

						UndoManager::GetInstance().AddCommand(
							std::make_unique<ChangeValueCommand<std::pair<CEVec3, CEVec3>>>(
								std::make_pair(oldP1, oldP2), std::make_pair(newP1, newP2),
								[sc](const std::pair<CEVec3, CEVec3>& v) { sc->p1 = v.first; sc->p2 = v.second; }
							)
						);
						sc->p1 = newP1; sc->p2 = newP2;
					}
				}
			}
		}
	};

	/*!
	@brief Adapter for editing Component::TextComponent.
	@details Edits type-specific parameters and supports a per-entity preview toggle.
	*/
	struct TextAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "TextComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::TextComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::TextComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::TextComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			auto* fc = ecs.GetComponent<TextComponent>(e);
			if (!fc) return;

			UIHelpers::DrawUndoableString("Font Name", fc->font);

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
					if (payload->Data && payload->DataSize == sizeof(AssetDragPayload)) {
						const auto* d = static_cast<const AssetDragPayload*>(payload->Data);

						// Verify it is a font by extension
						std::string p = d->relPath;
						std::string ext = std::filesystem::path(p).extension().string();
						std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

						if (ext == ".ttf" || ext == ".otf") {
							std::string oldVal = fc->font;
							// AssetManager keys fonts by their Stem (Name), not path.
							std::string newVal = d->name;

							fc->font = newVal;
							UndoManager::GetInstance().AddCommand(
								std::make_unique<ChangeValueCommand<std::string>>(oldVal, newVal, [fc](const std::string& v) { fc->font = v; })
							);
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			float btnSize = ImGui::GetFrameHeight();
			if (ImGui::Button("...", ImVec2(btnSize, btnSize))) {
				ImGui::OpenPopup("PickFontPopup");
			}

			if (ImGui::BeginPopupModal("PickFontPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				namespace fs = std::filesystem;
				static std::vector<std::string> s_fonts;
				static bool s_scanned = false;

				if (ImGui::Button("Refresh")) s_scanned = false;

				if (!s_scanned) {
					s_scanned = true;
					s_fonts.clear();
					// Assumes fonts are in assets/fonts/
					const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "fonts";

					try {
						if (fs::exists(assetRoot)) {
							for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
								if (!p.is_regular_file()) continue;
								std::string ext = p.path().extension().string();
								std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

								if (ext == ".ttf" || ext == ".otf") {
									// Store the STEM (filename without extension)
									s_fonts.push_back(p.path().stem().string());
								}
							}
							std::sort(s_fonts.begin(), s_fonts.end());
						}
					}
					catch (...) { /* ignore scan errors */ }
				}

				ImGui::Separator();
				ImGui::BeginChild("font_list", ImVec2(300, 300), true);
				for (const auto& f : s_fonts) {
					bool isSelected = (fc->font == f);
					if (ImGui::Selectable(f.c_str(), isSelected)) {
						std::string oldVal = fc->font;
						fc->font = f;
						UndoManager::GetInstance().AddCommand(
							std::make_unique<ChangeValueCommand<std::string>>(oldVal, fc->font, [fc](const std::string& v) { fc->font = v; })
						);
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndChild();

				ImGui::Separator();
				if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}

			UIHelpers::DrawUndoableString("Display Text", fc->text);
			UIHelpers::DrawUndoableBool("Centered", fc->centered);
			UIHelpers::DrawUndoableBool("World Space", fc->isWorldSpace);
			UIHelpers::DrawUndoableFloat("Scale", fc->scale);
			UIHelpers::DrawUndoableVec2("Offset", fc->offset);
			UIHelpers::DrawUndoableColor("Color", fc->color);
		}
	};

	/*!
	@brief Adapter for editing Component::AudioComponent.
	@details Edits type-specific parameters and supports a per-entity preview toggle.
	*/
	struct AudioAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "Audio"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::AudioEmitterComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::AudioEmitterComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			auto* c = ecs.GetComponent<Component::AudioEmitterComponent>(e);
			if (c) {
				// Stop ALL sounds before removing
				for (auto& clip : c->clips) {
					if (clip.inst) { clip.inst->stop(FMOD_STUDIO_STOP_IMMEDIATE); clip.inst->release(); }
					if (clip.channel) { clip.channel->stop(); }
				}
			}
			if (has(ecs, e)) ecs.RemoveComponent<Component::AudioEmitterComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			using Component::AudioEmitterComponent;
			using Component::AudioClip; // Important!

			auto* c = ecs.GetComponent<AudioEmitterComponent>(e);
			if (!c) return;

			// Button to Create New Clip 
			if (ImGui::Button("+ Add Audio Clip", ImVec2(-1, 0))) {
				c->clips.emplace_back(); // Add empty
				c->clips.back().name = "Sound " + std::to_string(c->clips.size());
			}

			ImGui::Separator();

			// Iterate Logic
			int indexToRemove = -1;

			for (int i = 0; i < c->clips.size(); ++i) {
				AudioClip& clip = c->clips[i];

				ImGui::PushID(i);

				bool open = ImGui::CollapsingHeader(clip.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::Selectable("Remove Clip")) indexToRemove = i;
					ImGui::EndPopup();
				}

				if (open) {
					ImGui::Indent();

					char nameBuf[64];
					strcpy_s(nameBuf, clip.name.c_str());
					if (ImGui::InputText("Name (Key)", nameBuf, sizeof(nameBuf))) {
						clip.name = nameBuf;
					}

					char pathBuf[256]{};
					if (clip.path.size() < sizeof(pathBuf)) std::memcpy(pathBuf, clip.path.data(), clip.path.size());

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
					if (ImGui::InputTextWithHint("##path", "assets/audio/...", pathBuf, sizeof(pathBuf))) {
						clip.path = pathBuf;
					}

					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
							if (payload->Data && payload->DataSize == sizeof(AssetDragPayload)) {
								const auto* d = static_cast<const AssetDragPayload*>(payload->Data);
								if (d->kind == AssetKind::Audio) {
									// Convert char array to path object first!
									std::filesystem::path p = d->relPath;
									std::string full = p.generic_string();

									if (full.find("assets/audio/") == 0) full = full.substr(13);
									clip.path = full;
								}
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine();
					if (ImGui::Button("...")) ImGui::OpenPopup("clip_select_popup");

					// Popup ID Conflict
					if (ImGui::BeginPopup("clip_select_popup")) {
						static std::vector<std::string> rawFiles;
						static bool s_audioScanned = false;

						// Add a refresh button (matches other adapters)
						if (ImGui::Button("Refresh List")) {
							s_audioScanned = false;
						}

						if (!s_audioScanned) {
							rawFiles.clear();
							Audio::AudioSystem::instance().listRawAudioFiles(rawFiles);

							std::sort(rawFiles.begin(), rawFiles.end());
							rawFiles.erase(std::unique(rawFiles.begin(), rawFiles.end()), rawFiles.end());

							s_audioScanned = true;
						}

						for (int j = 0; j < rawFiles.size(); ++j) {
							ImGui::PushID(j);
							if (ImGui::Selectable(rawFiles[j].c_str())) {
								clip.path = rawFiles[j];
								ImGui::CloseCurrentPopup();
							}
							ImGui::PopID();
						}
						ImGui::EndPopup();
					}

					ImGui::DragFloat("Volume", &clip.volume, 0.01f, 0.0f, 2.0f);
					ImGui::DragFloat("Pitch", &clip.pitch, 0.01f, 0.1f, 3.0f);
					ImGui::Checkbox("Looping", &clip.looping);
					ImGui::SameLine();
					ImGui::Checkbox("Is Music", &clip.isMusic);
					ImGui::SameLine();
					ImGui::Checkbox("3D Spatial", &clip.spatial3D);

					static float s_debugFadeTime = 0.5f;
					ImGui::DragFloat("Stop Fade (s)", &s_debugFadeTime, 0.1f, 0.0f, 5.0f);

					bool isPlaying = (clip.channel != nullptr);
					if (clip.channel) {
						bool p = false;
						clip.channel->isPlaying(&p);
						if (!p) clip.channel = nullptr;
					}

					if (ImGui::Button(isPlaying ? "Stop##test" : "Play##test")) {
						auto& audio = Audio::AudioSystem::instance();
						if (isPlaying) {
							if (clip.channel) audio.stopChannel(clip.channel, s_debugFadeTime);
							clip.channel = nullptr;
						}
						else {
							std::string p = clip.path;
							if (p.rfind("assets/", 0) != 0) p = "assets/audio/" + p;

							FMOD::Channel* ch = nullptr;
							if (clip.isMusic) ch = audio.playMusicFile(p, clip.looping);
							else ch = audio.playSFXFile(p);

							if (ch) {
								ch->setVolume(clip.volume);
								ch->setPitch(clip.pitch);
								ch->setMode(clip.looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
								clip.channel = ch;
							}
						}
					}

					if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
					{
						ImGui::BeginTooltip();

						ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Warning:");
						ImGui::SameLine();
						ImGui::Text("Sound will only play if the scene is Active! PLEASE MAKE SURE YOU SET THE CURRENT ENGINE STATE TO PLAY AT THE TOP!");

						ImGui::EndTooltip();
					}

					ImGui::Unindent();
				}
				ImGui::PopID();
			}

			if (indexToRemove != -1) {
				c->clips.erase(c->clips.begin() + indexToRemove);
			}
		}
	};

	/*!
	@brief Adapter for editing Component::ScriptComponent.
	@details Edits type-specific parameters and supports a per-entity preview toggle.
	*/
	struct ScriptAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "ScriptComponent"; }

		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<ScriptComponent>(e);
		}

		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) {
				static char className[64] = "PlayerController";
				ImGui::InputText("Class Name to Add", className, sizeof(className));
				ScriptComponent sc(className, e);
				ecs.AddComponent<ScriptComponent>(e, sc);
			}
		}

		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) {
				ecs.RemoveComponent<ScriptComponent>(e);
			}
		}

		void drawInspector(ECS& ecs, Entity e) override {
			auto* sc = ecs.GetComponent<ScriptComponent>(e);
			if (!sc)
				return;

			bool monoReady = (MonoManager::GetDomain() != nullptr &&
				MonoManager::GetAssembly() != nullptr);
			ImGui::TextDisabled("Mono: %s", monoReady ? "Ready" : "Not Ready");

			ImGui::SeparatorText("Scripts");

			// Button to add another script slot
			if (ImGui::Button("+ Add Script")) {
				sc->AddScript("NewScript");
			}

			// Draw each ScriptInstance
			for (size_t i = 0; i < sc->Scripts.size(); ++i) {
				auto& inst = sc->Scripts[i];
				ImGui::PushID(static_cast<int>(i));

				ImGui::Separator();
				ImGui::Text("Script %zu", i);

				// Editable class name per script
				char buf[128]{};
				if (inst.ClassName.size() < sizeof(buf))
					std::memcpy(buf, inst.ClassName.data(), inst.ClassName.size());

				static std::string s_lastScriptError; // error message storage
				std::string oldClass = inst.ClassName; // keep previous valid name

				if (ImGui::InputText("Class Name", buf, sizeof(buf))) {
					std::string newName = buf;

					// Validate using your new helper
					if (!newName.empty() &&
						!CrashoutsEngine::IsValidScriptBehaviourClass(newName))
					{
						s_lastScriptError =
							"The class '" + newName +
							"' is not a valid script.\n\n"
							"Scripts must contain [ScriptBehaviour] and match the filename.";

						// revert textbox value to old valid name
						std::strncpy(buf, oldClass.c_str(), sizeof(buf) - 1);
						buf[sizeof(buf) - 1] = '\0';

						ImGui::OpenPopup("Script Error");
					}
					else {
						// Accept new name
						inst.ClassName = newName;

						// Reinitialize if valid and Mono is ready
						if (!inst.ClassName.empty() && monoReady) {
							sc->Destroy();
							sc->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
						}
					}
				}

				// Also accept drag-dropped script class names
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT/CLASS")) {
						const char* dropped = static_cast<const char*>(payload->Data);
						if (dropped && dropped[0] != '\0') {
							inst.ClassName = dropped;
							if (!inst.ClassName.empty() && monoReady) {
								sc->Destroy();
								sc->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::SmallButton("...")) {
					ImGui::OpenPopup("Pick Script Class");
				}

				if (ImGui::BeginPopupModal("Pick Script Class", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					namespace fs = std::filesystem;
					static char filter[128] = {};
					ImGui::InputTextWithHint("##script_filter", "filter...", filter, sizeof(filter));
					ImGui::Separator();

					fs::path base = "../../projects/ManagedScripts";

					if (ImGui::BeginChild("script_list", ImVec2(320, 240), true)) {
						try {
							for (auto it = fs::recursive_directory_iterator(base, fs::directory_options::skip_permission_denied);
								it != fs::recursive_directory_iterator(); ++it) {

								if (it->is_directory()) {
									auto folder = it->path().filename().string();
									std::transform(folder.begin(), folder.end(), folder.begin(), ::tolower);
									if (folder == "obj" || folder == "bin" || folder == ".vs") {
										it.disable_recursion_pending();
									}
									continue;
								}

								if (!it->is_regular_file()) continue;

								auto ext = it->path().extension().string();
								std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
								if (ext != ".cs") continue;

								// THIS ENTIRE BLOCK ENSURES ONLY SCRIPTS WITH [ScriptBehaviour] ARE SHOWN
								// SO HOPEFULLY PEOPLE DON'T TRY TO ADD NON-SCRIPT CLASSES
								// IF NOT I'LL MENGGONNA DO SOMETHING LATER
								{
									std::ifstream in(it->path());
									if (in) {
										std::string line;
										bool hasAttribute = false;
										while (std::getline(in, line)) {
											if (line.find("[ScriptBehaviour]") != std::string::npos) {
												hasAttribute = true;
												break;
											}
										}
										if (!hasAttribute)
											continue;
									}
								}

								std::string stem = it->path().stem().string();
								if (filter[0] && stem.find(filter) == std::string::npos)
									continue;

								bool isCurrent = (inst.ClassName == stem);

								if (ImGui::Selectable(stem.c_str(), isCurrent)) {
									inst.ClassName = stem;
									if (!inst.ClassName.empty() && monoReady) {
										sc->Destroy();
										sc->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
									}
									ImGui::CloseCurrentPopup();
								}

								if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
									ImGui::SetDragDropPayload("SCRIPT/CLASS",
										stem.c_str(),
										static_cast<size_t>(stem.size() + 1));
									ImGui::TextUnformatted(stem.c_str());
									ImGui::EndDragDropSource();
								}
							}
						}
						catch (...) {
							// swallow
						}

						ImGui::EndChild();

						if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (ImGui::SmallButton("Open")) {
					namespace fs = std::filesystem;
					fs::path base = "../../projects/ManagedScripts";
					fs::path file = base / (inst.ClassName + ".cs");

					if (fs::exists(file)) {
						std::string cmd = "start \"\" \"" + file.string() + "\"";
						system(cmd.c_str());
					}
					else {
						std::cout << "Script not found: " << file << std::endl;
					}
				}

				// Remove this script instance
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove")) {
					sc->RemoveScript(i);
					ImGui::PopID();
					break; // list changed
				}

				// --- MONO REFLECTION UNDO ---
				if (inst.GCHandle && inst.Class) {
					MonoObject* instance = mono_gchandle_get_target(inst.GCHandle);
					if (!instance) continue;

					ImGui::Indent();
					void* iter = nullptr;
					while (MonoClassField* field = mono_class_get_fields(inst.Class, &iter)) {
						const char* fName = mono_field_get_name(field);
						uint32_t flags = mono_field_get_flags(field);

						if (flags & MONO_FIELD_ATTR_STATIC) continue;

						bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
						bool hasAttr = false;
						static MonoClass* attrClass = mono_class_from_name(MonoManager::GetImage(), "CrashoutsEngine", "SerializeField");
						if (attrClass) {
							MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(inst.Class, field);
							if (attrInfo) {
								hasAttr = mono_custom_attrs_has_attr(attrInfo, attrClass);
								mono_custom_attrs_free(attrInfo);
							}
						}

						if (isPublic || hasAttr) {
							MonoType* type = mono_field_get_type(field);
							int typeEnum = mono_type_get_type(type);

							if (typeEnum == MONO_TYPE_R4) { // Float
								float val = 0.f;
								mono_field_get_value(instance, field, &val);
								UIHelpers::DrawUndoableFloat(fName, val);
								mono_field_set_value(instance, field, &val);
							}
							else if (typeEnum == MONO_TYPE_I4) { // Int
								int val = 0;
								mono_field_get_value(instance, field, &val);
								UIHelpers::DrawUndoableInt(fName, val);
								mono_field_set_value(instance, field, &val);
							}
							else if (typeEnum == MONO_TYPE_BOOLEAN) { // Bool
								bool val = false;
								mono_field_get_value(instance, field, &val);
								UIHelpers::DrawUndoableBool(fName, val);
								mono_field_set_value(instance, field, &val);
							}
							else if (typeEnum == MONO_TYPE_STRING) { // String
								MonoString* monoStr = nullptr;
								mono_field_get_value(instance, field, &monoStr);

								std::string strVal = "";
								if (monoStr) {
									char* raw = mono_string_to_utf8(monoStr);
									strVal = raw;
									mono_free(raw);
								}

								std::string oldVal = strVal;
								UIHelpers::DrawUndoableString(fName, strVal);

								if (strVal != oldVal) {
									MonoString* newMonoStr = mono_string_new(MonoManager::GetDomain(), strVal.c_str());
									mono_field_set_value(instance, field, newMonoStr);
								}
							}
						}
					}
					ImGui::Unindent();
				}
				ImGui::PopID();
			}

			ImGui::Separator();

			if (ImGui::SmallButton("New C# Script")) {
				namespace fs = std::filesystem;
				fs::path base = "../../projects/ManagedScripts";

				std::string filename = "NewScript";
				int counter = 1;
				fs::path file = base / (filename + ".cs");
				while (fs::exists(file)) {
					file = base / (filename + std::to_string(counter++) + ".cs");
				}

				// write a basic template
				std::ofstream out(file);
				out <<
					R"(using CrashoutsEngine;

namespace CrashoutsEngine
{
    // Attach this script to an Entity through the ScriptComponent UI.
    // Scripts with [ScriptBehaviour] will appear in the editor's script list.
    [ScriptBehaviour]
    public class )" << file.stem().string() << R"(
    {
        // Reference to the entity this script instance is attached to.
        // Automatically assigned by the engine before Start() runs.
        public Entity Entity;

        // Called once when the script is first created.
        // Initialize references or setup values here.
        public void Start() { }

        // Called every frame.
        // Main game logic, movement, inputs, and updates go here.
        public void Update() { }

        // Called at a fixed timestep (ideal for physics logic).
        // Runs independently from the frame rate.
        public void FixedUpdate() { }
    }
}
)";
				out.close();

				// automatically add this new script
				sc->AddScript(file.stem().string());
			}

			// --- Mono reload / manual Start/Update/Destroy ---
			if (!monoReady) ImGui::BeginDisabled();

			static bool isBuilding = false;
			static std::atomic<bool> reloadPending = false;

			if (ImGui::Button("Reload Scripts"))
			{
				if (!isBuilding)
				{
					isBuilding = true;
					std::thread([]() {
						int result = system(
							"dotnet clean \"../../projects/ManagedScripts/ManagedScripts.csproj\" -c Debug > buildlog.txt 2>&1 && "
							"dotnet build \"../../projects/ManagedScripts/ManagedScripts.csproj\" -c Debug > buildlog.txt 2>&1"
						);
						if (result == 0) {
							reloadPending = true; // signal reload

						}
						isBuilding = false;
						}).detach();
				}
			}
			if (reloadPending)
			{
				reloadPending = false;
				MonoManager::ReloadScripts(); // called on main thread
			}

			if (ImGui::Button("Start All"))
				sc->Start();

			ImGui::SameLine();
			if (ImGui::Button("Update All"))
				sc->Update();

			ImGui::SameLine();
			if (ImGui::Button("Destroy All"))
				sc->Destroy();

			if (!monoReady)
				ImGui::EndDisabled();
		}
	};

	/*!
	@brief Adapter for editing Component::CameraComponent.
	@details Uses ImGui drag controls and writes values back to the ECS each frame.
	*/
	struct CameraAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "CameraComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::CameraComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::CameraComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::CameraComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			auto* cc = ecs.GetComponent<CameraComponent>(e);
			if (!cc) return;

			UIHelpers::DrawUndoableFloat("Zoom", cc->zoom, 0.005f, 0.05f);
			UIHelpers::DrawUndoableVec2("X Bounds", cc->xBounds);
			UIHelpers::DrawUndoableVec2("Y Bounds", cc->yBounds);

		}
	};

	/*!
	@brief Adapter for editing Component::ButtonComponent.
	@details Uses ImGui drag controls and writes values back to the ECS each frame.
	*/
	struct ButtonAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "ButtonComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::ButtonComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::ButtonComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::ButtonComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			using namespace Component;
			auto* bc = ecs.GetComponent<ButtonComponent>(e);
			if (!bc) return; // Corrected from 'cc' to 'bc'

			// --- Read-Only State Display ---
			ImGui::SeparatorText("Button State");
			const char* stateStr = "NORMAL";
			if (bc->buttonState == ButtonState::HOVERED) { stateStr = "HOVERED"; }
			else if (bc->buttonState == ButtonState::PRESSED) { stateStr = "PRESSED"; }

			ImGui::Text("Current State:");
			ImGui::SameLine();
			ImGui::TextDisabled("%s", stateStr); // Show current state for debugging

			// --- Shape Colors ---
			ImGui::SeparatorText("Shape Colors");

			// Helper function to convert 0-255 CEVec3 to 0-1 float[3] for ImGui's picker
			auto EditColor = [](const char* label, CEColor& colorVec) {
				float col[4] = { colorVec.x(), colorVec.y(), colorVec.z(), colorVec.w() };
				if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaBar)) {
					colorVec.x() = col[0];
					colorVec.y() = col[1];
					colorVec.z() = col[2];
					colorVec.w() = col[3];
				}
				};

			ImGui::PushID("shape_colors"); // Use PushID to avoid label conflicts
			EditColor("Normal", bc->shapeNormalColor);
			EditColor("Hovered", bc->shapeHoveredColor);
			EditColor("Pressed", bc->shapePressedColor);
			ImGui::PopID();

			// --- Text Colors ---
			ImGui::SeparatorText("Text Colors");
			ImGui::PushID("text_colors");
			EditColor("Normal", bc->textNormalColor);
			EditColor("Hovered", bc->textHoveredColor);
			EditColor("Pressed", bc->textPressedColor);
			ImGui::PopID();
		}
	};

	/*!
	@brief Adapter for editing Component::UILayoutComponent.
	@details Uses ImGui drag controls and writes values back to the ECS each frame.
	*/
	struct UILayoutAdapater : DebugReflection::IComponentAdapter {
		const char* name() const override { return "UILayoutComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::UILayoutComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::UILayoutComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::UILayoutComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			using namespace Component;
			auto* uc = ecs.GetComponent<UILayoutComponent>(e);
			if (!uc) return;

			ImGui::SeparatorText("UI Layout");

			// --- 1. Edit paddingXY ---
			ImGui::DragFloat2("Padding (X, Y)", &uc->paddingXY.x, 0.5f, 0.0f, 1000.0f, "%.1f");

			// --- 2. Edit uiLayout (the flags) ---
			ImGui::PushID("UILayoutFlags");

			unsigned int tempFlags = static_cast<unsigned int>(uc->uiLayout);
			unsigned int flags_before = tempFlags;

			// --- Vertical Alignment (Mutually Exclusive) ---
			ImGui::SeparatorText("Vertical Alignment");

			// Isolate the current vertical choice
			unsigned int vertChoice = tempFlags & (static_cast<unsigned int>(UILayout::TOP) |
				static_cast<unsigned int>(UILayout::DOWN) |
				static_cast<unsigned int>(UILayout::CENTER));

			// Default to CENTER if no vertical flag is set (i.e., it's ColliderType::NONE)
			if (vertChoice == 0) {
				vertChoice = static_cast<unsigned int>(UILayout::CENTER);
			}

			// Use RadioButtons for an "either/or" selection
			if (ImGui::RadioButton("Top", vertChoice & static_cast<unsigned int>(UILayout::TOP))) { vertChoice = static_cast<unsigned int>(UILayout::TOP); }
			ImGui::SameLine();
			if (ImGui::RadioButton("Center##Vertical", vertChoice & static_cast<unsigned int>(UILayout::CENTER))) { vertChoice = static_cast<unsigned int>(UILayout::CENTER); }
			ImGui::SameLine();
			if (ImGui::RadioButton("Down", vertChoice & static_cast<unsigned int>(UILayout::DOWN))) { vertChoice = static_cast<unsigned int>(UILayout::DOWN); }

			// --- Horizontal Alignment (Mutually Exclusive) ---
			ImGui::SeparatorText("Horizontal Alignment");

			// Isolate the current horizontal choice
			unsigned int horizChoice = tempFlags & (static_cast<unsigned int>(UILayout::LEFT) |
				static_cast<unsigned int>(UILayout::RIGHT));

			// Note: 0 (UILayout::ColliderType::NONE) is our horizontal center.
			bool isLeft = (horizChoice & static_cast<unsigned int>(UILayout::LEFT));
			bool isRight = (horizChoice & static_cast<unsigned int>(UILayout::RIGHT));
			bool isCenter = (horizChoice == 0);

			// Use RadioButtons again
			if (ImGui::RadioButton("Left", isLeft)) { horizChoice = static_cast<unsigned int>(UILayout::LEFT); }
			ImGui::SameLine();
			if (ImGui::RadioButton("Center##Horizontal", isCenter)) { horizChoice = 0; } // 0 (ColliderType::NONE) is our horizontal center
			ImGui::SameLine();
			if (ImGui::RadioButton("Right", isRight)) { horizChoice = static_cast<unsigned int>(UILayout::RIGHT); }

			// --- 3. Recombine flags ---
			// Clear all *old* alignment flags
			tempFlags &= ~(static_cast<unsigned int>(UILayout::TOP) | static_cast<unsigned int>(UILayout::DOWN) | static_cast<unsigned int>(UILayout::CENTER) |
				static_cast<unsigned int>(UILayout::LEFT) | static_cast<unsigned int>(UILayout::RIGHT));

			// Add the new choices
			tempFlags |= vertChoice;
			tempFlags |= horizChoice;

			ImGui::PopID();

			// --- 4. Write changes back to component ---
			if (tempFlags != flags_before) {
				uc->uiLayout = static_cast<UILayout>(tempFlags);
			}
		}
	};

	/*!
	@brief Adapter for editing Component::TextInputComponent.
	@details Uses ImGui drag controls and writes values back to the ECS each frame.
	*/
	struct TextInputAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "TextInputComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::TextInputComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::TextInputComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::TextInputComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			using namespace Component;
			// 1. Corrected this line to get the right component
			auto* tc = ecs.GetComponent<TextInputComponent>(e);

			if (!tc) return;

			ImGui::SeparatorText("Text Input");

			// 2. Added correct ImGui widgets for your component's members

			// isFocused should be read-only, as it's set by the system
			ImGui::Text("Is Focused:");
			ImGui::SameLine();
			ImGui::TextDisabled("%s", tc->isFocused ? "Yes" : "No");

			// Use DragInt for size_t (cast to int for ImGui)
			// Note: ImGui doesn't have a native DragU64 for size_t.
			// We'll use an int and cast, which is fine for these values.
			int maxChars = static_cast<int>(tc->maxCharacters);
			if (ImGui::DragInt("Max Characters", &maxChars, 1.0f, 1, 1024)) {
				if (maxChars < 1) maxChars = 1; // Enforce minimum
				tc->maxCharacters = static_cast<size_t>(maxChars);
			}

			// Show cursor position as read-only
			int cursorPos = static_cast<int>(tc->cursorPos);
			ImGui::BeginDisabled(); // Make read-only
			ImGui::DragInt("Cursor Position", &cursorPos);
			ImGui::EndDisabled();
		}
	};


	// -------------------------- HELPER ADAPTERS ---------------------------------
	static ButtonAdapter gButtonAdapter_helper;
	static TextInputAdapter gTextInputAdapter_helper;
	static UILayoutAdapater gUILayoutAdapter_helper;

	/*!
	@brief Adapter for editing Component::GuiComponent.
	@details Uses ImGui drag controls and writes values back to the ECS each frame.
	*/
	struct GuiAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "GuiComponent"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<GuiComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<GuiComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<GuiComponent>(e);

			// When the controller is removed, also clean up all UI components
			// (This is optional but good practice)
			if (ecs.HasComponent<ButtonComponent>(e)) ecs.RemoveComponent<ButtonComponent>(e);
			if (ecs.HasComponent<TextInputComponent>(e)) ecs.RemoveComponent<TextInputComponent>(e);
			if (ecs.HasComponent<UILayoutComponent>(e)) ecs.RemoveComponent<UILayoutComponent>(e);
			// Note: We might leave ShapeComponent and TextComponent on purpose
		}

		void drawInspector(ECS& ecs, Entity e) override {
			using namespace Component;

			auto* gc = ecs.GetComponent<GuiComponent>(e);
			if (!gc) return;

			ImGui::SeparatorText("GUI Type Flags");
			ImGui::PushID("GuiAdapter"); // Avoid ID conflicts

			// 1. --- PREPARE A RAW UINT FOR IMGUI ---
			// ImGui::CheckboxFlags needs an 'unsigned int*', not a 'UIType*'.
			// We create a temporary uint, let ImGui modify it, then cast it back.
			unsigned int tempFlags = static_cast<unsigned int>(gc->uiType);

			// Store the state *before* ImGui modifies it so we can detect changes
			unsigned int type_before = tempFlags;

			// 2. --- DRAW THE CHECKBOXES ---
			// We pass our temporary 'tempFlags' to ImGui.
			// We also must cast the flag values themselves to unsigned int.
			ImGui::CheckboxFlags("Image (Adds Shape)", &tempFlags, static_cast<unsigned int>(UIType::IMAGE));
			ImGui::CheckboxFlags("Text (Adds Text)", &tempFlags, static_cast<unsigned int>(UIType::TEXT));
			ImGui::CheckboxFlags("Button (Adds Button)", &tempFlags, static_cast<unsigned int>(UIType::BUTTON));
			ImGui::CheckboxFlags("Input (Adds Input)", &tempFlags, static_cast<unsigned int>(UIType::INPUTTEXT));

			ImGui::PopID();

			// 3. --- Only run the Sync (creation/deletion) logic *on change* ---
			if (tempFlags != type_before) {
				// A change was made! Update the real component.
				gc->uiType = static_cast<UIType>(tempFlags);

				// Define our rules:
				bool needsButton = (gc->uiType & UIType::BUTTON) != UIType::NONE;
				bool needsInput = (gc->uiType & UIType::INPUTTEXT) != UIType::NONE;
				bool needsText = needsInput || ((gc->uiType & UIType::TEXT) != UIType::NONE);
				bool needsShape = needsText || ((gc->uiType & UIType::IMAGE) != UIType::NONE);
				bool needsLayout = (gc->uiType != UIType::NONE); // adds UILayout if *any* flag is set

				// Helper lambda to add/remove components
				auto Sync = [&]<typename T>(bool shouldHave) {
					bool has = ecs.HasComponent<T>(e);
					if (shouldHave && !has) ecs.AddComponent<T>(e);
					else if (!shouldHave && has) ecs.RemoveComponent<T>(e);
				};

				// Run the sync! This creates/removes the components
				Sync.operator() < ButtonComponent > (needsButton);
				Sync.operator() < TextInputComponent > (needsInput);
				Sync.operator() < TextComponent > (needsText);
				Sync.operator() < ShapeComponent > (needsShape);
				Sync.operator() < UILayoutComponent > (needsLayout);
			}

			// 4. --- MANUALLY DRAW CHILD INSPECTORS (EVERY FRAME) ---
			// This part now runs every frame, so the inspectors stay visible
			// as long as their corresponding flags are set.

			// We re-calculate the 'needs' booleans here so they are
			// correct even when no change was made this frame.
			bool needsButton = (gc->uiType & UIType::BUTTON) != UIType::NONE;
			bool needsInput = (gc->uiType & UIType::INPUTTEXT) != UIType::NONE;
			bool needsLayout = (gc->uiType & UIType::TEXT) != UIType::NONE;

			if (needsButton) {
				gButtonAdapter_helper.drawInspector(ecs, e);
			}
			if (needsLayout) {
				gUILayoutAdapter_helper.drawInspector(ecs, e);
			}
			if (needsInput) {
				gTextInputAdapter_helper.drawInspector(ecs, e);
			}
		}
	};

	/*!
	 @brief Adapter for editing Component::PPComponent.
	 @details Uses ImGui controls and writes values back to the ECS each frame.
	 */
	struct PPAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "Post-Process Component"; }
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::PPComponent>(e);
		}
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::PPComponent>(e);
		}
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::PPComponent>(e);
		}
		void drawInspector(ECS& ecs, Entity e) override {
			auto* ppc = ecs.GetComponent<PPComponent>(e);
			if (!ppc) return;

			UIHelpers::DrawUndoableBool("Active", ppc->active);
			UIHelpers::DrawUndoableFloat("Brightness", ppc->brightness, 0.01f, -1.0f, 1.0f);
			UIHelpers::DrawUndoableFloat("Contrast", ppc->contrast, 0.01f, -1.0f, 1.0f);
			UIHelpers::DrawUndoableFloat("Saturation", ppc->saturation, 0.01f, -1.0f, 1.0f);

			if (ImGui::Button("Reset")) {
				ppc->brightness = 0.0f; ppc->contrast = 0.0f; ppc->saturation = 0.0f;
			}
		}
	};

	/*!
	@brief Function used to draw the imgui bitmask selector.
	@param mask The bit mask to modify.
	*/
	static void DrawBitmaskSelector(unsigned int& mask) {
		// Map the visual 3x3 grid to your specific Enum Bits
		// -1 means "No Checkbox" (Corners/Center)

		// Enum: Up=0, Right=1, Down=2, Left=3
		int bits[3][3] = {
			{ -1,  0, -1 }, // Top Row:   Empty, UP(Bit 0), Empty
			{  3, -1,  1 }, // Mid Row:   LEFT(Bit 3), Center, RIGHT(Bit 1)
			{ -1,  2, -1 }  // Bot Row:   Empty, DOWN(Bit 2), Empty
		};

		ImGui::BeginGroup();

		float cellSize = 25.0f;
		float startX = ImGui::GetCursorPosX();

		for (int r = 0; r < 3; ++r) {
			for (int c = 0; c < 3; ++c) {
				int bitIndex = bits[r][c];

				if (c > 0) ImGui::SameLine();
				ImGui::SetCursorPosX(startX + (c * cellSize));

				if (bitIndex == -1) {
					// Placeholder for corners/center
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.1f);
					ImGui::Dummy(ImVec2(20, 20));
					ImGui::PopStyleVar();
				}
				else {
					bool val = (mask & (1 << bitIndex)) != 0;
					std::string id = "##bit" + std::to_string(bitIndex);

					ImGui::PushID(bitIndex);
					if (ImGui::Checkbox(id.c_str(), &val)) {
						if (val) mask |= (1 << bitIndex);
						else     mask &= ~(1 << bitIndex);
					}
					ImGui::PopID();
				}
			}
		}
		ImGui::EndGroup();
	}

	/*!
	@brief Adapter for editing Component::TileMapComponent.
	@details Uses ImGui controls to manage Tile Map assets and editor state.
	*/
	struct TileMapAdapter : DebugReflection::IComponentAdapter {

		// 1. Update Name
		const char* name() const override { return "Tile Map Component"; }

		// 2. Update 'Has' Check
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::TileMapComponent>(e);
		}

		// 3. Update 'Add' Logic
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) ecs.AddComponent<Component::TileMapComponent>(e);
		}

		// 4. Update 'Remove' Logic
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::TileMapComponent>(e);
		}

		// 5. The Main Inspector UI
		void drawInspector(ECS& ecs, Entity e) override {
			using namespace Component;
			using namespace CrashoutsEngine::Assets;
			namespace fs = std::filesystem;

			// Get the specific component
			TileMapComponent* tm = ecs.GetComponent<TileMapComponent>(e);
			if (!tm) return;

			ImGui::SeparatorText("Tile Map Settings");

			// ---------------------------------------------------------
			// SECTION 1: General Settings
			// ---------------------------------------------------------

			// Tile Size
			if (ImGui::InputInt("Tile Size (Px)", &tm->tileSize)) {
				if (tm->tileSize < 1) tm->tileSize = 1;
				tm->isCacheDirty = true; // Redraw grid lines if size changes
			}

			// Open Editor Toggle
			if (ImGui::Checkbox("Open Tile Palette", &tm->pOpen)) {
				// Logic if needed when opening/closing
			}

			ImGui::Separator();
			ImGui::Text("Loaded Rule Sets:");

			// ---------------------------------------------------------
			// SECTION 2: Asset Management (List of loaded Rule Files)
			// ---------------------------------------------------------

			// List existing assets
			int indexToRemove = -1;
			static std::string assetToEdit = "";
			bool openRulesPopup = false;

			for (int i = 0; i < tm->tileAssetNames.size(); ++i) {
				ImGui::PushID(i);

				// Display Name
				std::string displayName = std::filesystem::path(tm->tileAssetNames[i]).filename().string();
				ImGui::Bullet();
				ImGui::Text("%s", displayName.c_str());

				ImGui::SameLine();

				// Edit Button
				if (ImGui::SmallButton("Edit Rules")) {
					assetToEdit = tm->tileAssetNames[i];
					openRulesPopup = true;
				}

				ImGui::SameLine();

				// Remove Button
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove")) {
					indexToRemove = i;
				}

				ImGui::PopID();
			}
			if (openRulesPopup) ImGui::OpenPopup("RuleSetEditorPopup");

			// Handle Removal
			if (indexToRemove != -1) {
				// Check if the active asset is the one being removed
				if (tm->activeAsset == tm->tileAssetNames[indexToRemove]) {
					tm->activeAsset = ""; // Empty string is safer than "none"
				}
				tm->tileAssetNames.erase(tm->tileAssetNames.begin() + indexToRemove);
				tm->isCacheDirty = true;
			}

			ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
			if (ImGui::BeginPopupModal("RuleSetEditorPopup", nullptr, ImGuiWindowFlags_MenuBar)) {
				static int ruleIndexToEdit = -1;
				bool openAssetPopup = false;
				// 1. Get the actual asset from memory

				std::string loadName = "tilemaps/" + assetToEdit;
				TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(loadName);

				if (!asset) {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Asset not loaded!");
					if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
				}
				else {
					if (ImGui::BeginMenuBar()) {
						if (ImGui::MenuItem("Save & Close")) {
							asset->SaveTileMaps(); // Save to disk
							tm->isCacheDirty = true; // Rebuild render cache
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndMenuBar();
					}

					ImGui::Text("Editing: %s", assetToEdit.c_str());
					ImGui::Separator();

					// Button to add a new empty rule
					if (ImGui::Button("+ Add New Rule")) {
						RuleTile newTile;
						newTile.texAssetName = "default.png"; // Placeholder
						asset->GetTilesMutable().push_back(newTile);
					}

					ImGui::Separator();

					// Scrollable list of rules
					ImGui::BeginChild("RulesList", ImVec2(0, 0), true);

					// Layer option
					ImGui::Text("Layer Filter:");
					DrawLayerMultiSelect(ecs, asset->GetLayerMaskMutable());

					auto& rules = asset->GetTilesMutable();
					int ruleToDelete = -1;

					for (int i = 0; i < rules.size(); ++i) {
						RuleTile& rule = rules[i];
						ImGui::PushID(i);

						if (ImGui::CollapsingHeader(("Rule " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

							// A. Texture Name
							ImGui::Text("Texture: %s", rule.texAssetName.c_str());
							ImGui::SameLine();

							// Unique ID for this button so it doesn't conflict
							std::string btnId = "Choose...##" + std::to_string(i);
							if (ImGui::SmallButton(btnId.c_str())) {
								// Open the SHARED popup
								openAssetPopup = true;

								ruleIndexToEdit = i;
							}

							// B. Collider Type
							const char* colliderNames[] = { "Circle", "AABB", "OBB", "Triangle", "Polygon", "None" };
							int currentItem = (int)rule.colliderType;
							if (ImGui::Combo("Collider", &currentItem, colliderNames, 6)) {
								rule.colliderType = (Collision::ColliderType)currentItem;
							}

							// C. Bitmask (The Grid)
							ImGui::Text("Neighbours:");
							DrawBitmaskSelector(rule.ruleBitMap);

							// D. Hex Preview
							ImGui::SameLine();
							ImGui::BeginGroup();
							ImGui::TextDisabled("Hex: 0x%02X", rule.ruleBitMap);
							if (ImGui::Button("Delete Rule")) ruleToDelete = i;
							ImGui::EndGroup();
						}
						ImGui::PopID();
					}

					if (ruleToDelete != -1) {
						asset->RemoveTileDefinition(ruleToDelete);
					}

					ImGui::EndChild();
					if (openAssetPopup) ImGui::OpenPopup("PickRuleTexturePopup");

					// SHARED TEXTURE PICKER POPUP
					if (ImGui::BeginPopupModal("PickRuleTexturePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
						namespace fs = std::filesystem;
						static std::vector<std::string> s_assets;
						static bool s_scanned = false;

						// Refresh List button
						if (ImGui::Button("Refresh List")) {
							s_scanned = false;
						}

						if (!s_scanned) {
							s_scanned = true;
							s_assets.clear();

							const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "images";
							if (fs::exists(assetRoot)) {
								for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
									if (!p.is_regular_file()) continue;

									const std::string ext = p.path().extension().string();
									std::string lower = ext;
									std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
									if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
										lower == ".tga" || lower == ".bmp") {

										std::string rel = fs::relative(p.path(), assetRoot).string();
										s_assets.push_back(rel);
									}
								}
								std::sort(s_assets.begin(), s_assets.end());
							}
						}

						ImGui::Separator();

						// Asset List
						ImGui::BeginChild("asset_list", ImVec2(420, 300), true);
						for (const auto& f : s_assets) {
							if (ImGui::Selectable(f.c_str())) {
								// Apply selection using the static index
								if (ruleIndexToEdit >= 0 && ruleIndexToEdit < rules.size()) {
									rules[ruleIndexToEdit].texAssetName = f;
								}
								ImGui::CloseCurrentPopup();
							}
						}
						ImGui::EndChild();

						if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
						ImGui::EndPopup();
					}
				}
				ImGui::EndPopup();
			}

			// ---------------------------------------------------------
			// SECTION 3: Add New Asset
			// ---------------------------------------------------------

			if (ImGui::Button("Add Rule Set...")) {
				ImGui::OpenPopup("AddRuleAssetPopup");
			}

			if (ImGui::BeginPopup("AddRuleAssetPopup")) {
				namespace fs = std::filesystem;
				std::string dirPath = CrashoutsEngine::Assets::rootPath + "tilemaps/";

				// --- SECTION 1: CREATE NEW ASSET ---
				static char newAssetName[64] = "";
				ImGui::InputTextWithHint("##NewAsset", "New Asset Name", newAssetName, IM_ARRAYSIZE(newAssetName));
				ImGui::SameLine();

				// Logic to Create if it doesn't exist
				if (ImGui::Button("Create")) {
					std::string rawName = newAssetName;
					if (!rawName.empty()) {
						// 2. Check if it already exists in the component list
						bool alreadyHas = false;
						for (const auto& s : tm->tileAssetNames) if (s == rawName) alreadyHas = true;

						if (!alreadyHas) {
							// 3. Create the physical file and Asset Object
							// (Your TileMapAsset::Create function handles file creation if missing)
							auto newAsset = Assets::TileMapAsset::Create(rawName);

							if (newAsset) {
								// 4. Register to AssetManager
								std::string loadName = "tilemaps/" + rawName;
								Assets::AssetManager::GetInstance().Load(loadName, std::move(newAsset));

								// 5. Add to Component
								tm->tileAssetNames.push_back(rawName);
								tm->isCacheDirty = true;

								// Clear input
								newAssetName[0] = '\0';
								ImGui::CloseCurrentPopup();
							}
						}
					}
				}

				ImGui::Separator();
				ImGui::TextDisabled("Select Existing:");

				// --- SECTION 2: SELECT EXISTING ---
				if (fs::exists(dirPath)) {
					for (const auto& entry : fs::directory_iterator(dirPath)) {
						if (!entry.is_regular_file()) continue;

						std::string filename = entry.path().filename().string();

						// Check if already added to avoid duplicates in the list
						bool alreadyHas = false;
						for (const auto& s : tm->tileAssetNames) if (s == filename) alreadyHas = true;

						if (!alreadyHas) {
							if (ImGui::Selectable(filename.c_str())) {

								// Ensure it's loaded in memory
								// FIX: Use the Factory method, don't move an empty unique_ptr
								auto loadedAsset = Assets::TileMapAsset::Create(filename);
								if (loadedAsset) {
									std::string loadName = "tilemaps/" + filename;

									Assets::AssetManager::GetInstance().Load(loadName, std::move(loadedAsset));
									tm->tileAssetNames.push_back(filename);
								}

								tm->isCacheDirty = true;
							}
						}
					}
				}
				else {
					ImGui::Text("No tilemaps folder found!");
				}
				ImGui::EndPopup();
			}

			// ---------------------------------------------------------
			// SECTION 4: Debug Stats
			// ---------------------------------------------------------
			ImGui::Separator();
			ImGui::TextDisabled("Stats:");
			ImGui::TextDisabled("Tiles Painted: %zu", tm->gridData.size());
			ImGui::TextDisabled("Active Brush: %s", tm->activeAsset.c_str());

			if (ImGui::Button("Force Rebuild Cache")) {
				tm->isCacheDirty = true;
			}

			if (ImGui::Button("Clear All Tiles")) {
				tm->gridData.clear();
				tm->isCacheDirty = true;
			}
		}
	};

	/*!
	@brief Adapter for editing Component::KeyframeAnimatorComponent.
	@details Uses ImGui controls to manage keyframe animations.
	*/
	struct KeyframeAnimatorAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "Animation (Keyframe)"; }

		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<KeyframeAnimatorComponent>(e);
		}

		void add(ECS& ecs, Entity e) override {
			using namespace KeyframeUnity;
			// Just ensure the component exists. Don't add sample data automatically.
			// Let the user add clips via code or loading.
			AddAnimator(ecs, e);
		}

		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e))
				ecs.RemoveComponent<KeyframeAnimatorComponent>(e);
		}

		void drawInspector(ECS& ecs, Entity e) override {
			using namespace KeyframeUnity;

			auto* anim = ecs.GetComponent<KeyframeAnimatorComponent>(e);
			if (!anim) return;

			// --- 1. Playback Controls ---
			ImGui::SeparatorText("Playback State");

			// Current Clip Name (Read-only or Dropdown)
			if (anim->clips.empty()) {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Clips Loaded!");
			}
			else {
				if (ImGui::BeginCombo("Current Clip", anim->current.c_str())) {
					for (const auto& pair : anim->clips) {
						bool isSelected = (anim->current == pair.first);
						if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
							Play(*anim, pair.first, true); // Restart on select
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			// Progress Bar / Time Slider
			float currentLen = 0.0f;
			if (!anim->current.empty() && anim->clips.count(anim->current)) {
				currentLen = anim->clips.at(anim->current).Length();
			}

			if (currentLen > 0.0f) {
				ImGui::Text("Time: %.2fs / %.2fs", anim->t, currentLen);
				ImGui::ProgressBar(anim->t / currentLen, ImVec2(-1, 0));

				// Manual Slider (for debugging specific frames)
				if (ImGui::SliderFloat("Seek", &anim->t, 0.0f, currentLen)) {
					anim->playing = false; // Pause if dragging
					// Force Update visual immediately (optional)
					// KeyframeUnity::Update(ecs, e, *anim, 0.0f); 
				}
			}

			// Buttons
			if (ImGui::Button(anim->playing ? "Pause" : "Play")) {
				anim->playing = !anim->playing;
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				anim->playing = false;
				anim->t = 0.f;
			}

			// --- 2. Clip List (Details) ---
			ImGui::SeparatorText("Loaded Clips");

			ImGui::Separator();
			ImGui::Text("Entity Clips (%zu):", anim->clips.size());

			std::string clipToDelete = "";

			if (ImGui::BeginTable("EntityClipsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("Clip Name");
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableHeadersRow();

				for (auto& [name, clip] : anim->clips) {
					ImGui::PushID(name.c_str());
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", name.c_str());
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Texture: %s\nGrid: %dx%d", clip.textureAsset.c_str(), clip.spriteCols, clip.spriteRows);
					}

					ImGui::TableSetColumnIndex(1);
					if (ImGui::SmallButton("Delete")) {
						clipToDelete = name;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			// Perform deletion safely outside loop
			if (!clipToDelete.empty()) {
				anim->clips.erase(clipToDelete);
				// If we deleted the currently playing clip, stop playback
				if (anim->current == clipToDelete) {
					anim->current = "";
					anim->playing = false;
				}
			}
		}
	};

	/*!
	@brief Game-state provider installed at runtime (no hardcoded state list).
	@details Bridges GameStateManager to the Debug Menu; exposes current/previous/next and issues ChangeGameState() transitions from a reflected state list.
	*/
	struct GameStateProvider final : DebugReflection::IGameStateProvider {
		int  current()  const override { return (int)CurrentGameState; }
		int  previous() const override { return (int)PreviousGameState; }
		int  next()     const override { return (int)NextGameState; }
		void changeTo(int id) override { ChangeGameState((GameState)id); }

		const std::vector<std::pair<const char*, int>>& states() const override {
			static std::vector<std::pair<const char*, int>> cache;
			cache.clear();
			for (auto const& [st, desc] : GetStateRegistry()) {
				const char* label = desc.Label ? desc.Label : "(unnamed)";
				cache.emplace_back(label, (int)st);
			}
			return cache;
		}
	};

	/*!
	@brief Adapter for editing Component::ParticleSystem.
	@details Uses ImGui controls to manage particle system data.
	*/
	struct ParticleSystemAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "Particle System"; }

		// Check for the new single component
		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<Component::ParticleComponent>(e);
		}

		// Add default component
		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) {
				Component::ParticleComponent pc;
				// Initialize defaults if needed
				pc.isEmitting = true;
				pc.maxParticles = 1000;
				auto& p = ecs.AddComponent<Component::ParticleComponent>(e, pc);
				ParticleSystem::ParticleInit(p);
			}
		}

		// Remove component
		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) ecs.RemoveComponent<Component::ParticleComponent>(e);
		}

		// Draw Inspector UI
		void drawInspector(ECS& ecs, Entity e) override {
			auto* pc = ecs.GetComponent<Component::ParticleComponent>(e);
			if (!pc) return;

			// Helper to draw two float inputs side-by-side
			auto DrawMinMaxRow = [](const char* label, float& minRef, float& maxRef, float speed, float minLimit, float maxLimit) {
				ImGui::Text("%s", label);
				float availW = ImGui::GetContentRegionAvail().x;
				float itemW = (availW - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

				ImGui::PushItemWidth(itemW);
				std::string labelMin = "##Min" + std::string(label);
				std::string labelMax = "##Max" + std::string(label);

				UIHelpers::DrawUndoableFloat(labelMin.c_str(), minRef, speed, minLimit, maxLimit);
				ImGui::SameLine();
				UIHelpers::DrawUndoableFloat(labelMax.c_str(), maxRef, speed, minLimit, maxLimit);
				ImGui::PopItemWidth();
				};

			// --- 1. General Settings ---
			// Compact Header
			if (ImGui::CollapsingHeader("Emitter Settings", ImGuiTreeNodeFlags_DefaultOpen)) {

				// Toggle and Space on one line
				float availW = ImGui::GetContentRegionAvail().x;
				ImGui::PushItemWidth(availW * 0.4f);
				UIHelpers::DrawUndoableBool("Emit on Stop", pc->playOnStop);
				ImGui::SameLine();
				UIHelpers::DrawUndoableBool("Emitting", pc->isEmitting);
				ImGui::SameLine();
				UIHelpers::DrawUndoableBool("World Space", pc->useWorldSpace);
				ImGui::PopItemWidth();

				int prevMax = pc->maxParticles;

				// Counts
				// Define the logic that runs during Undo/Redo AND when the user finishes editing
				auto ApplyParticleCounts = [pc](const int& val) {
					int v = val;
					if (v < 0) v = 0;

					pc->maxParticles = v;

					// Clamp emit rate if max is lower
					if (pc->emitRate > (f32)pc->maxParticles) {
						pc->emitRate = (f32)pc->maxParticles;
					}

					// Resize memory vectors
					ParticleSystem::ParticleInit(*pc);
				};

				// Pass 'ApplyParticleCounts' as the custom setter for the Undo Command
				UIHelpers::DrawUndoableInt("Max Count", pc->maxParticles, 1.0f, 0, 100000, ApplyParticleCounts);

				// If the integer changed this frame, resize memory NOW.
				// Do not wait for the mouse release.
				if (pc->maxParticles != prevMax) {
					ApplyParticleCounts(pc->maxParticles);
				}

				UIHelpers::DrawUndoableFloat("Rate (p/s)", pc->emitRate, 0.5f, 0.0f, (f32)pc->maxParticles);
			}

			// --- 2. Shape ---
			ImGui::Separator();
			const char* shapes[] = { "Line", "Circle", "Square" };
			int currentShape = (int)pc->emissionShape;

			// Draw Shape selector with label on left
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Shape:"); ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::Combo("##Shape", &currentShape, shapes, IM_ARRAYSIZE(shapes))) {
				pc->emissionShape = (ParticleSystem::ParticleShape)currentShape;
			}

			if (pc->emissionShape == ParticleSystem::ParticleShape::CIRCLE) {
				DrawMinMaxRow("Radius (Min / Max)", pc->radiusMin, pc->radiusMax, 0.1f, 0.0f, 0.0f);
			}
			else if (pc->emissionShape == ParticleSystem::ParticleShape::LINE) {
				// Controls for Line Length
				DrawMinMaxRow("Length (Min / Max)", pc->lengthMin, pc->lengthMax, 0.1f, 0.0f, 0.0f);
			}
			else if (pc->emissionShape == ParticleSystem::ParticleShape::SQUARE) {
				// Controls for Square Area (Width/Height)
				// Note: Since area is a Vec2, we need two rows or a Vec2 drawer

				// X Axis (Width)
				DrawMinMaxRow("Width (Min / Max)", pc->areaMin.x, pc->areaMax.x, 0.1f, 0.0f, 0.0f);

				// Y Axis (Height)
				DrawMinMaxRow("Height (Min / Max)", pc->areaMin.y, pc->areaMax.y, 0.1f, 0.0f, 0.0f);
			}

			// --- 3. Particle Properties (Compacted) ---
			ImGui::Separator();
			ImGui::TextDisabled("Particle Properties");

			DrawMinMaxRow("Lifetime (min / max)", pc->startLifeMin, pc->startLifeMax, 0.1f, 0.0f, 0.0f);
			DrawMinMaxRow("Speed (min / max)", pc->startSpeedMin, pc->startSpeedMax, 0.1f, 0.0f, 0.0f);
			
			// --- 4. Size & Rotation ---
			ImGui::SeparatorText("Size & Rotation");

			// 1. Uniform Toggle
			UIHelpers::DrawUndoableBool("Uniform Start Size", pc->uniformSize);

			// 2. Conditional UI
			if (pc->uniformSize) {
				// In Uniform mode, we edit X, and the Manager applies it to both X and Y
				DrawMinMaxRow("Start Size (Min / Max)", pc->startSizeXMin, pc->startSizeXMax, 0.05f, 0.0f, 0.0f);
			}
			else {
				// In Non-Uniform mode, we edit X and Y separately
				DrawMinMaxRow("Size X (Min / Max)", pc->startSizeXMin, pc->startSizeXMax, 0.05f, 0.0f, 0.0f);
				DrawMinMaxRow("Size Y (Min / Max)", pc->startSizeYMin, pc->startSizeYMax, 0.05f, 0.0f, 0.0f);
			}

			// Rotation
			DrawMinMaxRow("Start Rotation", pc->startRotationMin, pc->startRotationMax, 1.0f, 0.0f, 0.0f);

			// Modifiers Row
			ImGui::Text("Over Time Changes");
			float availW = ImGui::GetContentRegionAvail().x;
			float itemW = (availW - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
			ImGui::PushItemWidth(itemW);
			UIHelpers::DrawUndoableFloat("Size Delta", pc->sizeOverTime, 0.01f);
			//ImGui::SameLine();
			UIHelpers::DrawUndoableFloat("Rot. Delta", pc->rotationOverTime, 0.1f);
			ImGui::PopItemWidth();

			// --- 4. Physics & Color (Collapsing) ---
			if (ImGui::CollapsingHeader("Physics & Color")) {
				UIHelpers::DrawUndoableVec3("Gravity", pc->gravity);
				UIHelpers::DrawUndoableVec3("Velocity Offset", pc->startVelocity);

				ImGui::Separator();
				ImGui::Text("Color Gradient");
				// Draw colors full width
				UIHelpers::DrawUndoableColor("Start", pc->startcolor);
				UIHelpers::DrawUndoableColor("End", pc->endColor);
			}

			// --- 5. Rendering (Collapsing) ---
			if (ImGui::CollapsingHeader("Texture & UVs")) {
				UIHelpers::DrawUndoableBool("Use Texture", pc->useTexture);

				if (pc->useTexture) {
					// Texture Selection Row
					char buf[128];
					// Shorten name for display if it's long
					std::string displayTex = pc->textureName;
					if (displayTex.length() > 20) displayTex = "..." + displayTex.substr(displayTex.length() - 17);
					strncpy(buf, displayTex.c_str(), sizeof(buf));

					float btnSize = ImGui::GetFrameHeight();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnSize - 8);

					// Read-only text box (edit via popup)
					ImGui::InputText("##TexName", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

					// Drag Drop
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
							const auto* d = static_cast<const AssetDragPayload*>(payload->Data);
							if (d->kind == AssetKind::Texture) pc->textureName = d->relPath;
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine();
					if (ImGui::Button("...", ImVec2(btnSize, btnSize))) {
						ImGui::OpenPopup("Pick Particle Texture");
					}

					// Popup Implementation (Kept logic, simplified display)
					if (ImGui::BeginPopupModal("Pick Particle Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
						namespace fs = std::filesystem;
						static std::vector<std::string> s_assets;
						static bool s_scanned = false;

						if (ImGui::Button("Refresh")) s_scanned = false;

						if (!s_scanned) {
							s_scanned = true;
							s_assets.clear();
							const std::string assetRoot = "assets/images";
							if (fs::exists(assetRoot)) {
								for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
									if (!p.is_regular_file()) continue;
									std::string ext = p.path().extension().string();
									std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
									if (ext == ".png" || ext == ".jpg" || ext == ".tga") {
										s_assets.push_back(fs::relative(p.path(), assetRoot).string());
									}
								}
								std::sort(s_assets.begin(), s_assets.end());
							}
						}

						ImGui::Separator();
						ImGui::BeginChild("tex_list", ImVec2(300, 300), true);
						for (const auto& f : s_assets) {
							if (ImGui::Selectable(f.c_str(), pc->textureName == f)) {
								pc->textureName = f;
								ImGui::CloseCurrentPopup();
							}
						}
						ImGui::EndChild();

						if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
						ImGui::EndPopup();
					}

					UIHelpers::DrawUndoableVec2("UV Scale", pc->textureUV);
				}
			}

			// --- 6. Stats Footer ---
			ImGui::Separator();
			float fraction = (float)pc->data.activeCount / (float)pc->maxParticles;
			char overlay[32];
			sprintf(overlay, "%zu / %d", pc->data.activeCount, pc->maxParticles);
			ImGui::ProgressBar(fraction, ImVec2(-1, 0.0f), overlay);
		}
	};

	/*!
	@brief Physics debug provider (global forces / integrators).
	@details Exposes gravity vector and linear damping via PhysicsHandler getters/setters.
	*/
	struct BuiltinPhysicsProvider final : DebugReflection::IPhysicsProvider {
		const char* name() const override { return "Global Forces"; }
		void draw(CrashoutsEngine::ECS&) override {
			// Gravity
			CEVec3 g = Physics::PhysicsHandler::GetGravity();
			UIHelpers::DrawUndoableVec3("Gravity", g);
			Physics::PhysicsHandler::SetGravity(g);

			// Linear Damping
			float d = Physics::PhysicsHandler::GetLinearDamping();
			UIHelpers::DrawUndoableFloat("Damping", d);
			Physics::PhysicsHandler::SetLinearDamping(d);
		}
	};

	//----------------------------------------------------------------------------- registration singletons

	/*!
	@brief File-local instances of adapters and debug providers registered with reflection.
	@details These live for the lifetime of the process. The registries store raw pointers to
			 these singletons; keep them in an anonymous namespace to avoid ODR/linkage issues.
			 Adapters drive the ECS inspector; providers extend the Physics/Game State panels.
	*/
	static NameAdapter gNameAdapter;
	static LayerAdapter gLayerAdapter;
	static TransformAdapter gTransformAdapter;
	static RigidBodyAdapter gRigidBodyAdapter;
	static ColliderAdapter gColliderAdapter;
	static ShapeAdapter gShapeAdapter;
	static TextAdapter gTextAdapter;
	static AudioAdapter gAudioAdapter;
	static ScriptAdapter gScriptAdapter;
	static CameraAdapter gCameraAdapter;
	static GuiAdapter gGuiAdapter;
	static PPAdapter gPPAdapter;
	static TileMapAdapter gTileMapAdapter;
	static GameStateProvider gProvider;
	static BuiltinPhysicsProvider gBuiltinPhys;
	static KeyframeAnimatorAdapter gKeyframeAdapter;
	static ParticleSystemAdapter gParticleAdapter;
}

/*!
 @brief Registers all built-in component adapters and debug providers.
 @details Call once during startup (after ECS and GameStateManager are initialized).
		  - Registers ECS component adapters with DebugReflection::ComponentRegistry
		  - Installs the Game State provider used by the Game State panel
		  - Registers physics providers (e.g., gravity/damping) with PhysicsRegistry
 @note    Safe to call exactly once; repeated calls will duplicate entries.
 @see     DebugReflection::ComponentRegistry
 @see     DebugReflection::PhysicsRegistry
 @see     DebugReflection::SetGameStateProvider
*/
extern "C" void RegisterBuiltinComponentAdapters() {
	using namespace DebugReflection;
	ComponentRegistry::instance().registerAdapter(&gNameAdapter);
	ComponentRegistry::instance().registerAdapter(&gLayerAdapter);
	ComponentRegistry::instance().registerAdapter(&gTransformAdapter);
	ComponentRegistry::instance().registerAdapter(&gRigidBodyAdapter);
	ComponentRegistry::instance().registerAdapter(&gColliderAdapter);
	ComponentRegistry::instance().registerAdapter(&gShapeAdapter);
	ComponentRegistry::instance().registerAdapter(&gTextAdapter);
	ComponentRegistry::instance().registerAdapter(&gAudioAdapter);
	ComponentRegistry::instance().registerAdapter(&gScriptAdapter);
	ComponentRegistry::instance().registerAdapter(&gCameraAdapter);
	ComponentRegistry::instance().registerAdapter(&gGuiAdapter);
	ComponentRegistry::instance().registerAdapter(&gPPAdapter);
	ComponentRegistry::instance().registerAdapter(&gTileMapAdapter);
	ComponentRegistry::instance().registerAdapter(&gKeyframeAdapter);
	ComponentRegistry::instance().registerAdapter(&gParticleAdapter);
	DebugReflection::SetGameStateProvider(&gProvider);
	DebugReflection::PhysicsRegistry::instance().registerProvider(&gBuiltinPhys);

}
