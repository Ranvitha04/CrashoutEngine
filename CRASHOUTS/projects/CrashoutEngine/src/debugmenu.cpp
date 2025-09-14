
// Engine / app headers
#include "glapp.h"
#include "performanceViewer.h"
#include "entitymanager.h"
#include "log.h"
#include "debugmenu.h"


// ImGui headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <string>
#include <cstring>

using CrashoutsEngine::Entity;
using CrashoutsEngine::ECS;
using namespace Component;

namespace {
    // State that used to live in main.cpp is now private to this translation unit
    // ECS instance and entity storage
    ECS gECS;
    std::vector<Entity> gEntities;
    Entity gSelectedEntity = 0;

    // ImGui in-game performance viewer
    bool gShowPerf = false;
    std::vector<ProfilerSample> gPerfSnapshot;
    double gPerfSnapshotTotalMs = 0.0;
}

namespace UI {

    void Init(GLFWwindow* window) {
        // Setup ImGUI context
        IMGUI_CHECKVERSION();

        ImGui::SetAllocatorFunctions(
            [](size_t sz, void*) {
                ScopedMemTag tag(MemTag::UI);
                return ::operator new(sz);
            },
            [](void* p, void*) {
                ScopedMemTag tag(MemTag::UI);
                ::operator delete(p);
            },
            nullptr
        );

        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();

        ScopedMemTag boot(MemTag::Resources);

        // Initializing the backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    static void RenderEcsDemo(FrameProfiler& profiler) {
        using namespace CrashoutsEngine;
        FrameProfiler::Scoped _imgui(profiler, "ImGui");
        ScopedMemTag mem(MemTag::UI);

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
            Entity newEntity = gECS.CreateEntity();
            gEntities.push_back(newEntity);
            if (!gECS.HasComponent<NameComponent>(newEntity)) {
                gECS.AddComponent<NameComponent>(newEntity, "Entity " + std::to_string(newEntity));
            }
        }

        ImGui::Separator();

        // Toggle Button to show performance
        if (ImGui::Button(gShowPerf ? "Hide Performance" : "Show Performance")) {
            gShowPerf = !gShowPerf;
        }

        ImGui::Separator();

        // List all entities
        ImGui::Text("Entities");
        if (gEntities.empty()) {
            ImGui::Text("No entities created yet.");
        }
        else {
            for (const auto& entity : gEntities) {
                std::string entity_label;
                if (gECS.HasComponent<NameComponent>(entity)) {
                    NameComponent* nameComp = gECS.GetComponent<NameComponent>(entity);
                    entity_label = nameComp->name;
                }
                else {
                    entity_label = "Entity: " + std::to_string(entity);
                }
                if (ImGui::Selectable(entity_label.c_str(), gSelectedEntity == entity)) {
                    gSelectedEntity = entity;
                }
            }
        }

        ImGui::Separator();

        // Actions for the selected entity
        ImGui::Text("Selected Entity: %d", gSelectedEntity);

