/*!
@file    	main.cpp
@author  	pghali@digipen.edu
@co-author  parminder.singh@digipen.edu
@date    	17/04/2024

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Extension loader library's header must be included before GLFW's header!!!
#include "../include/glapp.h"
#include "../include/entitymanager.h"
#include "../include/performanceViewer.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS instance and entity storage
static ECS::ECS globalECS;
static std::vector<ECS::Entity> globalEntities;
static ECS::Entity globalSelectedEntity = 0;

// ImGui in-game performance viewer
static bool show_perf = false;
static FrameProfiler gProfiler;
static std::vector<ProfilerSample> gPerfSnapshot;
static double gPerfSnapshotTotalMs = 0.0;

// tests
static bool gOnce = false;



/*                                                      function declarations
----------------------------------------------------------------------------- */
static void draw();
static void update();
static void ImGuiDraw();
static void init();

/*                                                   objects with file scope
----------------------------------------------------------------------------- */

/*                                                      function definitions
----------------------------------------------------------------------------- */
/*  _________________________________________________________________________ */
/*! main

@param none

@return int

Indicates how the program existed. Normal exit is signaled by a return value of
0. Abnormal termination is signaled by a non-zero return value.
Note that the C++ compiler will insert a return 0 statement if one is missing.
*/
int main() {
    // start with a 16:9 aspect ratio
    if (!GLApp::init(1280, 720, "Tutorial 0 | Setting up OpenGL 4.5")) {
        std::cout << "Unable to create OpenGL context" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    init();

    // window's close flag is set by clicking close widget or Alt+F4
    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        draw();
        update();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    GLApp::cleanup();

    return EXIT_SUCCESS;
}

static void init() {
    // Setup ImGUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // docking enable just means you can move it move it
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ScopedMemTag boot(MemTag::Resources);

    // Initializing the backends
    ImGui_ImplGlfw_InitForOpenGL(GLApp::ptr_window, true);
    //ImGui_ImplOpenGL3_Init("#version 450");
    ImGui_ImplOpenGL3_Init("#version 330");
}

/*  _________________________________________________________________________ */
/*! draw

@param none

@return none

Call application to draw and then swap front and back frame buffers ...
Uses GLHelper::GLFWWindow* to get handle to OpenGL context.
*/
static void draw() {
    FrameProfiler::Scoped s(gProfiler, "Graphics/Scene");
    ScopedMemTag mem(MemTag::Graphics);
    // render scene
    GLApp::draw();

    // Render ImGui UI on top of the scene
    ImGuiDraw();


    // end of frame profiling
    gProfiler.end_frame();

    // swap buffers: front <-> back
    // GLApp::ptr_window is handle to window that defines the OpenGL context
    glfwSwapBuffers(GLApp::ptr_window);
}

/*  _________________________________________________________________________ */
/*! update

@param none

@return none

Make sure callbacks are invoked when state changes in input devices occur.
Ensure time per frame and FPS are recorded.
Let application update state changes (such as animation).
*/
static void update() {
    // process events if any associated with input devices
    glfwPollEvents();

    // start frame profiling
    gProfiler.begin_frame();

    // main loop computes fps and other time related stuff once for all apps ...
    GLApp::update_time(1.0);

    FrameProfiler::Scoped s(gProfiler, "Game/Update");
    ScopedMemTag mem(MemTag::Update);
    // place any per-system updates here (ECS, AI, Physics, etc...)

    // testing
    /*
    if (!gOnce) {
        gOnce = true;

        // 1) observe counters before
        auto beforeAlloc = MemTracker::instance().allocs[(size_t)MemTag::Resources].load();
        auto beforeLive = MemTracker::instance().live[(size_t)MemTag::Resources].load();

        // 2) do a tagged allocation that MUST go through our global operator new
        {
            ScopedMemTag tag(MemTag::Resources);           // set TLS tag:contentReference[oaicite:3]{index=3}
            void* p = ::operator new(1024 * 1024);         // 1 MB
            // keep it alive for a bit so UI can see it
            static void* hold = p;
        }

        // 3) read counters after
        auto afterAlloc = MemTracker::instance().allocs[(size_t)MemTag::Resources].load();
        auto afterLive = MemTracker::instance().live[(size_t)MemTag::Resources].load();

        // 4) log to console
        std::cout << "[MemTest] allocs(Resources): " << beforeAlloc << " -> " << afterAlloc
            << " | live(Resources): " << beforeLive << " -> " << afterLive << std::endl;
    }*/


    // animate scene
    GLApp::update();
}


/* _________________________________________________________________________ */
/*! ImGuiDraw

@param none

@return none

This function handles all the ImGui UI drawing for the ECS demonstration. It creates a main window
with buttons to manage entities and their components.
*/
static void ImGuiDraw() {
    using namespace ECS;

    FrameProfiler::Scoped _imgui(gProfiler, "ImGui");

    static bool lockLayout = false;


    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags lockFlags = lockLayout ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking) : ImGuiWindowFlags_None;

    // Create ImGui window
    ImGui::Begin("ECS Demo", nullptr, lockFlags);

    if (ImGui::Button(lockLayout ? "Unlock" : "Lock"))
    {
        lockLayout = !lockLayout;
    }

    // Create a new entity
    if (ImGui::Button("Create Entity")) {
        Entity newEntity = globalECS.CreateEntity();
        globalEntities.push_back(newEntity);
        if (!globalECS.HasComponent<NameComponent>(newEntity)) {
            globalECS.AddComponent<NameComponent>(newEntity, "Entity " + std::to_string(newEntity));
        }
    }

    ImGui::Separator();

    // Toggle Button to show performance
    if (ImGui::Button(show_perf ? "Hide Performance" : "Show Performance")) {
        show_perf = !show_perf;
    }

    ImGui::Separator();

    // List all entities
    ImGui::Text("Entities");
    if (globalEntities.empty()) {
        ImGui::Text("No entities created yet.");
    }
    else {
        for (const auto& entity : globalEntities) {
            std::string entity_label;
            if (globalECS.HasComponent<NameComponent>(entity)) {
                NameComponent* nameComp = globalECS.GetComponent<NameComponent>(entity);
                entity_label = nameComp->name;
            }
            else {
                entity_label = "Entity: " + std::to_string(entity);
            }
            if (ImGui::Selectable(entity_label.c_str(), globalSelectedEntity == entity)) {
                globalSelectedEntity = entity;
            }
        }
    }

    ImGui::Separator();

    // Actions for the selected entity
    ImGui::Text("Selected Entity: %d", globalSelectedEntity);

    if (globalSelectedEntity != 0) {
        if (ImGui::Button("Destroy Selected")) {
            globalECS.DestroyEntity(globalSelectedEntity);
            for (size_t i = 0; i < globalEntities.size(); ++i) {
                if (globalEntities[i] == globalSelectedEntity) {
                    globalEntities.erase(globalEntities.begin() + i);
                    break;
                }
            }
            globalSelectedEntity = 0;
        }

        ImGui::Separator();
        ImGui::Text("Add Components");
        if (ImGui::Button("Add NameComponent")) {
            if (!globalECS.HasComponent<NameComponent>(globalSelectedEntity)) {
                globalECS.AddComponent<NameComponent>(globalSelectedEntity, "Unnamed Entity");
            }
        }
        if (ImGui::Button("Add TransformComponent")) {
            if (!globalECS.HasComponent<TransformComponent>(globalSelectedEntity)) {
                globalECS.AddComponent<TransformComponent>(globalSelectedEntity, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Components on Selected Entity");
    if (globalSelectedEntity != 0) {
        // Display NameComponent
        NameComponent* nameComp = globalECS.GetComponent<NameComponent>(globalSelectedEntity);
        if (nameComp) {
            char buffer[256];
            strncpy(buffer, nameComp->name.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';      // Ensure null termination

            ImGui::Text("Name: ");
            if (ImGui::InputText("##Name", buffer, sizeof(buffer))) {
                nameComp->name = buffer;
            }
        }

        // Display TransformComponent
        TransformComponent* transformComp = globalECS.GetComponent<TransformComponent>(globalSelectedEntity);
        if (transformComp) {
            ImGui::Text("TransformComponent:");
            ImGui::DragFloat3("Position", &transformComp->posX);
            ImGui::DragFloat3("Rotation", &transformComp->rotX);
            ImGui::DragFloat3("Scale", &transformComp->scaleX);

            ImGui::Separator();
            if (ImGui::Button("Destroy Component")) {
                globalECS.RemoveComponent<TransformComponent>(globalSelectedEntity);
            }

            ImGui::Separator();
        }

        if (!nameComp && !transformComp) {
            ImGui::Text("No components found.");
        }

    }
    else {
        ImGui::Text("Select an entity to view components.");
    }

    // display performance viewer
    if (show_perf) {
        if (gProfiler.present_ready()) {
            gPerfSnapshot = gProfiler.results();
            gPerfSnapshotTotalMs = gProfiler.total_ms();
        }

        ImGui::Begin("Performance", &show_perf, lockFlags);
        if (ImGui::Button(lockLayout ? "Unlock" : "Lock"))
        {
            lockLayout = !lockLayout;
        }
        ImGui::Separator();
        ImGui::Text("Frame: %.3f ms (%.1f FPS)", gPerfSnapshotTotalMs, GLApp::fps);

        ImGui::Separator();
        DrawEngineMemoryPanel();
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
                ImGui::ProgressBar((float)(s.pct / 100.0), ImVec2(-1, 0), "");
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }

    ImGui::End();

    // Rendering
    ScopedMemTag mem(MemTag::UI);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Handle ImGui viewports for multiple windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}