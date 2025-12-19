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
#include <numeric>
#include <fstream>
#include "debugdefaults.h"
#include "debugAssetLoader.h"
#include "changevaluecommand.h"
#include "scriptvalidation.h"
#include "undomanager.h"

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
#include "hierarchy_components.h"

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

//----------------------------------------------------------------------------- file-local state & helpers
namespace {
	f32 max(f32 x1, f32 y2, f32 z3) {
		return std::max(std::max(x1, y2), z3);
	}
}

/*!
 @brief File-local adapters/providers and registration.

 All adapters/providers live in an anonymous namespace and are registered
 once via RegisterBuiltinComponentAdapters(), which the app calls at startup.
*/
namespace {

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
			if (auto* c = ecs.GetComponent<Component::NameComponent>(e)) {
				draw_reflected(*c, "Name");
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

				float buttonWidth = 25.f;
				float selectableWidth = ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x;

				// Use DontClosePopups so we can click multiple items
				if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups, ImVec2(selectableWidth, 0))) {
					mask ^= bit; // Toggle bit
				}

				// Delete Button (Don't allow deleting Default)
				if (name != "Default") {
					ImGui::SameLine();
					if (ImGui::Button(("X##Remove" + name).c_str(), ImVec2(buttonWidth, 0))) {
						layerToRemove = name;
					}
				}
			}

			// --- 4. Removal Logic (Global removal) ---
			if (!layerToRemove.empty()) {
				unsigned int bitToRemove = layerMap[layerToRemove];
				layerMap.erase(layerToRemove);

				// Clear this bit from ALL entities in the world
				// (This is why we need the ECS reference)
				std::vector<LayerComponent>& layerComponents = ecs.GetComponentArray<LayerComponent>().dense;
				for (LayerComponent& layer : layerComponents) {
					layer.currentLayer &= ~bitToRemove;
				}

				// Also clear it from the current mask being edited immediately
				mask &= ~bitToRemove;
			}

			// --- 5. Add New Layer Logic ---
			ImGui::Separator();
			static char newLayerBuffer[64] = "";
			if (ImGui::InputText("##NewLayer", newLayerBuffer, 64, ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (strlen(newLayerBuffer) > 0) {
					if (layerMap.find(newLayerBuffer) == layerMap.end()) {
						unsigned int nextBit = 1;
						if (!layerMap.empty()) {
							auto maxElement = std::max_element(layerMap.begin(), layerMap.end(),
								[](const auto& a, const auto& b) { return a.second < b.second; });
							nextBit = maxElement->second << 1;
						}
						layerMap.emplace(newLayerBuffer, nextBit);
					}
					newLayerBuffer[0] = '\0';
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Add")) {
				if (strlen(newLayerBuffer) > 0 && layerMap.find(newLayerBuffer) == layerMap.end()) {
					unsigned int nextBit = 1;
					if (!layerMap.empty()) {
						auto maxElement = std::max_element(layerMap.begin(), layerMap.end(),
							[](const auto& a, const auto& b) { return a.second < b.second; });
						nextBit = maxElement->second << 1;
					}
					layerMap.emplace(newLayerBuffer, nextBit);
					newLayerBuffer[0] = '\0';
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

				bool changed = false;

				// 1. Local Position
				float pos[3] = { parentComp->localPos.x, parentComp->localPos.y, parentComp->localPos.z };
				if (ImGui::DragFloat3("Local Position", pos, 0.1f)) {
					parentComp->localPos = { pos[0], pos[1], pos[2] };
					changed = true;
				}

				// 2. Local Rotation (Degrees for UI, Radians internally?)
				// Assuming localRot is in Radians to match TransformComponent standard
				float rot = parentComp->localRot;
				if (ImGui::DragFloat("Local Rotation", &rot, 1.0f)) {
					parentComp->localRot = rot;
					changed = true;
				}

				// 3. Local Scale
				float scale[2] = { parentComp->localScale.x, parentComp->localScale.y };
				if (ImGui::DragFloat2("Local Scale", scale, 0.05f)) {
					parentComp->localScale = { scale[0], scale[1] };
					changed = true;
				}

				// No Undo logic implemented here for brevity, but you can add it similar to below.
				// IMPORTANT: Force Update world transform immediately for responsiveness?
				// Usually the system handles it next frame, which is fine.
			}
			else {
				// ============================================================
				// ROOT MODE: Edit World Transform (TransformComponent)
				// ============================================================
				// This is your existing logic, just wrapped in an else block.

				ImGui::TextDisabled("World Space");

				TransformState before{
					t->position,
					t->rotation,
					t->size
				};

				static bool   sEditing = false;
				static Entity sEditingEntity = 0;
				static TransformState sStart{};

				draw_reflected(*t, "Transform"); // Your reflection helper

				const bool nowAnyActive = ImGui::IsAnyItemActive();

				if (!sEditing && nowAnyActive) {
					sEditing = true;
					sEditingEntity = e;
					sStart = before;
				}

				if (sEditing && sEditingEntity == e && !nowAnyActive) {
					sEditing = false;
					TransformState after{ t->position, t->rotation, t->size };

					const bool changed =
						(sStart.position.x != after.position.x) ||
						(sStart.position.y != after.position.y) ||
						(sStart.position.z != after.position.z) ||
						(sStart.rotation != after.rotation) ||
						(sStart.scale.x != after.scale.x) ||
						(sStart.scale.y != after.scale.y);

					if (changed) {
						UndoManager::GetInstance().AddCommand(
							std::make_unique<CrashoutsEngine::TransformEntityCommand>(e, sStart, after)
						);
					}
					sEditingEntity = 0;
				}
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
			if (auto* rb = ecs.GetComponent<RigidBodyComponent>(e)) {

				// Reflect current values to a temp
				auto oldRef = rb->reflection();
				auto ref = oldRef;

				// edit session tracking
				struct EditSession {
					bool active = false;
					ImGuiID beginActiveId = 0;
					Component::RigidBodyComponent::ReflectionType startRef{};
					Entity entity = 0;
				};
				// one static session
				static EditSession sess;

				ImGui::PushID((int)e);
				ImGuiID activeBefore = ImGui::GetActiveID();

				// Draw all fields
				bool anyChangedThisFrame = draw_reflected(ref, "RigidBody (data)");

				ImGuiID activeAfter = ImGui::GetActiveID();
				ImGui::PopID();

				// Start of an edit session
				if (!sess.active && activeBefore == 0 && activeAfter != 0) {
					sess.active = true;
					sess.beginActiveId = activeAfter;
					sess.startRef = oldRef;
					sess.entity = e;
				}

				// Apply live
				if (anyChangedThisFrame) {
					*rb = Component::RigidBodyComponent(ref);
				}

				// when no control is active anymore
				if (sess.active && sess.entity == e && activeAfter == 0) {
					sess.active = false;

					auto endRef = rb->reflection();
					if (sess.startRef.velocity != endRef.velocity ||
						sess.startRef.max_velocity != endRef.max_velocity ||
						sess.startRef.mass != endRef.mass ||
						sess.startRef.restitution != endRef.restitution)
					{
						using RefT = Component::RigidBodyComponent::ReflectionType;
						UndoManager::GetInstance().AddCommand(
							std::make_unique<CrashoutsEngine::ChangeValueCommand<RefT>>(
								sess.startRef, endRef,
								[rb](const RefT& v) { *rb = Component::RigidBodyComponent(v); }
							)
						);
					}
					// reset session state
					sess = {};
				}

				/*INERTIA CALCULATIONS*/
				f32 inertia{ rb->inertia };
				if (ImGui::DragFloat("Inertia", &inertia)) {
					rb->setInertia(inertia);
				}

				/*Static Friction*/
				f32 staticFric{ rb->staticFriction };
				if (ImGui::DragFloat("Static Friction", &staticFric)) {
					rb->staticFriction = staticFric;
				}

				/*Dynamic Friction*/
				f32 dynamicFric{ rb->dynamicFriction };
				if (ImGui::DragFloat("Dynamic Friction", &dynamicFric)) {
					rb->dynamicFriction = dynamicFric;
				}

				if (ecs.HasComponent<Component::ColliderComponent>(e)) {
					Component::ColliderComponent* collider{ ecs.GetComponent<Component::ColliderComponent>(e) };
					if (ImGui::Button("Calculate Inertia##rb")) rb->setInertia(collider->collider->calculateInertia(rb->getMass())); /*For rotational stuff*/
				}

				// Simulate toggle
				bool prevSim = rb->getFlags().test(Physics::SIMULATE_DYNAMICS);
				bool simulate = prevSim;  // use a local variable for ImGui checkbox

				if (ImGui::Checkbox("Simulate Dynamics", &simulate)) {
					// record undo for the flag change
					UndoManager::GetInstance().AddCommand(
						std::make_unique<CrashoutsEngine::ChangeValueCommand<bool>>(
							prevSim, simulate,
							[rb](bool v) {
								if (v)
								{
									rb->getFlags().set(Physics::SIMULATE_DYNAMICS);
								}
								else
								{
									rb->getFlags().reset(Physics::SIMULATE_DYNAMICS);
								}
							}
						)
					);
					// apply new value now
					if (simulate)
					{
						rb->getFlags().set(Physics::SIMULATE_DYNAMICS);
					}
					else
					{
						rb->getFlags().reset(Physics::SIMULATE_DYNAMICS);
					}
				}


				// Forces
				ImGui::SeparatorText("Forces");
				static float f[3]{ 0.f, 0.f, 0.f };
				ImGui::InputFloat3("Force", f);
				ImGui::SameLine();
				if (ImGui::Button("Apply Force##rb")) rb->addForce({ f[0], f[1], f[2] });
				if (ImGui::Button("Clear Force Accumulator##rb")) rb->clearForceAccumulator();
			}
		}

	};

	/*!
	@brief Adapter for editing Component::ColliderComponent (Sphere / ColliderType::AABB).
	@details Lets users switch collider type and tweak radius/min/max; can snap to Transform.
	*/
	struct ColliderAdapter : DebugReflection::IComponentAdapter {
		const char* name() const override { return "Collider"; }

		bool has(ECS& ecs, Entity e) const override {
			return ecs.HasComponent<ColliderComponent>(e);
		}

		void add(ECS& ecs, Entity e) override {
			auto& D = GetDebugDefaults();
			// Default to a 1.0-radius sphere at the entity's transform position (if present)
			CEVec3 center{ 0.f,0.f,0.f };
			if (auto* t = ecs.GetComponent<TransformComponent>(e)) center = t->position;

			auto col = std::make_unique<Collision::Sphere>(D.sphere_radius, center);
			col->helperType = Collision::HelperType::SPHERE_T;
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
			auto& D = GetDebugDefaults();
			if (!cc || !cc->collider) return;

			ImGui::Checkbox("Enabled", &cc->collider->enabled);

			if (!ecs.HasComponent<RigidBodyComponent>(e)) {
				if (ImGui::Button("Add Static RigidBody (mass=0)")) {
					auto& D3 = GetDebugDefaults();
					ecs.AddComponent<RigidBodyComponent>(e, CEVec3{ 0.f,0.f,0.f }, 0.0f, D3.rb_restitution);
				}
			}

			// Type selector
			int type = static_cast<int>(cc->collider->helperType);
			const char* items[] = { "Sphere", "AABB", "OBB", "Triangle", "Polygon", "None" };
			if (type < 0 || type >= IM_ARRAYSIZE(items)) type = 0;

			Collision::AABBRect shapeDefault{ {-D.aabb_half, -D.aabb_half, 0.f}, {D.aabb_half, D.aabb_half, 0.f} };
			if(ecs.HasComponent<Component::TransformComponent>(e)) {
				Component::TransformComponent *transform{ ecs.GetComponent<Component::TransformComponent>(e) };
				CEVec3 extents{transform->size.x * 0.5f, transform->size.y * 0.5f, 0.f};
				shapeDefault.min = transform->position - extents;
				shapeDefault.max = transform->position + extents;
			}

			// LOOK FOR A CHANGE IN DROPDOWN, CHANGE COLLIDER IF SO
			/*TODO CALC COLLIDER BASED ON TEXTURE*/
			if (ImGui::Combo("Type", &type, items, IM_ARRAYSIZE(items))) {

				CEVec3 c{ 0.f,0.f,0.f };
				if (auto* t = ecs.GetComponent<TransformComponent>(e)) c = t->position;

				std::unique_ptr<Collision::Collider> newCol = nullptr;
				Collision::HelperType targetType = static_cast<Collision::HelperType>(type);

				// SWITCH on the HelperType (UI choice), not the ColliderType
				switch (targetType) {
				case Collision::HelperType::SPHERE_T: {
					newCol = std::make_unique<Collision::Sphere>((shapeDefault.max - shapeDefault.min).x * 0.5f, c);
					break;
				}
				case Collision::HelperType::AABB_T: {
					newCol = std::make_unique<Collision::AABBRect>(
						shapeDefault.min,
						shapeDefault.max
					);
					break;
				}
				case Collision::HelperType::OBB_T: {
					newCol = std::make_unique<Collision::Polygon>(Collision::Polygon::createOBB(
						shapeDefault.min, 
						shapeDefault.max, 
						0.f, CEVec3{}));
					newCol->helperType = Collision::HelperType::OBB_T;
					break;
				}
				case Collision::HelperType::POLYGON_T: {
					newCol = std::make_unique<Collision::Polygon>(Collision::Polygon::createOBB(
						shapeDefault.min,
						shapeDefault.max,
						0.f, CEVec3{}));
					newCol->helperType = Collision::HelperType::POLYGON_T;
					break;
				}
				case Collision::HelperType::TRIANGLE_T: {
					/*o*/
					newCol = std::make_unique<Collision::Polygon>(
						std::initializer_list<CEVec3>{
							shapeDefault.min,
							CEVec3{ (shapeDefault.min.x + shapeDefault.max.x) * 0.5f, shapeDefault.max.y, 0.f },
							{ shapeDefault.min.x + (shapeDefault.max.x-shapeDefault.min.x) , shapeDefault.min.y, 0.f },
							
					},
						0.f, c
					);
					break;
				}
				default: break;
				}

				// Assign HelperType to NEW Collider
				if (newCol) {
					newCol->helperType = targetType;
					cc->collider = std::move(newCol);
				}
			}

			CEVec3 transformPos{ 0.f,0.f,0.f };
			if (auto* t = ecs.GetComponent<TransformComponent>(e)) transformPos = t->position;

			// Draw fields per type
			// --- SPHERE ---
			if (cc->collider->helperType == Collision::HelperType::SPHERE_T) {
				auto* s = static_cast<Collision::Sphere*>(cc->collider.get());
				ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "Sphere Collider");

				float r = s->radius;
				if (ImGui::DragFloat("Radius", &r, 0.05f, 0.0f, 10000.0f)) {
					if (r < 0.0f) r = 0.0f;
					s->radius = r;
				}

				float c[2]{ s->offset.x, s->offset.y };
				if (ImGui::DragFloat2("Offset", c, 0.1f)) {
					s->offset = CEVec3{ c[0], c[1], 0.f };
					s->position = transformPos + s->offset;
				}
			}
			// --- AABB ---
			else if (cc->collider->helperType == Collision::HelperType::AABB_T) {
				auto* a = static_cast<Collision::AABBRect*>(cc->collider.get());
				ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "ColliderType::AABB Collider");

				CEVec3& newExtents{ a->extents };

				CEVec3 transform{ a->position };

				bool edited = false;
				ImGui::DragFloat("Width", &newExtents.x, 0.05f);  edited |= ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit();
				ImGui::DragFloat("Height", &newExtents.y, 0.05f);  edited |= ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit();

				if (edited) {
					CEVec3 newMin{ transform - newExtents };
					CEVec3 newMax{ transform + newExtents };

					a->min = CEVec3{ newMin.x, newMin.y, 0.f };
					a->max = CEVec3{ newMax.x, newMax.y, 0.f };
				}

				float c[2]{ a->offset.x, a->offset.y };
				ImGui::DragFloat2("Offset", c, 0.1f);
				if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
					a->offset = CEVec3{ c[0], c[1], 0.f };
				}

				// optional: snap AABB around Transform
				if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
					if (ImGui::Button("Fit around Transform##aabb")) {
						if (ecs.HasComponent<Component::ShapeComponent>(e)) {
							auto* shape{ ecs.GetComponent<Component::ShapeComponent>(e) };
							CEVec3 extents{};


							switch (shape->type) {
							case ShapeType::Rectangle: {
								extents.x = t->size.x * 0.5f;
								extents.y = t->size.y * 0.5f;
								break;
							}
							case ShapeType::Triangle: {
								CEVec3 t1 = { t->position.x - .5f, t->position.y - .5f, t->position.z };
								CEVec3 t2 = { t->position.x + .5f, t->position.y - .5f, t->position.z };
								CEVec3 t3 = { t->position.x, t->position.y + .5f, t->position.z };

								CEVec3 diffP1P2{ t1 - t2 };
								CEVec3 diffP1P3{ t1 - t3 };
								CEVec3 diffP2P3{ t2 - t3 };

								extents.x = max(std::fabs(diffP1P2.x), std::fabsf(diffP1P3.x), std::fabsf(diffP2P3.x)) * 0.5f;
								extents.y = max(std::fabs(diffP1P2.y), std::fabsf(diffP1P3.y), std::fabsf(diffP2P3.y)) * 0.5f;
								break;
							}
							}

							a->min = t->position - extents;
							a->max = t->position + extents;
							a->extents = (a->max - a->min) * 0.5f;
						}

					}
				}
			}
			// --- OBB ---
			else if (cc->collider->helperType == Collision::HelperType::OBB_T) {
				// Cast to Polygon because that is what createOBB returns
				auto& p = dynamic_cast<Collision::Polygon&>(*cc->collider);

				ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "OBB Collider (Polygon)");

				// 1. Calculate current Extents (Half-Width/Height)
				CEVec3 extents = Collision::Polygon::calculateOBBExtents(p);

				bool edited = false;
				// Note: Dragging "Extents" implies editing Half-Width. 
				if (ImGui::DragFloat("Half Width", &extents.x, 0.05f, 0.1f)) edited = true;
				if (ImGui::DragFloat("Half Height", &extents.y, 0.05f, 0.1f)) edited = true;

				// 2. Edit Offset
				float c[2]{ p.offset.x, p.offset.y };
				if (ImGui::DragFloat2("Offset", c, 0.05f)) {
					p.offset = CEVec3{ c[0], c[1], 0.f };
					edited = true;
				}

				// 3. Edit Rotation
				float rot = p.rotation;
				if (ImGui::DragFloat("Rotation", &rot, 0.05f)) {
					// We must Update the internal rotation variable immediately
					// so that if we rebuild the box below, we don't lose the rotation.
					p.rotation = rot;
					edited = true;
				}

				if (edited) {
					// 4. Get LIVE Transform to prevent drift
					float targetRotation = p.rotation;

					CEVec3 currentTransformPos{ 0.f, 0.f, 0.f };
					if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
						currentTransformPos = t->position;
					}

					// 5. Calculate the True Center
					CEVec3 center = currentTransformPos + p.offset;
					p.position = center;

					// 6. Reconstruct Bounds around the Center
					CEVec3 min = center - extents;
					CEVec3 max = center + extents;

					// 7. Update the Polygon Vertices (Resets them to axis-aligned first)
					Collision::Polygon::updateOBB(min, max, p);

					// 8. Re-apply Rotation
					// updateOBB sets vertices axis-aligned. We must re-rotate them 
					// around the center to match the p.rotation we just edited.
					p.forceUpdateRotation(targetRotation);
				}
			}
