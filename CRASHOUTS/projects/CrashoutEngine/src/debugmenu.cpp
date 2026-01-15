/*!
@file       debugmenu.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

@brief      Renders the editor/debug Im

for Crashout Engine.
@details    This module draws every editor-facing window:
			- engine controls (play / pause / stop)
			- game viewport (with ImGuizmo)
			- ECS / Inspector panels
			- Asset browser / drag-and-drop
			- Performance / profiler view
			- Audio test panel

			It also contains all the small, file-local editor state
			(selected entity, drag payloads, gizmo mode, etc.)
			so that the rest of the engine code stays clean.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Engine / app headers
#include "debugmenu_internals.h"
#include "assetmanager.h"
#include "settings.h"
#include <filesystem>
#include <unordered_set>
#include "prefab.h"


/*
----------------------------------------------------------------------------- */

using namespace CrashoutsEngine;

namespace UI { // ----------------------------------------------------------------------------- public UI API

	std::string& currSceneName = CrashoutsEngine::Serialization::currSceneName;

	static std::string g_IniPath;

	// -- Lifecycle ------------------------------------------------------------
	/*!
	 @brief Initializes ImGui context, allocators, and platform bindings.
	 @param window GLFW window handle.
	*/
	void Init(GLFWwindow* window, UI::UIState& uiState) {
		IMGUI_CHECKVERSION();
		// Use your existing allocator tags (kept intact)
		ImGui::SetAllocatorFunctions(
			[](size_t sz, void*) {
				ScopedMemTag tag(MemTag::UI);
				return ::operator new(sz);
			},
			[](void* p, void*) {
				ScopedMemTag tag(MemTag::UI);
				::operator delete(p);
			},
			nullptr);

		ImGui::CreateContext();
		g_IniPath = CrashoutsEngine::Assets::rootPath + "imgui.ini";

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = g_IniPath.c_str();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::StyleColorsDark();
		SetDarkGrayTheme();
		// Load custom settings
		Settings::LoadEditorSettings(uiState);

		ScopedMemTag boot(MemTag::Resources);
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		glfwSetDropCallback(window, [](GLFWwindow*, int count, const char** paths) {
			for (int i = 0; i < count; ++i) {
				if (paths[i]) gDroppedPaths.emplace_back(paths[i]);
			}
			});

		LOGS(Core, "Engine initialized successfully");
		LOGW(Core, "Debug mode is ON");
		LOGI(Core, "Welcome to Crashout Engine Debugger!");
	}

	/*!
	 @brief Shuts down ImGui and cleans up resources.
	*/
	void Shutdown(UI::UIState& uiState) {

		// Save custom engine settings
		if (engineState == EngineState::STOP) {
			Settings::SaveEditorSettings(uiState);
			ImGui::SaveIniSettingsToDisk(g_IniPath.c_str());
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	// -- Frame ownership (exactly one pair per frame) -------------------------
	/*!
	 @brief Begins a new ImGui frame.
	*/
	void BeginFrame() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	/*!
	 @brief Ends the current ImGui frame and renders all draw data.
	*/
	void EndFrame() {
		ImGui::Render();
		ImGuiIO& io = ImGui::GetIO();
		if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f)
		{
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup);
		}
	}
	static void SaveAllPrefabsInScene(ECS& ecs)
	{
		namespace fs = std::filesystem;

		std::unordered_set<std::string> saved;

		auto& instArr = ecs.GetComponentArray<Component::PrefabInstanceComponent>();

		for (Entity e : instArr.indexToEntity)
		{
			if (!ecs.HasComponent<Component::PrefabInstanceComponent>(e)) continue;

			auto* inst = ecs.GetComponent<Component::PrefabInstanceComponent>(e);
			if (!inst) continue;

			std::string prefabName = inst->prefabName; // COPY (safer)
			if (prefabName.empty()) continue;

			if (!saved.insert(prefabName).second) continue; // save each prefab once

#if USE_IMGUI_EDITOR
			fs::path jsonPath = fs::path(Assets::rootPath) / "prefabs" / (prefabName + ".json");
#else
			fs::path jsonPath = fs::path(GetLocalAppData()) / "prefabs" / (prefabName + ".json");
#endif

			fs::create_directories(jsonPath.parent_path());

			SavePrefabToJson(
				ecs,
				e,
				prefabName,
				jsonPath.generic_string(),
				/*matchName*/ prefabName
			);
		}

#if USE_IMGUI_EDITOR
		const std::string dirPath = (fs::path(Assets::rootPath) / "prefabs").generic_string();
#else
		const std::string dirPath = (fs::path(GetLocalAppData()) / "prefabs").generic_string();
#endif

		ReloadPrefabsFromDisk(dirPath);
	}