        if (gSelectedEntity != 0) {
            if (ImGui::Button("Destroy Selected")) {
                gECS.DestroyEntity(gSelectedEntity);
                for (size_t i = 0; i < gEntities.size(); ++i) {
                    if (gEntities[i] == gSelectedEntity) {
                        gEntities.erase(gEntities.begin() + i);
                        break;
                    }
                }
                gSelectedEntity = 0;
            }

            ImGui::Separator();
            ImGui::Text("Add Components");
            if (ImGui::Button("Add NameComponent")) {
                if (!gECS.HasComponent<NameComponent>(gSelectedEntity)) {
                    gECS.AddComponent<NameComponent>(gSelectedEntity, "Unnamed Entity");
                }
            }
            if (ImGui::Button("Add TransformComponent")) {
                if (!gECS.HasComponent<TransformComponent>(gSelectedEntity)) {
                    gECS.AddComponent<TransformComponent>(gSelectedEntity, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }

        ImGui::Separator();
        ImGui::Text("Components on Selected Entity");
        if (gSelectedEntity != 0) {
            // Display NameComponent
            NameComponent* nameComp = gECS.GetComponent<NameComponent>(gSelectedEntity);
            if (nameComp) {
                char buffer[256];
                std::strncpy(buffer, nameComp->name.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';      // Ensure null termination

                ImGui::Text("Name: ");
                if (ImGui::InputText("##Name", buffer, sizeof(buffer))) {
                    nameComp->name = buffer;
                }
            }

            // Display TransformComponent
            TransformComponent* transformComp = gECS.GetComponent<TransformComponent>(gSelectedEntity);
            if (transformComp) {
                ImGui::Text("TransformComponent:");
                ImGui::DragFloat3("Position", &transformComp->posX);
                ImGui::DragFloat3("Rotation", &transformComp->rotX);
                ImGui::DragFloat3("Scale", &transformComp->scaleX);

                ImGui::Separator();
                if (ImGui::Button("Destroy Component")) {
                    gECS.RemoveComponent<TransformComponent>(gSelectedEntity);
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
        if (gShowPerf) {
            if (profiler.present_ready()) {
                gPerfSnapshot = profiler.results();
                gPerfSnapshotTotalMs = profiler.total_ms();
            }

            ImGui::Begin("Performance", nullptr, lockFlags);
            if (ImGui::Button(lockLayout ? "Unlock" : "Lock"))
            {
                lockLayout = !lockLayout;
            }
            ImGui::Separator();
            ImGui::Text("Frame: %.3f ms (%.1f FPS)", gPerfSnapshotTotalMs, GLApp::fps);

            ImGui::Separator();
            DrawEngineMemoryPanel();
            ImGui::Separator();
            DrawProfilerPanel(profiler);
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

        static bool showInfo = true, showWarn = true, showErr = true, showSucc = true;
        static bool sysCore = true, sysPhys = true, sysColl = true, sysInput = true, sysAI = true, sysECS = true, sysRend = true, sysUI = true;

        ImGui::Begin("Console");

        if (ImGui::Button(lockLayout ? "Unlock" : "Lock"))
        {
            lockLayout = !lockLayout;
        }
        ImGui::Separator();

        // level filters
        ImGui::Checkbox("Info", &showInfo); ImGui::SameLine();
        ImGui::Checkbox("Warn", &showWarn); ImGui::SameLine();
        ImGui::Checkbox("Error", &showErr); ImGui::SameLine();
        ImGui::Checkbox("Success", &showSucc);

        // subsystem filters
        ImGui::Separator();
        ImGui::TextUnformatted("Subsystems:");
        ImGui::Checkbox("Core", &sysCore); ImGui::SameLine();
        ImGui::Checkbox("Physics", &sysPhys); ImGui::SameLine();
        ImGui::Checkbox("Collision", &sysColl); ImGui::SameLine();
        ImGui::Checkbox("Input", &sysInput);
        ImGui::Checkbox("AI", &sysAI); ImGui::SameLine();
        ImGui::Checkbox("ECS", &sysECS); ImGui::SameLine();
        ImGui::Checkbox("Renderer", &sysRend); ImGui::SameLine();
        ImGui::Checkbox("UI", &sysUI);

        ImGui::Separator();

        const auto& logs = Logger::Instance().Get();
        for (const auto& log : logs) {
            // level gate
            if ((log.level == LogMessage::Level::Info && !showInfo) ||
                (log.level == LogMessage::Level::Warning && !showWarn) ||
                (log.level == LogMessage::Level::Error && !showErr) ||
                (log.level == LogMessage::Level::Success && !showSucc)) continue;

            // subsystem gate
            bool sysPass =
                (log.system == LogMessage::Subsystem::Core && sysCore) ||
                (log.system == LogMessage::Subsystem::Physics && sysPhys) ||
                (log.system == LogMessage::Subsystem::Collision && sysColl) ||
                (log.system == LogMessage::Subsystem::Input && sysInput) ||
                (log.system == LogMessage::Subsystem::AI && sysAI) ||
                (log.system == LogMessage::Subsystem::ECS && sysECS) ||
                (log.system == LogMessage::Subsystem::Renderer && sysRend) ||
                (log.system == LogMessage::Subsystem::UI && sysUI);
            if (!sysPass) continue;

            ImVec4 color =
                (log.level == LogMessage::Level::Error) ? ImVec4(1, 0.3f, 0.3f, 1) :
                (log.level == LogMessage::Level::Warning) ? ImVec4(1, 0.9f, 0.4f, 1) :
                (log.level == LogMessage::Level::Success) ? ImVec4(0.4f, 1, 0.4f, 1) :
                ImVec4(0.4f, 0.8f, 1, 1);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.text.c_str());
            ImGui::PopStyleColor();
        }
        if (ImGui::Button("Clear")) {
            // Quick hack: clear by casting away const
            const_cast<std::vector<LogMessage>&>(logs).clear();
        }
        ImGui::End();

        // Rendering
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

    void Render(FrameProfiler& profiler) {
        RenderEcsDemo(profiler);
    }

    void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

} // namespace UI