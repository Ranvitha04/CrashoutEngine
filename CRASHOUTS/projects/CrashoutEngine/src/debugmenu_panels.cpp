/*!
@file       debugmenu_panels.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       25/12/2025
@brief      File containing the implementation of the ECS Debug Panel for the Debug Menu.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "debugmenu_internals.h"
#include "debugmenu.h"
#include "physicsmanager.h"
#include "deleteentitycommand.h"
#include <unordered_set>
#include "engine.h"
#include "collidereventmanager.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

using namespace CrashoutsEngine;
using namespace Serialization;
using namespace KeyframeUnity;

// Raw string for copying template game project items
namespace {
	// --- main.cpp Template ---
	// --- main.cpp Template ---
	const std::string MAIN_CPP_TEMPLATE = R"(
// Extension loader library's header must be included before GLFW's header!!!
#pragma warning(push)
#pragma warning(disable: 4005)

#include "engine.h"

#pragma warning(pop)

#define NOMINMAX     // <--- Prevents windows.h from breaking std::min/max
#include <windows.h> // <-- Fixes the WINAPI and HINSTANCE syntax errors!
#include <cstdio>
#include <crtdbg.h>

#include "transformcomponent.h"
#include "performanceViewer.h"
#include "glapp.h"
#include "frameratecontroller.h"
#include "crashlogger.h"

static CrashoutsEngine::Entity globalSelectedEntity = 0;
auto& FRC = FrameRateController::GetInstance();

int WINAPI WinMain(
    [[maybe_unused]] _In_     HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_     LPSTR     lpCmdLine,
    [[maybe_unused]] _In_     int       nCmdShow
) {
#if DEBUG
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    SetupConsole();
#endif

    InstallCrashLogger("CrashoutsEngine");

    // Root is set dynamically to the generated game project
    SetGameRoot("../../projects/GameProjects/{{GAME_NAME}}");

    Init();
    double fixedDT = FRC.FixedDT();

    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        FRC.FrameStart();
        glfwPollEvents();
        fixedDT = FRC.FixedDT();

        Update(fixedDT);
        Draw(globalSelectedEntity);
        FRC.FrameEnd();
    }

    LogExitCode();
    CleanUp();

    return EXIT_SUCCESS;
}
)";

	// --- CMakeLists.txt Template ---
	// --- CMakeLists.txt Template ---
	const std::string CMAKELISTS_TEMPLATE = R"(
cmake_minimum_required(VERSION 3.20)
project({{GAME_NAME}})

set(project_dir ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h")

# ======================================================
# TARGET 1: Standalone Game
# ======================================================
add_executable(${PROJECT_NAME} WIN32 ${all_src_files} ${all_include_files})

# Link to the CLEAN engine library (USE_IMGUI_EDITOR=0)
target_link_libraries(${PROJECT_NAME} PRIVATE CrashoutEngine)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${project_dir}/include"
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)

# Fix for Visual Studio F5: Set Working Directory to output folder
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# ======================================================
# TARGET 2: Engine Editor
# ======================================================
add_executable(CrashoutEditor WIN32 ${all_src_files} ${all_include_files})

# Link to the EDITOR engine library (USE_IMGUI_EDITOR=1)
target_link_libraries(CrashoutEditor PRIVATE CrashoutEngine_Editor)

target_include_directories(CrashoutEditor PRIVATE 
    "${project_dir}/include"
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)

# Fix for Visual Studio F5: Set Working Directory to output folder
set_target_properties(CrashoutEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CrashoutEditor>")

# ======================================================
# Shared Build Steps (C# Scripts & Asset Sync)
# ======================================================
set(CSPROJ_PATH "${project_dir}/scripts/{{GAME_NAME}}.csproj")
add_custom_target({{GAME_NAME}}_Scripts ALL
    COMMAND dotnet build "${CSPROJ_PATH}" -c $<CONFIG>
    WORKING_DIRECTORY "${project_dir}/scripts"
)
add_dependencies(${PROJECT_NAME} {{GAME_NAME}}_Scripts)
add_dependencies(CrashoutEditor {{GAME_NAME}}_Scripts)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
add_custom_command(TARGET CrashoutEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:CrashoutEditor>/assets"
)
)";

	// --- .csproj Template ---
	const std::string CSPROJ_TEMPLATE = R"(<Project ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <Configuration Condition=" '$(Configuration)' == '' ">Debug</Configuration>
    <Platform Condition=" '$(Platform)' == '' ">AnyCPU</Platform>
    <ProjectGuid>{A1234567-ABCD-4321-9999-123456789ABC}</ProjectGuid>
    <OutputType>Library</OutputType>
    <RootNamespace>CrashoutsEngine</RootNamespace>
    <AssemblyName>{{GAME_NAME}}</AssemblyName>
    <TargetFrameworkVersion>v4.7.2</TargetFrameworkVersion>
    <LangVersion>latest</LangVersion>
    <Deterministic>false</Deterministic>
    
    <OutputPath>..\..\..\..\build\x64-$(Configuration)\scripts\{{GAME_NAME}}\</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
  </PropertyGroup>

  <ItemGroup>
    <Reference Include="System" />
    <Reference Include="System.Core" />
    <Reference Include="mscorlib" />
  </ItemGroup>

  <ItemGroup>
    <Compile Include="**\*.cs" Exclude="obj\**;bin\**" />
    
    <Compile Include="..\..\..\CrashoutEngine\scripts\**\*.cs" />
  </ItemGroup>

  <Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />
</Project>
)";

	// This function automates the "find-and-replace" and file creation.
	void ScaffoldProjectFile(const std::filesystem::path& targetPath, std::string content, const std::string& gameName) {
		size_t pos = 0;
		while ((pos = content.find("{{GAME_NAME}}", pos)) != std::string::npos) {
			content.replace(pos, 13, gameName);
			pos += gameName.length();
		}
		std::ofstream ofs(targetPath);
		if (ofs.is_open()) {
			ofs << content;
			ofs.close();
		}
	}

	// Safely writes the "start_scene" key to the current project's manifest
	void SetStartSceneInManifest(const std::string& sceneName) {
		std::string manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
		std::ifstream ifs(manifestPath);
		if (!ifs.is_open()) {
			std::cerr << "[Editor] Could not open manifest to set start scene.\n";
			return;
		}

		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document doc;
		doc.ParseStream(isw);
		ifs.close();

		if (doc.HasParseError() || !doc.IsObject()) {
			std::cerr << "[Editor] Manifest parse error.\n";
			return;
		}

		// Update or Add the "start_scene" key
		if (doc.HasMember("start_scene")) {
			doc["start_scene"].SetString(sceneName.c_str(), doc.GetAllocator());
		}
		else {
			rapidjson::Value key("start_scene", doc.GetAllocator());
			rapidjson::Value val(sceneName.c_str(), doc.GetAllocator());
			doc.AddMember(key, val, doc.GetAllocator());
		}

		// Write it back to the file with pretty formatting
		std::ofstream ofs(manifestPath);
		rapidjson::OStreamWrapper osw(ofs);
		rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
		doc.Accept(writer);

		std::cout << "[Editor] Set Start Scene to: " << sceneName << std::endl;
	}
}

namespace UI {

	// Helper local to this file
	static void DrawHierarchyNode(ECS& ecs, Entity e, Entity& selected)
	{
		std::string label = ecs.HasComponent<Component::NameComponent>(e) ? ecs.GetComponent<Component::NameComponent>(e)->name : ("Entity " + std::to_string(e));

		const bool hasKids = ecs.HasComponent<Component::ChildrenComponent>(e) && !ecs.GetComponent<Component::ChildrenComponent>(e)->children.empty();

		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanFullWidth |
			(selected == e ? ImGuiTreeNodeFlags_Selected : 0) |
			(!hasKids ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0);

		ImGui::PushID((int)e);
		const bool opened = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			selected = e;
		}

		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("HIER_ENTITY", &e, sizeof(Entity));
			ImGui::TextUnformatted(label.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("HIER_ENTITY")) {
				Entity dragged = *(const Entity*)pl->Data;
				if (dragged && dragged != e) {
					bool ok = true;
					std::vector<Entity> subtree; subtree.reserve(16);
					CrashoutsEngine::GatherDescendants(ecs, dragged, subtree);
					ok = (std::find(subtree.begin(), subtree.end(), e) == subtree.end());
					if (ok) CrashoutsEngine::Reparent(ecs, dragged, e);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem("node_ctx")) {
			if (ImGui::MenuItem("Ungroup (detach)"))
				CrashoutsEngine::DetachChild(ecs, e);
			if (ImGui::MenuItem("Create Empty Child")) {
				Entity child = ecs.CreateEntity();
				if (!ecs.HasComponent<Component::NameComponent>(child))
					ecs.AddComponent<Component::NameComponent>(child, "New Child");
				if (!ecs.HasComponent<Component::ActiveComponent>(child))
					ecs.AddComponent<Component::ActiveComponent>(child, true);
				if (!ecs.HasComponent<Component::TransformComponent>(child))
					ecs.AddComponent<Component::TransformComponent>(child);
				CrashoutsEngine::AttachChild(ecs, e, child);
			}
			ImGui::EndPopup();

		}

		if (hasKids && opened) {
			auto* kids = ecs.GetComponent<Component::ChildrenComponent>(e);
			for (Entity c : kids->children)
				DrawHierarchyNode(ecs, c, selected);
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	/*!
	 @brief Renders the ECS debug panel, allowing creation, deletion, and inspection of entities.
	 @param profiler Frame profiler reference.
	 @param ecs ECS reference.
	 @param entities List of ECS entities.
	 @param selected Currently selected entity.
	*/
	void RenderEcsPanel(CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected, UIState& ui)
	{
		static std::vector<Entity> multiSel;

		ScopedWindow win("Hierarchy", LockFlags(ui.lockLayout));

		// Create entity in the REAL ECS
		if (ImGui::Button("Create Entity")) {
			Entity e = ecs.CreateEntity();
			entities.push_back(e);
			if (!ecs.HasComponent<Component::NameComponent>(e)) {
				ecs.AddComponent<Component::NameComponent>(e, "Entity " + std::to_string(e));
			}
			if (!ecs.HasComponent<Component::ActiveComponent>(e)) {
				ecs.AddComponent<Component::ActiveComponent>(e, true);
			}
			if (!ecs.HasComponent<Component::TransformComponent>(e)) {
				ecs.AddComponent<Component::TransformComponent>(e);
			}
			if (!ecs.HasComponent<Component::UUIDComponent>(e)) {
				ecs.AddComponent<Component::UUIDComponent>(e);
			}
			if (!ecs.HasComponent<Component::LayerComponent>(e)) {
				ecs.AddComponent<Component::LayerComponent>(e);
			}
			if (!ecs.HasComponent<Component::TagComponent>(e)) {
				ecs.AddComponent<Component::TagComponent>(e);
			}
		}
		ImGui::SeparatorText("Prefab Creator");
		ImGui::Text("Entity Counter: %d", ecs.GetComponentArray<Component::NameComponent>().dense.size());
		static char nameBuf[128] = { 0 };
		{
			auto names = Prefab::Names();
			if (!names.empty()) {
				static int sel = 0;
				if (sel >= (int)names.size()) sel = 0;
				if (ImGui::BeginCombo("Choose prefab", names[sel].c_str())) {
					for (int i = 0; i < (int)names.size(); ++i) {
						bool isSel = (i == sel);
						if (ImGui::Selectable(names[i].c_str(), isSel)) sel = i;
						if (isSel) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				if (ImGui::Button("Create Prefab##Prefab")) {
					Prefab::Create(ecs, names[sel]);
				}
			}
		}
		ImGui::Separator();
		ImGui::TextUnformatted("Entities");

		ImGui::Separator();

		if (entities.size() > 0) {
			bool hasSelection = (selected != 0);

			// Gray out the button if nothing is selected
			if (!hasSelection) ImGui::BeginDisabled();

			if (ImGui::Button("Clone Selected")) {
				// Double check logic just to be safe
				if (hasSelection) {
					selected = DeepCloneEntity(ecs, selected);
					// If we disabled, we must end it before returning!
					if (!hasSelection) ImGui::EndDisabled();
					return;
				}
			}

			if (!hasSelection) ImGui::EndDisabled();

			const bool delPressed =
				ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
				ImGui::IsKeyPressed(ImGuiKey_Delete, false) &&
				!ImGui::GetIO().WantTextInput &&
				!ImGui::IsAnyItemActive();

			ImGui::SameLine();

			if (ImGui::Button("Destroy Selected") || delPressed)
			{
				if (selected)
				{
					// Use undoable delete command
					auto cmd = std::make_unique<DeleteEntityCommand>(selected);
					cmd->Execute(); // Perform delete
					UndoManager::GetInstance().AddCommand(std::move(cmd));

					// Update local entities list if necessary (remove invalid IDs)
					entities.erase(std::remove_if(entities.begin(), entities.end(),
						[&](Entity e) { return !ecs.HasComponent<Component::NameComponent>(e); }), entities.end());

					selected = 0;
				}
			}

		}
		ImGui::Text("Selected Entity: %d", selected);
		ImGui::Separator();
		{
			auto& tArr = ecs.GetComponentArray<Component::TransformComponent>();
			bool anyDrawn = false;
			for (Entity e : tArr.indexToEntity)
			{
				if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
				const bool hasParent = ecs.HasComponent<Component::ParentComponent>(e);
				if (!hasParent) {
					DrawHierarchyNode(ecs, e, selected);
					anyDrawn = true;
				}
			}
			if (!anyDrawn) ImGui::TextDisabled("No entities created yet.");
		}

		if (!selected) return;

		static char prefabNameBuf[64] = "MyBox";
		ImGui::SetNextItemWidth(220);
		ImGui::InputTextWithHint("##save_prefab_name", "Prefab name...", prefabNameBuf, IM_ARRAYSIZE(prefabNameBuf));
		ImGui::SameLine();
		if (ImGui::Button("Save Prefab")) {
			namespace fs = std::filesystem;

			std::string newName;
			if (prefabNameBuf[0]) {
				newName = prefabNameBuf;
			}
			else if (ecs.HasComponent<Component::PrefabInstanceComponent>(selected)) {
				auto* inst = ecs.GetComponent<Component::PrefabInstanceComponent>(selected);
				newName = inst ? inst->prefabName : "Entity";
			}
			else {
				newName = "Entity";
			}

#if USE_IMGUI_EDITOR
			// Save prefab into the current game's project folder
			fs::path jsonPath = fs::path(GetGameRoot())
				/ "assets"
				/ "prefabs"
				/ (newName + ".json");
#else
			fs::path jsonPath = fs::path(GetLocalAppData())
				/ "prefabs"
				/ (newName + ".json");
#endif

			fs::create_directories(jsonPath.parent_path());

			const bool ok = CrashoutsEngine::SavePrefabToJson(
				ecs,
				selected,
				newName,
				jsonPath.generic_string(),
				/*matchName*/ newName
			);

			if (ok) {
#if USE_IMGUI_EDITOR
				const fs::path dirPath = fs::path(GetGameRoot()) / "assets" / "prefabs";
#else
				const fs::path dirPath = fs::path(GetLocalAppData()) / "prefabs";
#endif
				CrashoutsEngine::ReloadPrefabsFromDisk(dirPath.generic_string());
				CrashoutsEngine::RecreateInstancesForPrefab(ecs, newName);

				prefabNameBuf[0] = '\0';
			}

		}

		//ImGui::End();
	}

	/*!
	 @brief Draw a Unity-like keyframe timeline for the selected entity
	 @param ecs ECS reference.
	 @param ui UI state reference.
	 @param selected Currently selected entity.
	*/
	void RenderAnimationPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui)
	{
		using namespace KeyframeUnity;
		using namespace KeyframeUnityEditor;
		using CrashoutsEngine::Entity;

		if (!ui.showAnimation) return;

		ScopedWindow w("Animation", LockFlags(ui.lockLayout));

		if (!selected) {
			ImGui::TextDisabled("No entity selected.");
			return;
		}

		auto* anim = ecs.GetComponent<KeyframeAnimatorComponent>(selected);
		if (!anim) {
			ImGui::TextDisabled("Entity has no KeyframeAnimatorComponent.");
			ImGui::Text("Add it in Inspector first.");
			return;
		}


		AnimationClip* clip = nullptr;
		if (!anim->current.empty()) {
			auto it = anim->clips.find(anim->current);
			if (it != anim->clips.end())
				clip = &it->second;
		}

		if (!clip) {
			ImGui::TextDisabled("Animator has no active clip.");
			ImGui::Text("Use your code to SetClip() and Play().");
			return;
		}

		const float length = clip->Length();
		ImGui::Text("Clip: %s (%.2fs)", clip->name.c_str(), length);
		ImGui::Separator();

		// --- Timeline layout ---
		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImVec2 origin = ImGui::GetCursorScreenPos();
		const float labelW = 140.0f;
		const float rowH = 20.0f;
		const float totalW = ImGui::GetContentRegionAvail().x;
		const float timeW = totalW - labelW;
		const int   rows = (int)clip->bindings.size();

		for (int bi = 0; bi < rows; ++bi) {
			KFBinding& b = clip->bindings[bi];
			float rowTop = origin.y + bi * rowH;
			float rowMid = rowTop + rowH * 0.5f;

			// Property label
			ImGui::SetCursorScreenPos(ImVec2(origin.x, rowTop));
			ImGui::TextUnformatted(KFPropertyName(b.property));

			// Row background
			ImU32 rowCol = IM_COL32(40, 40, 40, 255);
			draw->AddRectFilled(
				ImVec2(origin.x + labelW, rowTop),
				ImVec2(origin.x + labelW + timeW, rowTop + rowH),
				rowCol);

			// Keyframes 
			int keyCount = (int)b.curve.keys.size();
			for (int ki = 0; ki < keyCount; ++ki) {
				KFKey& k = b.curve.keys[ki];

				float t01 = (length > 0.f) ? (k.time / length) : 0.f;
				t01 = std::clamp(t01, 0.0f, 1.0f);
				float x = origin.x + labelW + t01 * timeW;
				ImVec2 p(x, rowMid);

				float size = 4.0f;
				ImVec2 diamond[4] = {
					ImVec2(p.x,        p.y - size),
					ImVec2(p.x + size, p.y),
					ImVec2(p.x,        p.y + size),
					ImVec2(p.x - size, p.y)
				};

				bool selectedKey =
					(gSel.entity == selected &&
						gSel.bindingIndex == bi &&
						gSel.keyIndex == ki);

				ImU32 col = selectedKey
					? IM_COL32(255, 200, 0, 255)
					: IM_COL32(200, 200, 200, 255);

				draw->AddConvexPolyFilled(diamond, 4, col);

				// Clickable area
				ImGui::SetCursorScreenPos(ImVec2(p.x - size - 3.f, p.y - size - 3.f));
				ImGui::InvisibleButton(
					(std::string("kf_") + std::to_string(bi) + "_" + std::to_string(ki)).c_str(),
					ImVec2((size + 3.f) * 2.f, (size + 3.f) * 2.f));

				if (ImGui::IsItemClicked()) {
					gSel.entity = selected;
					gSel.bindingIndex = bi;
					gSel.keyIndex = ki;
				}
			}
		}

		ImGui::Dummy(ImVec2(0.f, rows * rowH + 4.f));
		ImGui::Separator();

		// --- Keyframe inspector ---
		if (gSel.entity == selected &&
			gSel.bindingIndex >= 0 &&
			gSel.bindingIndex < (int)clip->bindings.size())
		{
			KFBinding& b = clip->bindings[gSel.bindingIndex];
			if (gSel.keyIndex >= 0 &&
				gSel.keyIndex < (int)b.curve.keys.size())
			{
				KFKey& k = b.curve.keys[gSel.keyIndex];

				ImGui::Text("Keyframe: %s", KFPropertyName(b.property));
				ImGui::DragFloat("Time", &k.time, 0.01f, 0.0f, length);
				ImGui::DragFloat("Value", &k.value, 0.1f);
				ImGui::DragFloat("In Tan", &k.inTan, 0.1f);
				ImGui::DragFloat("Out Tan", &k.outTan, 0.1f);

				int interp = (int)k.interp;
				const char* names[] = { "Constant", "Linear", "Cubic" };
				if (ImGui::Combo("Interpolation", &interp, names, 3)) {
					k.interp = (KFInterp)interp;
				}
			}
		}
		else {
			ImGui::TextDisabled("Click a keyframe diamond to edit it.");
		}
	}

	/*!
	@brief Renders the Inspector debug panel.
	@param ecs ECS reference.
	@param selected Currently selected entity.
	@param ui UI state toggles.
	*/
	void RenderInspectorPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui)
	{
		if (!ui.showInspector) return;
		ScopedWindow w("Inspector", LockFlags(ui.lockLayout));

		if (!selected) {
			ImGui::TextDisabled("No entity selected.");
			return;
		}

		ImGui::Text("Selected Entity: %u", (unsigned)selected);
		ImGui::SeparatorText("Add Components");

		// Unity-style button
		if (ImGui::Button("Add Component..."))
			ImGui::OpenPopup("add_component_popup");

		// Popup with search + list
		if (ImGui::BeginPopup("add_component_popup"))
		{
			auto& reg = DebugReflection::ComponentRegistry::instance().all();

			// Search bar
			static ImGuiTextFilter filter;
			filter.Draw("Search", ImGui::GetFontSize() * 16.0f);

			ImGui::Separator();

			// Build a sorted list of components that the entity DOESN'T yet have
			std::vector<DebugReflection::IComponentAdapter*> candidates;
			candidates.reserve(reg.size());
			for (auto* ad : reg)
			{
				if (!ad->has(ecs, selected)) // hide ones already on this entity
					candidates.push_back(ad);
			}

			std::sort(candidates.begin(), candidates.end(),
				[](auto* a, auto* b) { return std::strcmp(a->name(), b->name()) < 0; });

			// Scrollable list so popup doesn’t get huge
			ImGui::BeginChild("add_comp_list", ImVec2(0, 260), true);
			for (auto* ad : candidates)
			{
				const char* label = ad->name();
				if (!filter.PassFilter(label)) // filter by text
					continue;

				if (ImGui::Selectable(label))
				{
					ad->add(ecs, selected);
					ImGui::CloseCurrentPopup(); // close after choosing one
					break;
				}
			}
			ImGui::EndChild();

			ImGui::EndPopup();
		}

		ImGui::NewLine();

		ImGui::SeparatorText("Components on Selected Entity");

		for (auto* ad : DebugReflection::ComponentRegistry::instance().all()) {
			if (!ad->has(ecs, selected)) continue;

			ImGui::PushID(ad);

			// --- 1. SETUP COLLAPSIBLE HEADER ---
			// Framed makes it look like a distinct block (like Unity).
			// AllowItemOverlap lets us put the Remove button on the same line.
			ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_AllowOverlap;

			ImGui::Separator();
			//ImGui::TextUnformatted();

			bool isOpen = ImGui::TreeNodeEx((void*)ad, headerFlags, "%s", ad->name());

			// --- 2. RIGHT-ALIGNED REMOVE BUTTON ---
			// Safety Check: We hide the Remove button for "Core Settings" 
			// so users don't accidentally delete the Name and UUID!
			bool isCoreSettings = (std::strcmp(ad->name(), "Core Settings") == 0);

			if (!isCoreSettings) {
				float remove_w = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
				// Calculate position pushed to the far right edge
				float rightAlignX = ImGui::GetWindowWidth() - remove_w - ImGui::GetStyle().WindowPadding.x;

				ImGui::SameLine(rightAlignX);

				if (ImGui::SmallButton("Remove")) {
					ad->remove(ecs, selected);

					// Cleanup ImGui state before breaking the loop
					if (isOpen) ImGui::TreePop();
					ImGui::PopID();
					break;
				}
			}

			// --- 3. DRAW INNER COMPONENT UI ---
			if (isOpen) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
				ImGui::Spacing(); // Add a little breathing room at the top

				ad->drawInspector(ecs, selected);

				ImGui::Spacing(); // Add a little breathing room at the bottom
				ImGui::PopStyleVar();

				// TreeNodeEx requires a TreePop if it evaluates to true!
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}

	/*!
	 @brief Renders the performance panel with profiler results.
	 @param profiler Frame profiler reference.
	 @param ui UI state toggles.
	*/
	void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui) {
		if (!ui.showPerf) return;

		ScopedWindow w("Performance", LockFlags(ui.lockLayout));

		if (profiler.present_ready()) {
			gPerfSnapshot = profiler.results();
			gPerfSnapshotTotalMs = profiler.total_ms();
		}

		auto& frc = FrameRateController::GetInstance();
		const double fps = frc.FPS();
		const double frame_ms = (fps > 0.0) ? (1000.0 / fps) : 0.0;
		//const int cap = frc.CAP();

		ImGui::Text("Frame: %.3f ms (%.1f FPS)", frame_ms, fps);
		ImGui::Separator();

		// FPS Controls
		int currentCap = frc.CAP();

		ImGui::Text("Target FPS:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		// 0 to 240 range. 0 = Uncapped.
		if (ImGui::DragInt("##FPSLimit", &currentCap, 1, 0, 240, "%d FPS")) {
			if (currentCap < 0) currentCap = 0;
			frc.SetTargetFPS(currentCap);
		}

		// Helper buttons for quick setting
		if (ImGui::Button("VSync (60)")) frc.SetTargetFPS(60);
		ImGui::SameLine();
		if (ImGui::Button("120")) frc.SetTargetFPS(120);
		ImGui::SameLine();
		if (ImGui::Button("Uncapped")) frc.SetTargetFPS(0);

		ImGui::Separator();

		ImGui::Text("Draw Calls: %u", Component::Rendering::Get().drawCalls);

		ImGui::Separator();

		if (ImGui::BeginTable("perf_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("System");
			ImGui::TableSetupColumn("ms");
			ImGui::TableSetupColumn("% of frame");
			ImGui::TableHeadersRow();

			for (const auto& s : gPerfSnapshot) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(s.name.c_str());
				ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", s.ms);
				ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f%%", s.pct);
				ImGui::ProgressBar(float(s.pct / 100.0), ImVec2(-1, 0), "");
			}
			ImGui::EndTable();
		}
		DrawEngineMemoryPanel();      // memory view
		DrawProfilerPanel(profiler);
	}

	/*!
	 @brief Renders the console log viewer panel.
	 @param ui UI state toggles.
	*/
	void RenderConsolePanel(UIState& ui) {
		if (!ui.showConsole) return;

		ScopedWindow w("Console", LockFlags(ui.lockLayout));

		// Dynamic filters driven by the X-macro lists - I don't even know if this is like the socially acceptable way of doing it
		static bool levelOn[] = {
		#define X(n) true,
			LOG_LEVELS(X)
		#undef X
		};
		static bool systemOn[] = {
		#define X(n) true,
			LOG_SYSTEMS(X)
		#undef X
		};
		static char search[128] = { 0 };
		static bool autoscroll = true;

		// Level row
		ImGui::TextDisabled("Levels:");
		int li = 0;
#define X(n) do { if (li > 0) ImGui::SameLine(); ImGui::Checkbox(#n, &levelOn[li++]); } while(0);
		LOG_LEVELS(X)
#undef X

			// Subsystems row (simple grid)
			ImGui::Separator();
		ImGui::TextDisabled("Subsystems:");
		int si = 0;
		const int cols = 4;
#define X(n) do { if (si % cols != 0) ImGui::SameLine(); ImGui::Checkbox(#n, &systemOn[si++]); } while(0);
		LOG_SYSTEMS(X)
#undef X

			// Search & options
			ImGui::Separator();
		ImGui::InputTextWithHint("##search", "search text...", search, sizeof(search));
		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &autoscroll);

		ImGui::Separator();

		// Log list (virtualized)
		const auto& logs = Logger::Instance().Get();

		// Build filtered view (indices so we can clip)
		static std::vector<int> filtered;
		filtered.clear();
		filtered.reserve(logs.size());
		for (int i = 0; i < (int)logs.size(); ++i) {
			const auto& log = logs[i];
			if (!levelOn[(int)log.level])  continue;
			if (!systemOn[(int)log.system]) continue;
			if (search[0] && std::string(log.text).find(search) == std::string::npos) continue;
			filtered.push_back(i);
		}

		ImGuiListClipper clipper;
		clipper.Begin((int)filtered.size());
		while (clipper.Step()) {
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
				const auto& log = logs[filtered[row]];
				// Color by level
				const ImVec4 color =
					(log.level == LogMessage::Level::Error) ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
					(log.level == LogMessage::Level::Warning) ? ImVec4(1.0f, 0.9f, 0.4f, 1.0f) :
					(log.level == LogMessage::Level::Success) ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) :
					ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);

				// Prefix + message (avoid fmt when not needed)
				ImGui::Text("[%s][%s] %s", ToString(log.level), ToString(log.system), log.text.c_str());

				ImGui::PopStyleColor();
			}
		}

		// Controls
		static size_t lastLogSize = 0;
		bool newLogsArrived = (logs.size() > lastLogSize);
		lastLogSize = logs.size();

		// ... [Draw Filtered Logs] ...

		// --- SMART AUTO SCROLL ---
		// Only scroll if we are at the bottom OR if new logs just arrived while we were at the bottom.
		if (autoscroll && newLogsArrived)
		{
			ImGui::SetScrollHereY(1.0f);
		}

		if (ImGui::Button("Clear")) {
			Logger::Instance().Clear();
		}
	}

	/*!
	 @brief Renders the gameState panel
	 @param state gameState reference.
	 @param ui UI state toggles.
	*/
	void RenderGameStatePanel(UIState& ui) {
		if (!ui.showGameState) return;

		ScopedWindow w("GameState", LockFlags(ui.lockLayout));

		auto* P = DebugReflection::GetGameStateProvider();
		if (!P) { ImGui::TextDisabled("No GameStateProvider registered."); return; }

		auto nameFor = [&](int id) -> const char* {
			for (auto const& kv : P->states()) {
				if (kv.second == id) return kv.first;
			}
			return "(unknown)";
			};

		const int cur = P->current() ? P->current() : -1;
		const int prev = P->previous() ? P->previous() : -1;
		//const int next = P->next() ? P->next() : -1;

		ImGui::Text("Current:  %s", nameFor(cur));
		ImGui::Text("Previous: %s", nameFor(prev));

		if (cur != GameState::SCENE && !currSceneName.empty()) currSceneName.clear();

		ImGui::Separator();
		int col = 0;
		for (auto const& [label, id] : P->states()) {
			if (ImGui::Button(label)) P->changeTo(id);
			if (++col % 10 != 0) ImGui::SameLine();
		}

		ImGui::Separator();

		// Utility Action
		if (ImGui::Button("Restart Current State")) {
			RestartGameState();
		}
		ImGui::SameLine();
		if (ImGui::Button("Quit")) {
			glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
		}
	}

	/*!
	 @brief Renders the scene panel
	 @param ui UI state toggles.
	*/
	/*!
	 @brief Renders the scene panel
	 @param ui UI state toggles.
	*/
	void RenderGameScenePanel(UIState& ui) {

		if (!ui.showGameScene) return;

		ScopedWindow w("GameScene", LockFlags(ui.lockLayout));

		ImGui::Separator();

		// 1. Buffer to store the scene name from the text field
		static char sceneNameBuffer[128] = "";

		// 2. Draw the text input field. 
		//    We use "##" to hide the label but keep a unique ID
		ImGui::InputText("##SceneNameInput", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));
		//    Add a hint if the buffer is empty
		if (sceneNameBuffer[0] == '\0') {
			ImGui::SameLine();
			ImGui::TextDisabled("Enter new scene name...");
		}

		// 3. Draw the "Create Scene" button
		ImGui::SameLine();
		if (ImGui::Button("Create Scene")) {
			// Only create if the name field is not empty
			if (std::strlen(sceneNameBuffer) > 0) {

				std::string newSceneName = sceneNameBuffer;

				// SAFETY CHECK: Prevent overwriting the current scene if the user typed the same name
				if (newSceneName == currSceneName) {
					// Log error or open a popup saying "Scene already exists/is active"
					std::cerr << "[Editor] Cannot create scene with same name as active scene." << std::endl;
				}
				else {
					// STEP 1: Save the OLD scene first (if valid) so we don't lose progress
					if (!currSceneName.empty()) {
						Serialization::SaveScenes(currSceneName);
						std::cout << "[Editor] Saved previous scene: " << currSceneName << std::endl;
					}

					// STEP 2: Wipe the ECS to create a blank slate
					ECS::getInstance()->Reset();

					// STEP 3: Save this NEW (empty) state to the NEW file
					Serialization::SaveScenes(newSceneName);

					// STEP 4: Update the Engine's current state tracking
					// We are now "living" in the new scene
					currSceneName = newSceneName;

					// Clear input
					sceneNameBuffer[0] = '\0';
				}
			}
		}
		// --- End New UI ---

		ImGui::SeparatorText("Loaded Scenes (Right-Click to set Start Scene)");

		// --- NEW: SYNC MANIFEST BUTTON ---
		if (ImGui::Button("Refresh / Sync Missing Scenes")) {
			std::string manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
			std::string scenesDir = GetGameRoot() + "/assets/scenes";

			std::ifstream ifs(manifestPath);
			if (ifs.is_open()) {
				rapidjson::IStreamWrapper isw(ifs);
				rapidjson::Document doc;
				doc.ParseStream(isw);
				ifs.close();

				if (!doc.HasParseError() && doc.HasMember("scenes") && doc["scenes"].IsArray()) {
					rapidjson::Value& scenesArray = doc["scenes"];

					// Iterate backwards so we can safely erase elements
					for (int i = scenesArray.Size() - 1; i >= 0; --i) {
						if (scenesArray[i].HasMember("name")) {
							std::string sceneName = scenesArray[i]["name"].GetString();
							std::filesystem::path fullPath = std::filesystem::path(scenesDir) / sceneName;

							// If the file no longer exists on disk, remove it from the manifest
							if (!std::filesystem::exists(fullPath)) {
								std::cout << "[Editor] Removing deleted scene from manifest: " << sceneName << std::endl;
								scenesArray.Erase(scenesArray.Begin() + i);
							}
						}
					}

					// Save the cleaned manifest back to disk
					std::ofstream ofs(manifestPath);
					rapidjson::OStreamWrapper osw(ofs);
					rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
					doc.Accept(writer);
				}
			}
		}

		// Draw all scene buttons
		std::ifstream ifs(Assets::assetManifestPath);
		if (!ifs.is_open()) return;

		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document doc;
		doc.ParseStream(isw);

		if (doc.HasParseError()) return;

		// --- NEW: Identify the current start scene ---
		std::string currentStartScene = "";
		if (doc.HasMember("start_scene") && doc["start_scene"].IsString()) {
			currentStartScene = doc["start_scene"].GetString();
		}
		else if (doc.HasMember("scenes") && doc["scenes"].IsArray() && doc["scenes"].Size() > 0) {
			// Fallback to first scene if key doesn't exist yet
			if (doc["scenes"][0].HasMember("name")) {
				currentStartScene = doc["scenes"][0]["name"].GetString();
			}
		}

		if (doc.HasMember("scenes") && doc["scenes"].IsArray()) {
			const rapidjson::Value& sceneArray = doc["scenes"];
			std::unordered_set<std::string> seenSceneNames;
			seenSceneNames.reserve(sceneArray.Size());

			ImGuiStyle& style = ImGui::GetStyle();
			float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			bool first_button = true;

			for (rapidjson::SizeType i = 0; i < sceneArray.Size(); ++i) {
				const rapidjson::Value& asset = sceneArray[i];
				if (asset.HasMember("name") && asset.HasMember("path")) {
					std::string name = asset["name"].GetString();
					std::string relativePath = asset["path"].GetString();
					if (!seenSceneNames.insert(name).second) continue;

					// Highlight the starting scene with a star and special ID
					bool isStartScene = (name == currentStartScene);
					std::string btnText = isStartScene ? ("[*] " + name) : name;
					std::string btnId = btnText + "##scene_" + relativePath + "_" + std::to_string((int)i);

					ImVec2 text_size = ImGui::CalcTextSize(btnText.c_str());
					float expected_button_width = text_size.x + style.FramePadding.x * 2.0f;

					if (!first_button) {
						float last_button_x2 = ImGui::GetItemRectMax().x;
						float next_button_x2 = last_button_x2 + style.ItemSpacing.x + expected_button_width;
						if (next_button_x2 < window_visible_x2) ImGui::SameLine();
					}
					first_button = false;

					// Change color for the Start Scene so it stands out
					if (isStartScene) {
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
					}

					// Draw the button
					if (ImGui::Button(btnId.c_str())) {
						if (!currSceneName.empty() && engineState == EngineState::STOP) {
							Serialization::SaveScenes(currSceneName);
						}
						g_PendingSceneName = name;
						g_IsSceneLoadPending = true;

						if (engineState != EngineState::PLAY) {
							::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(true);
						}
					}

					// --- NEW: Right-Click Context Menu ---
					if (ImGui::BeginPopupContextItem(("Ctx_" + name).c_str())) {
						if (ImGui::MenuItem("Set as Start Scene")) {
							SetStartSceneInManifest(name);
						}
						ImGui::EndPopup();
					}

					if (isStartScene) {
						ImGui::PopStyleColor(2);
					}
				}
			}
		}
	}

	/*!
	 @brief Renders the physics panel, including collision event listings and controls.
	 @param profiler Frame profiler reference.
	 @param ecs ECS reference.
	 @param phys Physics handler reference.
	 @param ui UI state toggles.
	*/
	void RenderPhysicsPanel([[maybe_unused]] FrameProfiler& profiler, ECS& ecs, UIState& ui)
	{
		if (!ui.showPhysics) return;
		ScopedWindow w("Physics / Collision", LockFlags(ui.lockLayout));

		ImGui::SameLine();
		if (ImGui::Button("Step")) {
			::Physics::PhysicsHandler::StepOnce(); Help("Advance one simulation tick when paused.");
		}

		ImGui::Checkbox("Freeze on collision", &ui.freezeOnCollision);
		ImGui::SameLine(); Help("Automatically pause when any collision is detected this frame.");

		static bool overrideDt = false;
		static double dbgDt = 1.0 / 30.0;
		ImGui::Checkbox("Override dt", &overrideDt); ImGui::SameLine();
		if (overrideDt && ImGui::DragScalar("dt", ImGuiDataType_Double, &dbgDt, 0.001f, nullptr, nullptr, "%.4f")) {
			::Physics::PhysicsHandler::SetOverrideDt(true);
			::Physics::PhysicsHandler::SetDebugDt(dbgDt);
		}
		else if (!overrideDt) {
			::Physics::PhysicsHandler::SetOverrideDt(false);
		}

		ImGui::Separator();

		ImGui::TextDisabled("Forces & Integrators");

		for (auto* p : DebugReflection::PhysicsRegistry::instance().all()) {
			if (!p) continue;
			if (ImGui::TreeNode(p->name())) {
				p->draw(ecs); // <- use the ecs param from this function
				ImGui::TreePop();
			}
		}

		ImGui::Separator();

		const auto& evs = Collision::CollisionHandler::GetInstance().GetFrameCollisions();

		ImGui::Separator();

		ImGui::Checkbox("Draw overlay", &ui.drawCollOverlay);
		ImGui::SameLine(); ImGui::Checkbox("All colliders", &ui.drawAllColliders);
		ImGui::SameLine(); ImGui::Checkbox("QuadTree", &ui.drawQuadTree);
		ImGui::SameLine(); ImGui::Checkbox("Script Debug Draw", &ui.drawGameDebug);

		ImGui::Separator();

		Collision::CollisionHandler& collisionHandler{ Collision::CollisionHandler::GetInstance() };

		f32 newMin[2]{ collisionHandler.quadTree.boundary.min.x, collisionHandler.quadTree.boundary.min.y };
		f32 newMax[2]{ collisionHandler.quadTree.boundary.max.x, collisionHandler.quadTree.boundary.max.y };

		if (ImGui::DragFloat2("Min", newMin)) {
			collisionHandler.quadTree.Resize(Collision::AABBRect{ CEVec3{newMin[0], newMin[1], 0.f}, CEVec3{newMax[0], newMax[1], 0.f} });
		} if (ImGui::DragFloat2("Max", newMax)) {
			collisionHandler.quadTree.Resize(Collision::AABBRect{ CEVec3{newMin[0], newMin[1], 0.f}, CEVec3{newMax[0], newMax[1], 0.f} });
		}

		// When a way to implement the velocity line happens
		// ImGui::SameLine(); ImGui::Checkbox("Velocities", &ui.drawVelocities);

		// When a way to figure out how to make it a line instead of a whole shape
		//ImGui::DragFloat("Line thickness", &ui.overlayThickness, 0.1f, 1.0f, 8.0f, "%.1f");

		ImGui::TextDisabled("Pairs this frame: %d", (int)evs.size());
	}

	void RenderAudioPanel([[maybe_unused]] FrameProfiler& profiler, [[maybe_unused]] CrashoutsEngine::ECS& ecs, UIState& ui)
	{
		if (!ui.showAudio) return;
		ScopedWindow w("Audio", LockFlags(ui.lockLayout));

		auto& audio = Audio::AudioSystem::instance();

		// 1. DEFINE A HANDLE STRUCT
		// This removes the need to guess pointers or use magic numbers like 0xBEEF
		struct EditorHandle {
			void* ptr = nullptr;
			enum Type { None, Event, Channel, MusicTag } type = None;
		};

		static float s_panelFadeDuration = 0.5f;

		// 2. PLAY FUNCTION
		auto PlayAny = [&](const std::string& id, bool bgm = false) -> EditorHandle
			{
				EditorHandle result;
				if (id.empty()) return result;

				// FMOD STUDIO EVENT
				if (id.rfind("event:/", 0) == 0) {
					auto* inst = audio.createInstance(id);
					if (inst) {
						inst->start();
						result.ptr = inst;
						result.type = EditorHandle::Event;
					}
					return result;
				}

				// --- RAW FILE PATH ---
				std::string full = id;
				if (id.rfind("[RAW] ", 0) == 0) {
					std::string rel = id.substr(6);

					if (rel.rfind("assets/", 0) == 0 || rel.rfind("assets\\", 0) == 0) {
						full = rel;
					}
					else {
						full = "assets/audio/" + rel;
					}
				}
				else if (full.rfind("assets/", 0) != 0) {
					full = "assets/audio/" + full;
				}

				if (bgm) {
					// Play as Music (Streamed)
					audio.playMusicFileTagged("BGM", full, true);
					result.type = EditorHandle::MusicTag;
					return result;
				}

				// Play as SFX (One-Shot)
				FMOD::Channel* ch = audio.playSFXFile(full, false, 1.0f, 100.0f);
				if (ch) {
					result.ptr = ch;
					result.type = EditorHandle::Channel;
				}
				return result;
			};

		// 3. UPDATED STOP FUNCTION
		auto StopAny = [&](EditorHandle& h)
			{
				if (h.type == EditorHandle::None) return;

				if (h.type == EditorHandle::Event) {
					auto* inst = static_cast<FMOD::Studio::EventInstance*>(h.ptr);
					if (inst) {
						inst->stop(s_panelFadeDuration > 0.f ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
						inst->release();
					}
				}
				else if (h.type == EditorHandle::MusicTag) {
					audio.stopTagged("BGM", s_panelFadeDuration);
				}
				else if (h.type == EditorHandle::Channel) {
					auto* ch = static_cast<FMOD::Channel*>(h.ptr);
					if (ch) ch->stop();
				}
				h = EditorHandle{};
			};

		auto IsPlaying = [&](EditorHandle& h) -> bool {
			if (h.type == EditorHandle::None) return false;
			if (h.type == EditorHandle::MusicTag) return true;
			if (h.type == EditorHandle::Channel) {
				bool p = false;
				((FMOD::Channel*)h.ptr)->isPlaying(&p);
				return p;
			}
			if (h.type == EditorHandle::Event) {
				FMOD_STUDIO_PLAYBACK_STATE st;
				((FMOD::Studio::EventInstance*)h.ptr)->getPlaybackState(&st);
				return st != FMOD_STUDIO_PLAYBACK_STOPPED;
			}
			return false;
			};


		// INITIALIZATION
		if (!audio.isInitialized())
		{
			ImGui::TextDisabled("FMOD not initialized.");
			if (ImGui::Button("Initialize Audio (assets/audio)"))
				audio.init("assets/audio");
			return;
		}

		// RELOAD
		static bool needRefresh = false;
		if (ImGui::Button("Reload Manifest / Files"))
		{
			audio.stopAll(true);
			audio.stopAllRaw();
			audio.unloadAllBanks();
			audio.unloadAllRawSounds();
			CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
			needRefresh = true;
		}

		// BUILD EVENT LIST
		static std::vector<std::string> allEvents;
		if (needRefresh || allEvents.empty())
		{
			allEvents.clear();

			// 1. Get FMOD Studio Events
			audio.listAllEventPaths(allEvents);

			// 2. Get Raw Files directly from disk
			std::vector<std::string> rawFiles;
			audio.listRawAudioFiles(rawFiles);

			for (auto& f : rawFiles) {
				// f will look like "assets/audio/mysound.wav"
				allEvents.push_back(f);
			}

			std::sort(allEvents.begin(), allEvents.end());
			allEvents.erase(std::unique(allEvents.begin(), allEvents.end()), allEvents.end());
			needRefresh = false;
		}

		ImGui::SameLine();
		ImGui::TextDisabled("%zu items", allEvents.size());

		// COMBO SELECTOR
		auto EventCombo = [&](const char* label, std::string& sel) {
			if (ImGui::BeginCombo(label, sel.empty() ? "(select)" : sel.c_str())) {
				for (auto& e : allEvents) {
					if (ImGui::Selectable(e.c_str(), sel == e)) sel = e;
				}
				ImGui::EndCombo();
			}
			};

		ImGui::SeparatorText("Debug Settings");
		ImGui::SliderFloat("Fade Out Duration (s)", &s_panelFadeDuration, 0.0f, 5.0f);

		if (audio.isInitialized())
		{
			// Master
			float masterVol = audio.GetMasterVolume();
			if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f, "%.2f")) {
				audio.setMasterVolume(masterVol);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Reset##Master")) {
				masterVol = 1.0f;
				audio.setMasterVolume(masterVol);
			}

			// Music group
			float musicVol = audio.GetMusicVolume();
			if (ImGui::SliderFloat("Music Volume", &musicVol, 0.0f, 1.0f, "%.2f")) {
				audio.setMusicGroupVolume(musicVol);
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				Settings::SaveEditorSettings(ui);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Reset##Music")) {
				musicVol = 1.0f;
				audio.setMusicGroupVolume(musicVol);
				Settings::SaveEditorSettings(ui);
			}

			// SFX group
			float sfxVol = audio.GetSFXVolume();
			if (ImGui::SliderFloat("SFX Volume", &sfxVol,
				0.0f, 1.0f, "%.2f")) {
				audio.setSFXGroupVolume(sfxVol);
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				Settings::SaveEditorSettings(ui);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Reset##SFX")) {
				sfxVol = 1.0f;
				audio.setSFXGroupVolume(sfxVol);
				Settings::SaveEditorSettings(ui);
			}

			// Quick diagnostic buttons
			if (ImGui::Button("Log Volumes")) {
				std::cout << "[AudioPanel] Master=" << audio.GetMasterVolume()
					<< " Music=" << audio.GetMusicVolume()
					<< " SFX=" << audio.GetSFXVolume() << std::endl;
			}
			ImGui::SameLine();
			if (ImGui::Button("Unmute All")) {
				// Try unmuting common buses if present
				audio.setBusMute("bus:/", false);
				audio.setBusMute("bus:/Master", false); // harmless if not present
				audio.setMasterVolume(1.0f);
				audio.setMusicGroupVolume(1.0f);
				audio.setSFXGroupVolume(1.0f);
			}
		}
		else
		{
			ImGui::TextDisabled("Audio not initialized (init first to show volumes).");
		}

		ImGui::SeparatorText("Quick Test");

		static EditorHandle hBGM, hSFX1, hSFX2;
		static std::string pickBGM, pickSFX1, pickSFX2;

		auto DrawSmartControl = [&](const char* label, std::string& pick, EditorHandle& h, bool isBGM) {
			ImGui::PushID(label);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

			// Combo box
			EventCombo(label, pick); // Uses helper

			ImGui::SameLine();

			if (IsPlaying(h)) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
				if (ImGui::Button("Stop", ImVec2(-1, 0))) StopAny(h);
				ImGui::PopStyleColor();
			}
			else {
				bool hasSel = !pick.empty();
				if (!hasSel) ImGui::BeginDisabled();
				if (ImGui::Button("Play", ImVec2(-1, 0))) h = PlayAny(pick, isBGM);
				if (!hasSel) ImGui::EndDisabled();
			}
			ImGui::PopID();
			};

		DrawSmartControl("Background Music", pickBGM, hBGM, true);
		DrawSmartControl("SFX 1", pickSFX1, hSFX1, false);
		DrawSmartControl("SFX 2", pickSFX2, hSFX2, false);

		ImGui::Separator();
		if (ImGui::Button("Stop All (Panic)", ImVec2(-1, 0))) {
			audio.stopAll(true);
			hBGM = {}; hSFX1 = {}; hSFX2 = {};
		}

		audio.Update();
	}

	/*!
	 @brief Renders the Scene control bar on top of game viewport
	*/
	void RenderEngineStateBar(UIState& ui) {
		if (!ui.showEngineControl) return;

		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove;

		if (ImGui::Begin("Engine Controls", nullptr, window_flags)) {

			// Play Stop Pause buttons
			// Play
			if (ImGui::Button("PLAY", ImVec2(70, 25))) {
				if (engineState != EngineState::PLAY && !currSceneName.empty()) Serialization::SaveScenes(currSceneName);
				engineState = EngineState::PLAY;

				::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(false);
				CrashoutsEngine::Audio::AudioSystem::instance().setPaused(false);

				CrashoutsEngine::RenderCamera::engineCamera.SetPostiion(CEVec3{ 0.f, 0.f, 0.f });
				CrashoutsEngine::RenderCamera::engineCamera.SetZoom(1.f);
				CrashoutsEngine::RenderCamera::engineCamera.SetTargetAR(16.0f / 9.0f);
				CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(Settings::GetScreenWidth(), Settings::GetScreenHeight());

			}
			ImGui::SameLine();
			// Pause
			if (ImGui::Button("PAUSE", ImVec2(70, 25))) {
				if (engineState != EngineState::PLAY && !currSceneName.empty()) Serialization::SaveScenes(currSceneName);
				engineState = EngineState::PAUSE;

				::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(true);
				CrashoutsEngine::Audio::AudioSystem::instance().setPaused(true);
			}
			ImGui::SameLine();
			// Stop
			if (ImGui::Button("STOP", ImVec2(70, 25))) {
				// If we were in STOP or PAUSE, save our tweaks before reloading
				if (engineState != EngineState::PLAY && !currSceneName.empty()) Serialization::SaveScenes(currSceneName);

				if (currSceneName.empty()) RestartGameState();       // Legacy scene system
				else {
					g_PendingSceneName = currSceneName;
					g_IsSceneLoadPending = true;
				}
				//MonoManager::ReloadScripts();
				engineState = EngineState::STOP;

				::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(true);

				auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
				audio.stopAll(false);          // Stop FMOD Studio Events (Immediate, no fade)
				audio.stopAllChannels();       // Stop FMOD Core Sounds (Music & SFX)
				audio.setPaused(false);        // Unpause the system for the editor

			}
			ImGui::SameLine();
			// --- ADD THE RECOMPILE BUTTON ---
			// We highlight it in a slightly different color so it stands out
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
			if (ImGui::Button("Recompile C#", ImVec2(100, 25))) {

				// 1. Figure out which game is currently active
				std::string gameName = std::filesystem::path(GetGameRoot()).filename().string();
				std::filesystem::path csprojPath = std::filesystem::path(GetGameRoot()) / "scripts" / (gameName + ".csproj");

				// 2. Command Windows to compile it
				std::string buildCmd = "dotnet build \"" + csprojPath.generic_string() + "\"";
				std::cout << "[Editor] Recompiling scripts for " << gameName << "...\n";

				int result = std::system(buildCmd.c_str());

				if (result == 0) {
					std::cout << "[Editor] Compile Success! Hot Reloading...\n";

					// Backs up all script variables, drops the AppDomain, 
					// loads the new C# code, and restores your variables automatically.
					CrashoutsEngine::ScriptSystem::ReloadAll();
				}
				else {
					std::cerr << "[Editor] Compile Failed! Check your C# syntax.\n";
				}
			}
			ImGui::PopStyleColor();
			// --------------------------------

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			const char* stateText = "UNKNOWN";
			switch (engineState) {
			case EngineState::PLAY:  stateText = "PLAYING"; break;
			case EngineState::PAUSE: stateText = "PAUSED";  break;
			case EngineState::STOP:  stateText = "STOPPED"; break;
			}
			ImGui::TextDisabled("State: %s", stateText);
		}

		ImGui::End();
	}

	/*!
	 @brief Renders a panel to create new game projects and manage the current project root.
	*/
	void RenderProjectManager(UIState& ui) {
		ScopedWindow w("Project Manager", LockFlags(ui.lockLayout));
		namespace fs = std::filesystem;

		ImGui::Text("Current Project Root:");
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", GetGameRoot().c_str());

		ImGui::Separator();

		// =========================================================================
		// HELPER LAMBDA: Hot-Swaps the engine to a new project & loads Start Scene
		// =========================================================================
		auto SwitchProjectContext = [](const std::string& projName, const std::string& projPath) {
			Audio::AudioSystem& audio = Audio::AudioSystem::instance();
			audio.stopAll(true);
			audio.unloadAllBanks();
			audio.unloadAllRawSounds();

			CrashoutsEngine::ScriptSystem::DestroyAll();
			CrashoutsEngine::ECS::getInstance()->Reset();
			CrashoutsEngine::CollisionEventManager::GetInstance().Clear();

			// 1. Update Root Pointer
			SetGameRoot(projPath);

			// 2. Refresh Mono Scripts for the new project
			fs::path csprojPath = fs::path(projPath) / "scripts" / (projName + ".csproj");
			std::string buildCommand = "dotnet build \"" + csprojPath.generic_string() + "\"";

			std::cout << "[Editor] Compiling scripts for " << projName << "...\n";
			int buildResult = std::system(buildCommand.c_str());

			if (buildResult == 0) {
				std::cout << "[Editor] Script compilation successful!\n";
			}
			else {
				std::cerr << "[Editor] Script compilation failed. Check your C# code!\n";
			}

			// 3. Refresh Mono Scripts
			MonoManager::ReloadAssembly();
			CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

			// 4. Refresh Asset Manager & Manifest
			auto& am = CrashoutsEngine::Assets::AssetManager::GetInstance();
			am.Shutdown(); // Soft Reset

			std::string newMPath = GetGameRoot() + "/assets/json/assetmanifest.json";
			CrashoutsEngine::Assets::GenerateAssetManifest(newMPath);
			am.LoadManifestAssets(newMPath);

			// Reload Prefabs
			std::string newPrefabPath = GetGameRoot() + "/assets/prefabs";
			CrashoutsEngine::ReloadPrefabsFromDisk(newPrefabPath);

			// 5. LOAD THE STARTING SCENE DYNAMICALLY
			// This safely falls back to "StartScene" if no scene is found
			g_PendingSceneName = GetStartingSceneName();
			g_IsSceneLoadPending = true;

			LOGS(Core, "Context Switched to: " + projName + " | Booting into: " + g_PendingSceneName);
			};
		// =========================================================================

		ImGui::SeparatorText("Create New Project");

		static char newProjectName[64] = "";
		ImGui::InputTextWithHint("##ProjName", "Project Name", newProjectName, IM_ARRAYSIZE(newProjectName));

		if (ImGui::Button("Create & Initialize Project")) {
			if (newProjectName[0] != '\0') {
				std::string gameName = newProjectName;
				fs::path projectPath = fs::path("..") / ".." / "projects" / "GameProjects" / gameName;
				fs::path templatePath = fs::path("..") / ".." / "projects" / "Template";

				try {
					// 1. Create Core Structure
					fs::create_directories(projectPath / "assets/images");
					fs::create_directories(projectPath / "assets/scenes");
					fs::create_directories(projectPath / "assets/json");
					fs::create_directories(projectPath / "src");
					fs::create_directories(projectPath / "include");
					fs::create_directories(projectPath / "scripts");

					std::ofstream dummyScene(projectPath / "assets/scenes/StartScene.json"); // Added .json for safety
					if (dummyScene.is_open()) {
						dummyScene << "{\n  \"Entities\": []\n}";
						dummyScene.close();
					}

					// 2. Generate Scaffolding from Raw Strings
					ScaffoldProjectFile(projectPath / "CMakeLists.txt", CMAKELISTS_TEMPLATE, gameName);
					ScaffoldProjectFile(projectPath / "src/main.cpp", MAIN_CPP_TEMPLATE, gameName);
					ScaffoldProjectFile(projectPath / ("scripts/" + gameName + ".csproj"), CSPROJ_TEMPLATE, gameName);

					// 3. Copy Starter Scripts
					if (fs::exists(templatePath / "scripts")) {
						fs::copy(templatePath / "scripts", projectPath / "scripts",
							fs::copy_options::recursive | fs::copy_options::overwrite_existing);
					}

					LOGS(Core, "Project " + gameName + " is ready!");

					// --- AUTO-LOAD THE NEW PROJECT ---
					// This will instantly switch the editor into the new project and load the StartScene!
					SwitchProjectContext(gameName, projectPath.generic_string());

					newProjectName[0] = '\0';
				}
				catch (const std::exception& e) {
					LOGE(Core, std::string("Scaffolding failed: ") + e.what());
				}
			}
		}

		ImGui::SeparatorText("Project Quick Switch");

		fs::path gamesDir = fs::path("..") / ".." / "projects" / "GameProjects";
		if (fs::exists(gamesDir)) {
			for (auto const& entry : fs::directory_iterator(gamesDir)) {
				if (entry.is_directory()) {
					std::string name = entry.path().filename().string();
					std::string currentRoot = GetGameRoot();
					bool isActive = (currentRoot.find(name) != std::string::npos);

					if (ImGui::Selectable(name.c_str(), isActive)) {
						if (isActive) {
							LOGI(Core, "Project '" + name + "' is already active.");
						}
						else {
							// --- USE THE HELPER TO QUICK SWITCH ---
							SwitchProjectContext(name, entry.path().generic_string());
						}
					}
				}
			}
		}
	}
}