	// -- Coordinator(s) --------------------------------------------------------

	void RenderDebugMenuBar(UIState& ui) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Save Scene")) {

					LOGW(Core, "SAVE SCENE CLICKED: currSceneName='%s'", currSceneName.c_str());

					if (!currSceneName.empty())
						Serialization::SaveScenes(currSceneName);
					else
						LOGE(Core, "Save Scene failed: currSceneName is EMPTY");

					SaveAllPrefabsInScene(*ECS::getInstance());
				}


				// --- ZOOM INPUT ---
				// Display as integer percentage (50% to 300%)
				ImGui::Text("UI Zoom");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100.0f);
				if (ImGui::DragFloat("##ZoomLvl", &ui.fontScale, 0.01f)) {
					// Clamp the values to reasonable limits
					if (ui.fontScale < 0.5f) ui.fontScale = 0.5f;
					if (ui.fontScale > 2.0f) ui.fontScale = 2.0f;

					// Apply to ImGui Global Scale
					ImGui::GetIO().FontGlobalScale = ui.fontScale;
				}

				if (ImGui::MenuItem("Reset Window Layout")) {
					// delete the file and reload
					std::string path = CrashoutsEngine::Assets::rootPath + "imgui.ini";
					std::remove(path.c_str()); // Delete the file

					ImGui::ClearIniSettings();
					ImGuiID dockspace_id = ImGui::GetID("MyMainDockSpace");
					ImGui::DockBuilderRemoveNode(dockspace_id);
				}

				// --- THEME CUSTOMIZATION ---
				if (ImGui::BeginMenu("Theme Settings")) {

					if (ImGui::Button("Reset to Default Gray")) {
						SetDarkGrayTheme();
					}
					ImGui::Separator();

					// Helper lambda to make color edits cleaner
					auto ColorEdit = [](const char* label, ImGuiCol_ col) {
						ImGui::ColorEdit4(label, (float*)&ImGui::GetStyle().Colors[col], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview);
						};

					// Group: Window & Panels
					if (ImGui::TreeNode("Window & Panels")) {
						ColorEdit("Window Bg", ImGuiCol_WindowBg);
						ColorEdit("Child Bg", ImGuiCol_ChildBg);
						ColorEdit("Popup Bg", ImGuiCol_PopupBg);
						ColorEdit("Border", ImGuiCol_Border);
						ColorEdit("Border Shadow", ImGuiCol_BorderShadow);
						ColorEdit("Docking Preview", ImGuiCol_DockingPreview);
						ColorEdit("Docking Empty", ImGuiCol_DockingEmptyBg);
						ImGui::TreePop();
					}

					// Group: Text
					if (ImGui::TreeNode("Text")) {
						ColorEdit("Text", ImGuiCol_Text);
						ColorEdit("Text Disabled", ImGuiCol_TextDisabled);
						ImGui::TreePop();
					}

					// Group: Headers & Titles
					if (ImGui::TreeNode("Headers & Titles")) {
						ColorEdit("Menu Bar Bg", ImGuiCol_MenuBarBg);
						ColorEdit("Title Bg", ImGuiCol_TitleBg);
						ColorEdit("Title Bg Active", ImGuiCol_TitleBgActive);
						ColorEdit("Title Bg Collapsed", ImGuiCol_TitleBgCollapsed);
						ColorEdit("Header", ImGuiCol_Header);
						ColorEdit("Header Hovered", ImGuiCol_HeaderHovered);
						ColorEdit("Header Active", ImGuiCol_HeaderActive);
						ImGui::TreePop();
					}

					// Group: Interactables (Buttons, Tabs, Inputs)
					if (ImGui::TreeNode("Interactables")) {
						ColorEdit("Button", ImGuiCol_Button);
						ColorEdit("Button Hovered", ImGuiCol_ButtonHovered);
						ColorEdit("Button Active", ImGuiCol_ButtonActive);
						ColorEdit("Tab", ImGuiCol_Tab);
						ColorEdit("Tab Hovered", ImGuiCol_TabHovered);
						ColorEdit("Tab Active", ImGuiCol_TabActive);
						ColorEdit("Tab Unfocused", ImGuiCol_TabUnfocused);
						ColorEdit("Tab Unfocused Active", ImGuiCol_TabUnfocusedActive);
						ColorEdit("Frame Bg", ImGuiCol_FrameBg);
						ColorEdit("Frame Bg Hovered", ImGuiCol_FrameBgHovered);
						ColorEdit("Frame Bg Active", ImGuiCol_FrameBgActive);
						ColorEdit("Check Mark", ImGuiCol_CheckMark);
						ColorEdit("Slider Grab", ImGuiCol_SliderGrab);
						ColorEdit("Slider Grab Active", ImGuiCol_SliderGrabActive);
						ImGui::TreePop();
					}

					// Group: Misc
					if (ImGui::TreeNode("Misc")) {
						ColorEdit("Scrollbar Bg", ImGuiCol_ScrollbarBg);
						ColorEdit("Scrollbar Grab", ImGuiCol_ScrollbarGrab);
						ColorEdit("Scrollbar Grab Hovered", ImGuiCol_ScrollbarGrabHovered);
						ColorEdit("Scrollbar Grab Active", ImGuiCol_ScrollbarGrabActive);
						ColorEdit("Resize Grip", ImGuiCol_ResizeGrip);
						ColorEdit("Resize Grip Hovered", ImGuiCol_ResizeGripHovered);
						ColorEdit("Resize Grip Active", ImGuiCol_ResizeGripActive);
						ImGui::TreePop();
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Exit")) {
					glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows")) {
				ImGui::MenuItem("Inspector", nullptr, &ui.showInspector);
				ImGui::MenuItem("Assets", nullptr, &ui.showAssets);
				ImGui::MenuItem("Hierarchy (ECS)", nullptr, &ui.showECS);
				ImGui::MenuItem("Console", nullptr, &ui.showConsole);
				ImGui::MenuItem("Performance", nullptr, &ui.showPerf);
				ImGui::MenuItem("Physics", nullptr, &ui.showPhysics);
				ImGui::MenuItem("Audio", nullptr, &ui.showAudio);
				ImGui::MenuItem("Animation", nullptr, &ui.showAnimation);
				ImGui::MenuItem("Game State", nullptr, &ui.showGameState);
				ImGui::MenuItem("Game Scene", nullptr, &ui.showGameScene);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	/*!
	 @brief Renders all panels and overlays in a single call.
	 @param profiler Frame profiler reference.
	 @param ecs ECS reference.
	 @param phys Physics handler reference.
	 @param ui UI state toggles.
	 @param entities List of ECS entities.
	 @param selected Currently selected entity.
	*/
	void RenderAll(FrameProfiler& profiler, UIState& ui, ECS& ecs, std::vector<Entity>& entities, Entity& selected)
	{
		const auto& collisions = Collision::CollisionHandler::GetInstance().GetFrameCollisions();
		if (ui.drawCollOverlay) {

			// Get ratio based off zoom

			float thicknessRatio = 1.f;
			if (engineState == EngineState::STOP) thicknessRatio = CrashoutsEngine::RenderCamera::engineCamera.GetZoom();
			else thicknessRatio = CrashoutsEngine::RenderCamera::gameCamera.GetZoom();

			if (ui.drawAllColliders) {
				CollisionGizmos::DrawAllColliders(ecs, collisions, ui.overlayThickness / thicknessRatio);
				CollisionGizmos::DrawContactsAndNormals(*ECS::getInstance(), collisions, 3.5f, ui.overlayThickness / thicknessRatio);
			}
			if (ui.drawQuadTree) Collision::CollisionHandler::GetInstance().quadTree.Draw();

		}
		RenderDebugMenuBar(ui);

		ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Setup the transparent window for the dockspace
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		window_flags |= ImGuiWindowFlags_NoBackground; // Transparent background

		ImGui::Begin("Dockspace", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("Dockspace");

		// If it is nullptr, it means this is the First Run (or ini is missing).
		if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
		{
			// Rebuild docking layout
			ImGui::DockBuilderRemoveNode(dockspace_id);
			int builder_flags_i = (int)ImGuiDockNodeFlags_DockSpace | (int)ImGuiDockNodeFlags_NoDockingInCentralNode;
			ImGuiDockNodeFlags builder_flags = (ImGuiDockNodeFlags)builder_flags_i;
			ImGui::DockBuilderAddNode(dockspace_id, builder_flags);
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

			// Split into main zones
			ImGuiID dock_main = dockspace_id;
			ImGuiID dock_left, dock_right, dock_bottom, dock_top, dock_left_bottom, dock_right_bottom;
			ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.07f, &dock_top, &dock_main);
			ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.18f, &dock_left, &dock_main);
			ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.26f, &dock_right, &dock_main);
			ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.28f, &dock_bottom, &dock_main);
			ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, &dock_left_bottom, &dock_main);
			ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.5f, &dock_right_bottom, &dock_right);

			// Dock windows
			ImGui::DockBuilderDockWindow("Debug Controls", dock_left);
			ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
			ImGui::DockBuilderDockWindow("Sprite Sheet", dock_right);
			ImGui::DockBuilderDockWindow("Assets", dock_bottom);
			ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
			ImGui::DockBuilderDockWindow("Physics / Collision", dock_bottom);
			ImGui::DockBuilderDockWindow("Console", dock_bottom);
			ImGui::DockBuilderDockWindow("Performance", dock_left_bottom);
			ImGui::DockBuilderDockWindow("GameState", dock_bottom);
			ImGui::DockBuilderDockWindow("GameScene", dock_bottom);
			ImGui::DockBuilderDockWindow("Animation", dock_bottom);
			ImGui::DockBuilderDockWindow("Audio", dock_right_bottom);
			ImGui::DockBuilderDockWindow("Engine Controls", dock_top);
			ImGui::DockBuilderDockWindow("Game Viewport", dock_main);

			ImGui::DockBuilderFinish(dockspace_id);
		}

		if (InputManager::IsKeyHeld(Key::LeftCtrl) && InputManager::IsKeyTriggered(Key::Z))
		{
			LOGI(Core, "Undo Triggered");
			UndoManager::GetInstance().Undo();
		}
		if (InputManager::IsKeyHeld(Key::LeftCtrl) && InputManager::IsKeyTriggered(Key::Y))
		{
			LOGI(Core, "Redo Triggered");
			UndoManager::GetInstance().Redo();
		}

		if (ui.showECS) {
			RenderEcsPanel(ecs, entities, selected, ui);
		}
		if (ui.showAssets) {
			static std::string gPickedAsset;
			RenderAssetBrowserPanel(ui, Assets::rootPath.c_str(), gPickedAsset);
		}
		if (ui.showSpriteSheet) {
			RenderSpriteSheetPanel(ecs, selected);
		}
		if (ui.showInspector) {
			RenderInspectorPanel(ecs, selected, ui);
		}
		if (ui.showPhysics) {
			RenderPhysicsPanel(profiler, ecs, ui);
		}
		if (ui.showAudio) {
			RenderAudioPanel(profiler, ecs, ui);
		}
		if (ui.showPerf) {
			RenderPerformancePanel(profiler, ui);
		}
		if (ui.showConsole) {
			RenderConsolePanel(ui);
		}
		if (ui.showGameState) {
			RenderGameStatePanel(ui);
		}
		if (ui.showGameScene) {
			RenderGameScenePanel(ui);
		}
		if (ui.showEngineControl) {
			RenderEngineStateBar(ui);
		}
		if (ui.showAnimation) {
			RenderAnimationPanel(ecs, selected, ui);
		}


		//EndFrame();