#if 0
			// --- POLYGON / TRIANGLE ---
			else if (cc->collider->helperType == Collision::HelperType::POLYGON_T ||
				cc->collider->helperType == Collision::HelperType::TRIANGLE_T)
			{
				auto* p = static_cast<Collision::Polygon*>(cc->collider.get());
				ImGui::TextColored(ImVec4(0.6f, 1.f, 0.6f, 1.f), "Polygon/Triangle Collider");
				ImGui::Text("Vertices: %d", p->vertexCount);

				float rot = p->rotation;
				if (ImGui::DragFloat("Rotation", &rot, 0.05f)) {
					p->updateRotation(rot);
				}

				// Offset Logic for Polygon
				float c[2]{ p->offset.x, p->offset.y };
				if (ImGui::DragFloat2("Offset", c, 0.05f)) {
					p->offset = CEVec3{ c[0], c[1], 0.f };
					p->updatePosition(transformPos); // Use helper to shift vertices
				}
			}
#endif
			ImGui::Checkbox("IsTrigger", &cc->collider->isTrigger);
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
			using namespace Component;
			auto* sc = ecs.GetComponent<ShapeComponent>(e);
			auto& D = GetDebugDefaults();
			if (!sc) return;

			// type
			int oldType = (int)sc->type;
			int t = oldType;
			const char* items[] = { "Rectangle","Line","Triangle","Circle" };
			if (ImGui::Combo("Type", &t, items, IM_ARRAYSIZE(items))) {
				UndoManager::GetInstance().AddCommand(
					std::make_unique<CrashoutsEngine::ChangeValueCommand<int>>(
						oldType, t,
						[sc](int v) { sc->type = (ShapeType)v; }
					)
				);
				sc->type = (ShapeType)t;
			}

			//draw_reflected(*sc, "Shape");

			// shared
			bool oldTex = sc->enableTexture;
			if (ImGui::Checkbox("Use TextureAsset", &sc->enableTexture)) {
				UndoManager::GetInstance().AddCommand(
					std::make_unique<CrashoutsEngine::ChangeValueCommand<bool>>(
						oldTex, sc->enableTexture,
						[sc](bool v) { sc->enableTexture = v; }
					)
				);
			}
			bool oldWorld = sc->isWorldSpace;
			if (ImGui::Checkbox("Use World Space", &sc->isWorldSpace)) {
				UndoManager::GetInstance().AddCommand(
					std::make_unique<CrashoutsEngine::ChangeValueCommand<bool>>(
						oldWorld, sc->isWorldSpace,
						[sc](bool v) { sc->isWorldSpace = v; }
					)
				);
			}

			char buf[256]{};
			if (sc->textureName.size() < sizeof(buf)) std::memcpy(buf, sc->textureName.data(), sc->textureName.size());

			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 18.0f);
			ImGui::InputText("##tex_name", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
					// Hard guard: verify this is actually an AssetDragPayload
					if (!payload->Data || payload->DataSize != sizeof(AssetDragPayload)) {
						LOGW(Debug, "Dropped non-asset payload on Shape texture field");
					}
					else {
						const auto* d = static_cast<const AssetDragPayload*>(payload->Data);

						// Only accept textures here
						if (d->kind == AssetKind::Texture) {
							std::string oldTexName = sc->textureName;

							// d->relPath is something like "images/ui/button.png"
							std::string rel = d->relPath;
							const std::string prefix = "images/";
							auto pos = rel.find(prefix);
							if (pos == 0) {
								// Store path relative to the images root, matching the picker
								rel = rel.substr(prefix.size());
							}

							// Fallback to the asset name if something is weird
							sc->textureName = rel.empty() ? std::string(d->name) : rel;

							// Using a texture now
							sc->enableTexture = true;

							// Undo support
							UndoManager::GetInstance().AddCommand(
								std::make_unique<CrashoutsEngine::ChangeValueCommand<std::string>>(
									oldTexName, sc->textureName,
									[sc](const std::string& v) { sc->textureName = v; }
								)
							);
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			ImGui::TextDisabled("TextureAsset");

			ImGui::SameLine();
			if (ImGui::SmallButton("Choose...")) {
				ImGui::OpenPopup("Pick Texture");
			}

			// Modal popup with a simple file browser
			if (ImGui::BeginPopupModal("Pick Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				namespace fs = std::filesystem;
				static std::vector<std::string> s_assets;
				static bool s_scanned = false;

				// Refresh List button
				if (ImGui::Button("Refresh List")) {
					s_scanned = false;
				}
				ImGui::SameLine();

				if (!s_scanned) {
					s_scanned = true;
					s_assets.clear();

					const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "images"; // base folder
					for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
						if (!p.is_regular_file()) continue;

						const std::string ext = p.path().extension().string();
						std::string lower = ext;
						std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
						if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
							lower == ".tga" || lower == ".bmp") {

							// Store relative path for cleaner display
							std::string rel = fs::relative(p.path(), assetRoot).string();
							s_assets.push_back(rel);
						}
					}

					std::sort(s_assets.begin(), s_assets.end());
				}

				// Left: list; Right: preview name
				ImGui::BeginChild("asset_list", ImVec2(420, 300), true);
				for (auto const& f : s_assets) {
					if (ImGui::Selectable(f.c_str())) {
						std::string oldTexName = sc->textureName;
						sc->textureName = f;
						UndoManager::GetInstance().AddCommand(
							std::make_unique<CrashoutsEngine::ChangeValueCommand<std::string>>(
								oldTexName, sc->textureName,
								[sc](const std::string& v) { sc->textureName = v; }
							)
						);
						ImGui::CloseCurrentPopup();
						break;
					}
				}
				ImGui::EndChild();

				if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}

			// If using texture, enable the texture options
			if (sc->enableTexture) {

				const char* wrapModes[] = { "Clamp To Edge","Repeat","Mirror" };
				const int numWrapModes = IM_ARRAYSIZE(wrapModes);

				// U (HORIZONTAL) WRAP MODE
				int oldU = sc->wrapModeU;
				if (ImGui::Combo("U Wrap Mode", &sc->wrapModeU, wrapModes, numWrapModes)) {
					UndoManager::GetInstance().AddCommand(
						std::make_unique<CrashoutsEngine::ChangeValueCommand<int>>(
							oldU, sc->wrapModeU,
							[sc](int v) { sc->wrapModeU = v; }
						)
					);
				}

				// V (HORIZONTAL) WRAP MODE
				int oldV = sc->wrapModeV;
				if (ImGui::Combo("V Wrap Mode", &sc->wrapModeV, wrapModes, numWrapModes)) {
					UndoManager::GetInstance().AddCommand(
						std::make_unique<CrashoutsEngine::ChangeValueCommand<int>>(
							oldV, sc->wrapModeV,
							[sc](int v) { sc->wrapModeV = v; }
						)
					);
				}

				// UV size
				CEVec2 oldUV = sc->uvSize;
				float size[2] = { sc->uvSize.x, sc->uvSize.y };
				if (ImGui::DragFloat2("UV Size", size, D.drag_small)) {
					CEVec2 newUV = { size[0], size[1] };
					if (ImGui::IsItemDeactivatedAfterEdit() && newUV != oldUV) {
						UndoManager::GetInstance().AddCommand(
							std::make_unique<CrashoutsEngine::ChangeValueCommand<CEVec2>>(
								oldUV, newUV,
								[sc](const CEVec2& v) { sc->uvSize = v; }
							)
						);
					}
					sc->uvSize = newUV;
				}

				// UV offset
				CEVec2 oldUVoffset = sc->uvOffset;
				float offset[2] = { sc->uvOffset.x, sc->uvOffset.y };
				if (ImGui::DragFloat2("UV Offset", offset, D.drag_small)) {
					CEVec2 newUV = { offset[0], offset[1] };
					if (ImGui::IsItemDeactivatedAfterEdit() && newUV != oldUV) {
						UndoManager::GetInstance().AddCommand(
							std::make_unique<CrashoutsEngine::ChangeValueCommand<CEVec2>>(
								oldUV, newUV,
								[sc](const CEVec2& v) { sc->uvOffset = v; }
							)
						);
					}
					sc->uvOffset = newUV;
				}

			}

			//ImGui::DragFloat3("Color", &sc->color.x, D.drag_small, 0.f, 255.f);
			//ImGui::DragFloat("Rotation", &sc->rotation, D.drag_rotation);
			//ImGui::DragFloat("Scale", &sc->scale, D.drag_small, 0.01f, 100.f);

			// Colour Mixer a la carte
			{
				static CEColor startColor;
				static bool editingColor = false;

				//CEVec3 oldColor = sc->color;
				float col[4] = {
					sc->color.x(),
					sc->color.y(),
					sc->color.z(),
					sc->color.w()
				};

				ImGuiColorEditFlags flags =
					ImGuiColorEditFlags_DisplayHSV |
					ImGuiColorEditFlags_InputRGB |
					ImGuiColorEditFlags_PickerHueBar |
					ImGuiColorEditFlags_AlphaBar;

				// start editing when user opens popup
				if (ImGui::ColorEdit4("Color", col, flags) && !editingColor) {
					editingColor = true;
					startColor = sc->color;
				}
				if (ImGui::IsItemDeactivated() && editingColor) {
					// item lost focus (enter pressed / tab / clicked away)
					CEColor newColor = { col[0], col[1], col[2], col[3] };
					if (newColor != startColor) {
						UndoManager::GetInstance().AddCommand(
							std::make_unique<CrashoutsEngine::ChangeValueCommand<CEColor>>(
								startColor, newColor,
								[sc](const CEColor& v) { sc->color = v; }));
					}
					editingColor = false;
				}

				// apply live color
				sc->color = { col[0], col[1], col[2], col[3] };

				if (ImGui::SmallButton("Open Color Picker"))
					ImGui::OpenPopup("##shape_color_picker");

				if (ImGui::BeginPopup("##shape_color_picker")) {
					static CEColor popupStart;
					static bool popupEditing = false;

					// snapshot when opened
					if (!popupEditing) { popupEditing = true; popupStart = sc->color; }

					float pick[4] = {
						sc->color.x() ,
						sc->color.y() ,
						sc->color.z() ,
						sc->color.w()
					};
					if (ImGui::ColorPicker4("##picker", pick, flags)) {
						sc->color = { pick[0], pick[1], pick[2], pick[3] };
					}

					// detect popup close
					if (!ImGui::IsWindowAppearing() && !ImGui::IsWindowFocused()) {
						CEColor newCol = sc->color;
						if (popupEditing && newCol != popupStart) {
							UndoManager::GetInstance().AddCommand(
								std::make_unique<CrashoutsEngine::ChangeValueCommand<CEColor>>(
									popupStart, newCol,
									[sc](const CEColor& v) { sc->color = v; }));
						}
						popupEditing = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			}

			// per-type
			if (sc->type == ShapeType::Line) {
				CEVec3 oldP1 = sc->p1;
				CEVec3 oldP2 = sc->p2;
				float oldThick = sc->thickness;

				ImGui::DragFloat3("P1", &sc->p1.x, 0.5f);
				if (ImGui::IsItemDeactivatedAfterEdit() && sc->p1 != oldP1) {
					UndoManager::GetInstance().AddCommand(
						std::make_unique<CrashoutsEngine::ChangeValueCommand<CEVec3>>(
							oldP1, sc->p1,
							[sc](const CEVec3& v) { sc->p1 = v; }));
				}

				ImGui::DragFloat3("P2", &sc->p2.x, 0.5f);
				if (ImGui::IsItemDeactivatedAfterEdit() && sc->p2 != oldP2) {
					UndoManager::GetInstance().AddCommand(
						std::make_unique<CrashoutsEngine::ChangeValueCommand<CEVec3>>(
							oldP2, sc->p2,
							[sc](const CEVec3& v) { sc->p2 = v; }));
				}

				ImGui::DragFloat("Thickness", &sc->thickness, 0.1f, 0.0f, 1e6f);
				if (ImGui::IsItemDeactivatedAfterEdit() && sc->thickness != oldThick) {
					UndoManager::GetInstance().AddCommand(
						std::make_unique<CrashoutsEngine::ChangeValueCommand<float>>(
							oldThick, sc->thickness,
							[sc](float v) { sc->thickness = v; }));
				}
			}


			/*CEVec3 pos{ 0,0,0 };
			if (auto* tf = ecs.GetComponent<Component::TransformComponent>(e)) pos = tf->position;
			switch (sc->type) {
			case Component::ShapeType::Square:    Component::Shapes::CreateSquare(pos, sc->size, sc->rotation, sc->scale, sc->color); break;
			case Component::ShapeType::Rectangle: Component::Shapes::CreateRectangle(pos, sc->width, sc->height, sc->rotation, sc->scale, sc->color); break;
			case Component::ShapeType::Line:      Component::Shapes::
			(sc->p1, sc->p2, sc->thickness, sc->rotation, sc->scale, sc->color); break;
			case Component::ShapeType::Triangle:  Component::Shapes::CreateTriangle(sc->t1, sc->t2, sc->t3, sc->rotation, sc->scale, sc->color); break;
			}*/
			// reminder to ask how/where is the rendering of the shapes done in general, i don't wanna hunt for functions
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
			using namespace Component;
			auto* fc = ecs.GetComponent<TextComponent>(e);
			//auto& D = GetDebugDefaults();
			if (!fc) return;

			char buf[256]{};
			if (fc->font.size() < sizeof(buf)) std::memcpy(buf, fc->font.data(), fc->font.size());

			ImGui::InputText("Font Name", buf, sizeof(buf));
			if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
				fc->font = buf;
			}

			char buf2[512]{};
			if (fc->text.size() < sizeof(buf2)) std::memcpy(buf2, fc->text.data(), fc->text.size());

			ImGui::InputText("Display Text", buf2, sizeof(buf2));
			if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
				fc->text = buf2;
			}

			ImGui::Checkbox("Center Text", &fc->centered);

			ImGui::Checkbox("Use World Space", &fc->isWorldSpace);

			ImGui::DragFloat("Scale", &fc->scale);

			ImGui::DragFloat2("offset", &fc->offset.x);

			//ImGui::DragFloat3("Color", &fc->color.x, 1.f, 0.f, 255.f);

			{
				float col[4] = {
					fc->color.x(),
					fc->color.y(),
					fc->color.z(),
					fc->color.w()
				};

				ImGuiColorEditFlags flags =
					ImGuiColorEditFlags_DisplayHSV |
					ImGuiColorEditFlags_InputRGB |
					ImGuiColorEditFlags_PickerHueBar |
					ImGuiColorEditFlags_AlphaBar;

				if (ImGui::ColorEdit4("Color", col, flags) || ImGui::IsItemDeactivatedAfterEdit())
				{
					fc->color.x() = col[0];
					fc->color.y() = col[1];
					fc->color.z() = col[2];
					fc->color.w() = col[3];
				}

				if (ImGui::SmallButton("Open Color Picker"))
					ImGui::OpenPopup("##shape_color_picker");

				if (ImGui::BeginPopup("##shape_color_picker"))
				{
					if (ImGui::ColorPicker4("##picker", col, flags))
					{
						fc->color.x() = col[0];
						fc->color.y() = col[1];
						fc->color.z() = col[2];
						fc->color.w() = col[3];
					}
					ImGui::EndPopup();
				}
			}
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
			if (has(ecs, e)) {
				if (auto* c = ecs.GetComponent<Component::AudioEmitterComponent>(e)) {

					if (c->inst) {
						c->inst->stop(FMOD_STUDIO_STOP_IMMEDIATE);
						c->inst->release();
						c->inst = nullptr;
					}

					if (c->channel) {
						c->channel->stop();
						c->channel = nullptr;
					}
				}
				ecs.RemoveComponent<Component::AudioEmitterComponent>(e);
			}
		}
		void drawInspector(ECS& ecs, Entity e) override {
			using Component::AudioEmitterComponent;
			auto* c = ecs.GetComponent<AudioEmitterComponent>(e);
			if (!c) return;

			// Event path text
			ImGui::SeparatorText("Event");

			char buf[256]{};
			if (c->eventPath.size() < sizeof(buf)) std::memcpy(buf, c->eventPath.data(), c->eventPath.size());
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 18.0f);
			ImGui::InputTextWithHint("##event_path", "event:/…", buf, sizeof(buf));
			if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
				c->eventPath = buf;
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
					// Hard guard: ignore wrong payload types
					if (!payload->Data || payload->DataSize != sizeof(AssetDragPayload)) {
						LOGW(Debug, "Dropped non-audio payload on AudioComponent eventPath");
					}
					else {
						const auto* d = static_cast<const AssetDragPayload*>(payload->Data);

						// Only accept Audio assets here
						if (d->kind == AssetKind::Audio) {
							std::filesystem::path relPath = d->relPath;
							std::string ext = relPath.extension().string();
							std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

							std::string rel = relPath.generic_string();
							const std::string prefix = "audio/";
							auto pos = rel.find(prefix);
							if (pos == 0) {
								rel = rel.substr(prefix.size());
							}

							if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
								// raw clip → just store relative path
								c->eventPath = rel;
							}
							else {
								// bank event → strip extension and prepend event:/
								if (auto dot = rel.find_last_of('.'); dot != std::string::npos) {
									rel.erase(dot);
								}
								c->eventPath = std::string("event:/") + rel;
							}
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			ImGui::TextDisabled("Event");

			ImGui::SameLine();
			if (ImGui::SmallButton("Choose...")) {
				ImGui::OpenPopup("Pick Audio Event");
			}

			if (ImGui::BeginPopupModal("Pick Audio Event", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				namespace fs = std::filesystem;
				static std::vector<std::string> s_audioDisplay;
				static std::vector<std::string> s_audioEvent;
				static bool s_scanned = false;

				// one-time (per session) scan
				if (!s_scanned) {
					s_scanned = true;
					s_audioDisplay.clear();
					s_audioEvent.clear();

					const fs::path assetRoot = "assets/audio";
					auto to_event_like = [&](const fs::path& p) -> std::string {
						// turn assets/audio/SFX/Beep.wav -> event:/SFX/Beep
						fs::path rel = fs::relative(p, assetRoot);
						fs::path withoutExt = rel;
						if (withoutExt.has_extension()) withoutExt.replace_extension();
						std::string e = withoutExt.generic_string();

						// Capitalization/punctuation are preserved; prepend event:/ and normalize slashes.
						return "event:/" + e;
						};

					try {
						for (auto& entry : fs::recursive_directory_iterator(assetRoot)) {
							if (!entry.is_regular_file()) continue;

							std::string ext = entry.path().extension().string();
							std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

							std::string disp = fs::relative(entry.path(), assetRoot).generic_string();

							if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
							{
								// raw audio file → store relative path
								s_audioDisplay.push_back(disp);
								s_audioEvent.push_back(disp);   // <-- no "event:/"
							}

							// Include common authoring/artifacts; refine as needed for your pipeline
							else if (ext == ".bank") {
								std::string rel = fs::relative(entry.path(), assetRoot).generic_string(); // e.g. "SFX/explosion.wav"

								s_audioDisplay.push_back(rel);
								s_audioEvent.push_back(rel);   // this will become c->eventPath
							}
						}

						// Sort by display name
						std::vector<size_t> idx(s_audioDisplay.size());
						std::iota(idx.begin(), idx.end(), 0);
						std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
							return s_audioDisplay[a] < s_audioDisplay[b];
							});

						std::vector<std::string> disp2, ev2;
						disp2.reserve(idx.size()); ev2.reserve(idx.size());
						for (size_t i : idx) { disp2.push_back(std::move(s_audioDisplay[i])); ev2.push_back(std::move(s_audioEvent[i])); }
						s_audioDisplay.swap(disp2); s_audioEvent.swap(ev2);
					}
					catch (...) {
						// swallow errors; empty list will be shown
					}
				}

				// Left: list
				ImGui::BeginChild("audio_list", ImVec2(420, 300), true);
				for (size_t i = 0; i < s_audioDisplay.size(); ++i) {
					if (ImGui::Selectable(s_audioDisplay[i].c_str())) {
						c->eventPath = s_audioEvent[i];

						// reflect selection back to the read-only text box
						std::strncpy(buf, c->eventPath.c_str(), sizeof(buf) - 1);
						buf[sizeof(buf) - 1] = '\0';

						ImGui::CloseCurrentPopup();
						break;
					}
				}
				ImGui::EndChild();

				if (ImGui::Button("Rescan")) {
					// allow rescanning if you add new audio during runtime
					s_scanned = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}

			ImGui::DragFloat("Volume", &c->volume, 0.01f, 0.f, 5.f);
			ImGui::DragFloat("Pitch", &c->pitch, 0.01f, 0.25f, 4.f);
			ImGui::Checkbox("3D Spatial", &c->spatial3D);

			auto& audio = Audio::AudioSystem::instance();
			bool hasRunning = (c->inst != nullptr) || (c->channel != nullptr);
			// Create/Play/Stop lifecycle
			if (ImGui::Button(hasRunning ? "Restart" : "Play")) {
				if (!c->eventPath.empty()) {
					// stop old instance if any
					if (c->inst) {
						c->inst->stop(FMOD_STUDIO_STOP_IMMEDIATE);
						c->inst->release();
						c->inst = nullptr;
					}

					if (c->channel) {
						c->channel->stop();
						c->channel = nullptr;
					}

					// --- FMOD Studio event path ---
					if (c->eventPath.rfind("event:/", 0) == 0) {
						if (FMOD::Studio::EventInstance* inst = audio.createInstance(c->eventPath)) {
							inst->setVolume(c->volume);
							inst->setPitch(c->pitch);

							if (c->spatial3D) {
								if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) {
									// your existing 3D attach code here
								}
							}

							inst->start();
							c->inst = inst;
						}
					}
					// --- raw file path under assets/audio (wav/ogg/mp3) ---
					else {
						std::string path = c->eventPath;
						if (path.rfind("assets/", 0) != 0)
							path = "assets/audio/" + path;

						// stop previous raw channel
						if (c->channel) {
							c->channel->stop();
							c->channel = nullptr;
						}

						// play new raw file and store channel
						c->channel = audio.playSFXFileEx(path);

						if (c->channel)
						{
							c->channel->setVolume(c->volume);
							c->channel->setPitch(c->pitch);
						}
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop"))
			{
				if (c->inst) {
					c->inst->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
					c->inst = nullptr;
				}
				if (c->channel) {
					c->channel->stop();
					c->channel = nullptr;
				}
			}

			if (c->channel) {
				bool isPlaying = false;
				c->channel->isPlaying(&isPlaying);
				ImGui::Text("Channel valid: %s", isPlaying ? "PLAYING" : "NOT PLAYING");
			}
			else {
				ImGui::Text("Channel is NULL!");
			}


			// Live controls for a running instance
			if (c->inst) {
				c->inst->setVolume(c->volume);
				c->inst->setPitch(c->pitch);

				// Expose event parameters dynamically
				FMOD::Studio::EventDescription* desc = nullptr;
				c->inst->getDescription(&desc);
				if (desc) {
					int count = 0; desc->getParameterDescriptionCount(&count);
					for (int i = 0; i < count; ++i) {
						FMOD_STUDIO_PARAMETER_DESCRIPTION pd{};
						if (desc->getParameterDescriptionByIndex(i, &pd) == FMOD_OK) {
							float v = 0.f;
							c->inst->getParameterByID(pd.id, &v, nullptr);
							if (ImGui::DragFloat(pd.name, &v, 0.01f, pd.minimum, pd.maximum, "%.3f")) {
								c->inst->setParameterByID(pd.id, v);
							}
						}
					}
				}
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
			return ecs.HasComponent<CrashoutsEngine::ScriptComponent>(e);
		}

		void add(ECS& ecs, Entity e) override {
			if (!has(ecs, e)) {
				static char className[64] = "PlayerController";
				ImGui::InputText("Class Name to Add", className, sizeof(className));
				CrashoutsEngine::ScriptComponent sc(className, e);
				ecs.AddComponent<CrashoutsEngine::ScriptComponent>(e, sc);
			}
		}

		void remove(ECS& ecs, Entity e) override {
			if (has(ecs, e)) {
				ecs.RemoveComponent<CrashoutsEngine::ScriptComponent>(e);
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

				if (inst.Instance && inst.Class) {
					ImGui::Indent();
					ImGui::TextDisabled("Properties:");

					void* iter = nullptr;
					while (MonoClassField* field = mono_class_get_fields(inst.Class, &iter)) {
						const char* fieldName = mono_field_get_name(field);
						uint32_t flags = mono_field_get_flags(field);

						// 1. Filter: Must NOT be static
						if (flags & MONO_FIELD_ATTR_STATIC) continue;

						// 2. Filter: Must be Public OR have [SerializeField]
						bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
						bool hasAttribute = false;

						// Look up the attribute class (Cache this static pointer for speed!)
						static MonoClass* attrClass = mono_class_from_name(
							MonoManager::GetImage(), "CrashoutsEngine", "SerializeField");

						if (attrClass) {
							// Get all attributes on this specific field
							MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(inst.Class, field);
							if (attrInfo) {
								// Check if our specific attribute is present
								hasAttribute = mono_custom_attrs_has_attr(attrInfo, attrClass);
								mono_custom_attrs_free(attrInfo); // Important: Clean up!
							}
						}

						if (isPublic || hasAttribute) { // || hasAttribute
							MonoType* type = mono_field_get_type(field);
							int typeEnum = mono_type_get_type(type);

							// Integer
							if (typeEnum == MONO_TYPE_I4) {
								int val = 0;
								mono_field_get_value(inst.Instance, field, &val);
								if (ImGui::DragInt(fieldName, &val)) {
									mono_field_set_value(inst.Instance, field, &val);
								}
							}
							// Float
							else if (typeEnum == MONO_TYPE_R4) {
								float val = 0.0f;
								mono_field_get_value(inst.Instance, field, &val);
								if (ImGui::DragFloat(fieldName, &val, 0.1f)) {
									mono_field_set_value(inst.Instance, field, &val);
								}
							}
							// Boolean
							else if (typeEnum == MONO_TYPE_BOOLEAN) {
								bool val = false;
								mono_field_get_value(inst.Instance, field, &val);
								if (ImGui::Checkbox(fieldName, &val)) {
									mono_field_set_value(inst.Instance, field, &val);
								}
							}
							// String
							else if (typeEnum == MONO_TYPE_STRING) {
								MonoString* monoStr = nullptr;
								mono_field_get_value(inst.Instance, field, &monoStr);
								char strBuf[128] = "";
								if (monoStr) {
									char* raw = mono_string_to_utf8(monoStr);
									strncpy(strBuf, raw, 127);
									mono_free(raw);
								}
								if (ImGui::InputText(fieldName, strBuf, 128)) {
									MonoString* newStr = mono_string_new(MonoManager::GetDomain(), strBuf);
									mono_field_set_value(inst.Instance, field, newStr);
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
			using namespace Component;
			auto* cc = ecs.GetComponent<CameraComponent>(e);
			//auto& D = GetDebugDefaults();
			if (!cc) return;

			// ImGui::Checkbox("Enable Camera", &cc->enabled);      // CURRENTLY NOT IMPLEMENTED
			// ImGui::DragFloat3("Position", &cc->position.x, 1.f); // Locked access
			ImGui::DragFloat("Zoom", &cc->zoom, 0.005f, 0.05f);
			// ImGui::DragFloat("Target Aspect Ratio", &cc->targetAR);
			ImGui::DragFloat2("X Boundaries (min/max)", &cc->xBounds.x, 1.f);
			ImGui::DragFloat2("Y Boundaries (min/max)", &cc->yBounds.x, 1.f);

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
			using namespace Component;
			// 1. Get the PPComponent
			auto* ppc = ecs.GetComponent<PPComponent>(e);

			if (!ppc) return;

			ImGui::SeparatorText("Post-Processing");

			// 2. Add ImGui widgets for each component member

			// Checkbox for the 'active' bool
			ImGui::Checkbox("Active", &ppc->active);

			// DragFloats for the color grading values
			// We use a small speed (0.01f) and a sensible range.
			// These values are 0.0 by default, representing "no change".

			// Brightness: -1.0 (black) to 1.0 (double bright)
			ImGui::DragFloat("Brightness", &ppc->brightness, 0.01f, -1.0f, 1.0f);

			// Contrast: -1.0 (all gray) to 1.0 (double contrast)
			ImGui::DragFloat("Contrast", &ppc->contrast, 0.01f, -1.0f, 1.0f);

			// Saturation: -1.0 (grayscale) to 1.0 (double saturation)
			ImGui::DragFloat("Saturation", &ppc->saturation, 0.01f, -1.0f, 1.0f);

			// A reset button is very useful for these settings
			if (ImGui::Button("Reset Values")) {
				ppc->brightness = 0.0f;
				ppc->contrast = 0.0f;
				ppc->saturation = 0.0f;
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
							const char* colliderNames[] = { "Sphere", "AABB", "OBB", "Triangle", "Polygon", "None" };
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
	@brief Physics debug provider (global forces / integrators).
	@details Exposes gravity vector and linear damping via PhysicsHandler getters/setters.
	*/
	struct BuiltinPhysicsProvider final : DebugReflection::IPhysicsProvider {
		const char* name() const override { return "Global Forces"; }
		void draw(CrashoutsEngine::ECS&) override {
			// Gravity
			CEVec3 g = Physics::PhysicsHandler::GetGravity();
			float gv[3]{ g.x,g.y,g.z };
			if (ImGui::DragFloat3("Gravity (m/s^2)", gv, 0.1f)) {
				Physics::PhysicsHandler::SetGravity({ gv[0],gv[1],gv[2] });
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset g##phys")) {
				Physics::PhysicsHandler::SetGravity({ 0.f,-2.f,0.f });
			}

			// Linear damping
			float d = Physics::PhysicsHandler::GetLinearDamping();
			if (ImGui::DragFloat("Linear damping", &d, 0.05f, 0.f, 100.f)) {
				Physics::PhysicsHandler::SetLinearDamping(d);
			}
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
	DebugReflection::SetGameStateProvider(&gProvider);
	DebugReflection::PhysicsRegistry::instance().registerProvider(&gBuiltinPhys);

}