#if USE_IMGUI_EDITOR
		RenderGameViewport(ecs, selected);
#endif
		ImGui::End();

	}

	/*!
	 @brief Backward-compatible wrapper for rendering without explicit UIState.
	 @param profiler Frame profiler reference.
	 @param ecs ECS reference.
	 @param phys Physics handler reference.
	 @param entities List of ECS entities.
	 @param selected Currently selected entity.
	*/
	void Render(FrameProfiler& profiler, Entity& selected, UIState& ui)
	{
		ECS* ecs = ECS::getInstance();

		std::vector<CrashoutsEngine::Entity> entities =
			ecs->GetComponentArray<Component::NameComponent>().indexToEntity;
		//std::cout << entities.size() << " ";

		//static UIState ui;
		RenderAll(profiler, ui, *ecs, entities, selected);
	}

	void SetDarkGrayTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		// Window & Panels
		colors[ImGuiCol_WindowBg] = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.08f, 0.08f, 0.6f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

		// Text
		colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

		// Menus & Title Bars
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

		// Buttons / Headers
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);

		colors[ImGuiCol_Header] = ImVec4(0.21f, 0.21f, 0.21f, 1.0f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4(0.19f, 0.19f, 0.19f, 1.0f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
		colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);

		// Frames / Inputs
		colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);

		// Scrollbars
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

		// Checkboxes / Sliders
		colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);

		// Docking
		colors[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.35f, 0.35f, 0.35f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);

		// Misc
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.30f, 0.30f, 0.5f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.7f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.50f, 0.50f, 0.9f);

		// Rounding & Borders
		style.WindowRounding = 3.0f;
		style.FrameRounding = 3.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 3.0f;
		style.ScrollbarRounding = 5.0f;

		style.FrameBorderSize = 1.0f;
		style.WindowBorderSize = 1.0f;
	}


} // namespace UI