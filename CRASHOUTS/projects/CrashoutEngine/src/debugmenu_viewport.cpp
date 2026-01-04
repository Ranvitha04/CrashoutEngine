#include "debugmenu_internals.h"

using namespace CrashoutsEngine;

namespace UI {

    // Move file-local statics here
    static ImGuizmo::OPERATION g_gizmoOp = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE      g_gizmoMode = ImGuizmo::LOCAL;
    static bool                g_snapOn = false;
    static float               g_snapTrans[3] = { 1.0f, 1.0f, 1.0f };
    static float               g_snapAngle = 15.0f;

    // Helper math functions specific to viewport
    static inline CEVec3 ViewUV_toWorld(float u, float v) {
        const float ndcX = u * 2.f - 1.f;
        const float ndcY = v * 2.f - 1.f;
        const glm::mat4 VP = Component::Rendering::Get().projectionMatrix * Component::Rendering::Get().viewMatrix;
        const glm::mat4 invVP = glm::inverse(VP);
        glm::vec4 w = invVP * glm::vec4(ndcX, ndcY, 0.f, 1.f);
        if (w.w != 0.f) w /= w.w;
        return CEVec3{ (float)w.x, (float)w.y, (float)w.z };
    }

    // Ensure this macro is defined in settings.h or engine.h. 
    // If it's not defined, this whole block is skipped.
#if USE_IMGUI_EDITOR

    /*!
     @brief  Draws the editor game viewport.
    */
    void RenderGameViewport(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity& selected) {

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // Begin the ImGUI window, enable Docking
        bool open = ImGui::Begin("Game Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        static bool allowGizmoInPlay = false;

        if (open) {

            ImVec2 start = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 10));
            ImGui::BeginChild("ViewportToolbar", ImVec2(0, 32),
                false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

            // 1) toolbar (translate / rotate / scale)
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
            if (ImGui::Button("None")) g_gizmoOp = (ImGuizmo::OPERATION)0;
            ImGui::SameLine();
            if (ImGui::Button("Translate")) g_gizmoOp = ImGuizmo::TRANSLATE;
            ImGui::SameLine();
            if (ImGui::Button("Rotate")) g_gizmoOp = ImGuizmo::ROTATE;
            ImGui::SameLine();
            if (ImGui::Button("Scale")) g_gizmoOp = ImGuizmo::SCALE;
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            bool isLocal = (g_gizmoMode == ImGuizmo::LOCAL);
            if (ImGui::Checkbox("Local", &isLocal)) g_gizmoMode = isLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            ImGui::SameLine();
            ImGui::Checkbox("Snap", &g_snapOn);

            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            ImGui::Checkbox("Allow in Play", &allowGizmoInPlay);

            ImGui::PopStyleVar();

            ImGui::Separator();
            ImGui::EndChild();
            ImGui::SetCursorScreenPos(start);

            gameWinHovered = ImGui::IsWindowHovered();
            bool gameWinFocused = ImGui::IsWindowFocused();

            // Get the size of ImGui window content region
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

            // Guard to not render when we minimize
            if (viewportPanelSize.x <= 0.0f || viewportPanelSize.y <= 0.0f)
            {
                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }

            // 2) aspect-ratio / letterbox calculation
            float panelWidth = viewportPanelSize.x;
            float panelHeight = viewportPanelSize.y;
            float panelAspect = panelWidth / panelHeight;

            float targetWidth = panelWidth;
            float targetHeight = panelHeight;
            float paddingX = 0.0f;
            float paddingY = 0.0f;

            if (panelAspect > Settings::TARGET_ASPECT_RATIO) { // Viewport is wider than game (Letterbox)
                targetWidth = panelHeight * Settings::TARGET_ASPECT_RATIO;
                paddingX = (panelWidth - targetWidth) * 0.5f;
            }
            else if (panelAspect < Settings::TARGET_ASPECT_RATIO) { // Viewport is taller than game (Pillarbox)
                targetHeight = panelWidth / Settings::TARGET_ASPECT_RATIO;
                paddingY = (panelHeight - targetHeight) * 0.5f;
            }

            // Ensure dimensions are at least 1 pixel
            int finalTextureWidth = std::max(1, (int)targetWidth);
            int finalTextureHeight = std::max(1, (int)targetHeight);

            // 3) draw the actual viewport texture
            if (gameTextureWidth != finalTextureWidth || gameTextureHeight != finalTextureHeight) {
                if (finalTextureWidth > 0 && finalTextureHeight > 0) {
                    gameTextureWidth = finalTextureWidth;
                    gameTextureHeight = finalTextureHeight;

                    // Resize Texture
                    glBindTexture(GL_TEXTURE_2D, gameTextureColorBuffer);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                    // Resize RenderBuffer
                    glBindRenderbuffer(GL_RENDERBUFFER, gameRboDepthStencil);
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gameTextureWidth, gameTextureHeight);

                    // Resize Texture 2
                    glBindTexture(GL_TEXTURE_2D, CrashoutsEngine::PostProcessing::ppTextureID);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                    char logBuffer[256];
                    std::snprintf(logBuffer, sizeof(logBuffer), "Resized Game Viewport Texture to %d x %d", gameTextureWidth, gameTextureHeight);
                    LOGI(Core, logBuffer);
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(screenPos.x + paddingX, screenPos.y + paddingY));

            ImGui::SetNextItemAllowOverlap();

            // Display the texture within the ImGui window
            ImGui::Image((void*)(intptr_t)CrashoutsEngine::PostProcessing::ppTextureID,
                ImVec2((float)finalTextureWidth, (float)finalTextureHeight), ImVec2(0, 1), ImVec2(1, 0));

            ImVec2 imgMin = ImGui::GetItemRectMin();
            ImVec2 imgMax = ImGui::GetItemRectMax();
            ImVec2 imgSize = ImGui::GetItemRectSize();

            // 4) ImGuizmo overlay
            ImGuizmo::BeginFrame();
            ImGuizmo::Enable(true);
            ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
            ImGuizmo::SetRect(imgMin.x, imgMin.y, imgSize.x, imgSize.y);
            ImGuizmo::SetOrthographic(true);

            // TILEMAP EDITOR INTEGRATION
            Component::TileMapComponent* activeTM = nullptr;

            if (selected != 0 && ecs.HasComponent<Component::TileMapComponent>(selected)) {
                activeTM = ecs.GetComponent<Component::TileMapComponent>(selected);
            }
            else {
                for (Entity e : ecs.GetComponentArray<Component::TileMapComponent>().indexToEntity) {
                    Component::TileMapComponent* tm = ecs.GetComponent<Component::TileMapComponent>(e);
                    if (tm && tm->pOpen) {
                        activeTM = tm;
                        break;
                    }
                }
            }

            bool isPaintingMode = (activeTM && activeTM->pOpen);

            // Draw Grid & Handle Input
            if (isPaintingMode) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                auto WorldToScreen = [&](CEVec3 worldPos) -> ImVec2 {
                    glm::vec3 v(worldPos.x, worldPos.y, worldPos.z);
                    glm::vec4 clipSpace = Component::Rendering::Get().projectionMatrix *
                        Component::Rendering::Get().viewMatrix * glm::vec4(v, 1.f);

                    if (clipSpace.w == 0.f) return ImVec2(-1, -1);
                    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

                    float imgu = (ndc.x + 1.f) * .5f;
                    float imgv = (ndc.y + 1.f) * .5f;

                    return ImVec2(imgMin.x + imgu * imgSize.x, imgMin.y + (1.0f - imgv) * imgSize.y);
                    };

                float tileSize = (float)activeTM->tileSize;

                CEVec3 camPos{ 0,0,0 };
                if (engineState == EngineState::STOP) {
                    camPos = RenderCamera::engineCamera.GetPosition();
                }
                else camPos = RenderCamera::gameCamera.GetPosition();

                ImU32 gridColor = IM_COL32(255, 255, 255, 40);

                int centerGx = (int)std::floor(camPos.x / tileSize);
                int centerGy = (int)std::floor(camPos.y / tileSize);

                ImVec2 centerScreen = WorldToScreen(camPos);
                ImVec2 rightScreen = WorldToScreen({ camPos.x + tileSize, camPos.y, camPos.z });
                float pxPerTile = std::abs(rightScreen.x - centerScreen.x);

                if (pxPerTile < 6.f) {
                    float t = (pxPerTile - 3.f) / 3.f;
                    t = std::max(0.f, std::min(t, 1.f));
                    int alpha = (int)(40 * t);
                    gridColor = IM_COL32(255, 255, 255, alpha);
                }

                float visibleTilesX = imgSize.x / pxPerTile;
                float visibleTilesY = imgSize.y / pxPerTile;

                int gridRadius = (int)(std::max(visibleTilesX, visibleTilesY) * .5f) + 5;
                if ((gridColor >> 24) == 0) gridRadius = 0;
                if (gridRadius > 200) gridRadius = 200;

                float worldMinX = (centerGx - gridRadius) * tileSize;
                float worldMaxX = (centerGx + gridRadius) * tileSize;
                float worldMinY = (centerGy - gridRadius) * tileSize;
                float worldMaxY = (centerGy + gridRadius) * tileSize;

                for (int i = -gridRadius; i <= gridRadius; ++i) {
                    float currentX = (centerGx + i) * tileSize;
                    ImVec2 pStart = WorldToScreen({ currentX, worldMinY, 0.0f });
                    ImVec2 pEnd = WorldToScreen({ currentX, worldMaxY, 0.0f });
                    if (pStart.x != -1 && pEnd.x != -1) drawList->AddLine(pStart, pEnd, gridColor, 1.0f);
                }

                for (int j = -gridRadius; j <= gridRadius; ++j) {
                    float currentY = (centerGy + j) * tileSize;
                    ImVec2 pStart = WorldToScreen({ worldMinX, currentY, 0.0f });
                    ImVec2 pEnd = WorldToScreen({ worldMaxX, currentY, 0.0f });
                    if (pStart.x != -1 && pEnd.x != -1) drawList->AddLine(pStart, pEnd, gridColor, 1.0f);
                }

                ImVec2 mouse = ImGui::GetMousePos();
                bool overImage = (mouse.x >= imgMin.x && mouse.x <= imgMax.x && mouse.y >= imgMin.y && mouse.y <= imgMax.y);

                if (gameWinFocused && overImage) {
                    float imgu = (mouse.x - imgMin.x) / (imgMax.x - imgMin.x);
                    float imgv = 1.f - (mouse.y - imgMin.y) / (imgMax.y - imgMin.y);

                    CEVec3 worldMouse = ViewUV_toWorld(imgu, imgv);

                    // Note: If TileMapUtils is in a namespace, ensure it is qualified or 'using' is added
                    // Assuming CrashoutsEngine::TileMapUtils
                    std::pair<int, int> gridPos = CrashoutsEngine::TileMapUtils::WorldToGrid(*activeTM, worldMouse.x, worldMouse.y);
                    float snappedX = (float)gridPos.first * tileSize;
                    float snappedY = (float)gridPos.second * tileSize;

                    ImVec2 p1 = WorldToScreen({ snappedX, snappedY, 0.0f });
                    ImVec2 p2 = WorldToScreen({ snappedX + tileSize, snappedY, 0.0f });
                    ImVec2 p3 = WorldToScreen({ snappedX + tileSize, snappedY + tileSize, 0.0f });
                    ImVec2 p4 = WorldToScreen({ snappedX, snappedY + tileSize, 0.0f });

                    drawList->AddQuad(p1, p2, p3, p4, IM_COL32(191, 0, 255, 200), 2.f);

                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        if (!activeTM->activeAsset.empty() && activeTM->activeAsset != "none") {
                            CrashoutsEngine::TileMapUtils::PaintTile(*activeTM, gridPos.first, gridPos.second, activeTM->activeAsset);
                        }
                    }
                    else if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                        CrashoutsEngine::TileMapUtils::EraseTile(*activeTM, gridPos.first, gridPos.second);
                    }
                }
            }

            // 5) if we have a selected entity, let gizmo edit its TRS
            bool isPlaying = (engineState == EngineState::PLAY || engineState == EngineState::PAUSE);
            if ((!isPlaying || allowGizmoInPlay) && selected && ecs.HasComponent<Component::TransformComponent>(selected))
            {
                Entity root = CrashoutsEngine::GetRoot(ecs, selected);

                std::vector<Entity> group;
                group.reserve(16);
                CrashoutsEngine::GatherDescendants(ecs, root, group);

                CEVec3 center{ 0.f, 0.f, 0.f };
                int count = 0;
                for (CrashoutsEngine::Entity e : group)
                {
                    if (!ecs.HasComponent<Component::TransformComponent>(e))
                        continue;
                    auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                    center.x += t->position.x;
                    center.y += t->position.y;
                    center.z += t->position.z;
                    ++count;
                }
                if (count > 0)
                {
                    center.x /= count;
                    center.y /= count;
                    center.z /= count;
                }

                auto* rootT = ecs.GetComponent<Component::TransformComponent>(root);
                if (rootT)
                {
                    float tr[3] = { rootT->position.x, rootT->position.y, rootT->position.z };
                    float rt[3] = { 0.f, 0.f, rootT->rotation };
                    float sc[3] = { rootT->size.x, rootT->size.y, 1.f };

                    float m[16];
                    ImGuizmo::RecomposeMatrixFromComponents(tr, rt, sc, m);

                    const float* view = glm::value_ptr(Component::Rendering::Get().viewMatrix);
                    const float* proj = glm::value_ptr(Component::Rendering::Get().projectionMatrix);

                    const float* snapPtr = nullptr;
                    float snap[3]{};
                    if (g_snapOn)
                    {
                        if (g_gizmoOp == ImGuizmo::ROTATE) {
                            snap[0] = g_snapAngle;
                            snapPtr = snap;
                        }
                        else {
                            snap[0] = g_snapTrans[0];
                            snap[1] = g_snapTrans[1];
                            snap[2] = g_snapTrans[2];
                            snapPtr = snap;
                        }
                    }

                    ImGuizmo::Manipulate(view, proj, g_gizmoOp, g_gizmoMode, m, nullptr, snapPtr);

                    static bool dragging = false;
                    static std::vector<CrashoutsEngine::Entity> movedEntities;
                    static std::vector<CrashoutsEngine::TransformState> startStates;
                    static bool  undoTriggeredThisFrame = false;
                    static float s_parentAccumRotDeg = 0.0f;
                    static float s_prevGizmoDeg = 0.0f;

                    if (ImGuizmo::IsUsing())
                    {
                        float newTr[3], newRt[3], newSc[3];
                        ImGuizmo::DecomposeMatrixToComponents(m, newTr, newRt, newSc);

                        if (!dragging) {
                            dragging = true;
                            undoTriggeredThisFrame = false;
                            movedEntities.clear();
                            startStates.clear();

                            for (Entity e : group) {
                                if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                                auto* transform = ecs.GetComponent<Component::TransformComponent>(e);
                                CrashoutsEngine::TransformState state;
                                state.position = transform->position;
                                state.rotation = transform->rotation;
                                state.scale = transform->size;
                                startStates.push_back(state);
                                movedEntities.push_back(e);
                            }
                            s_parentAccumRotDeg = rootT->rotation;
                            s_prevGizmoDeg = newRt[2];
                        }

                        if (g_gizmoOp == ImGuizmo::TRANSLATE) {
                            CEVec3 offset{ newTr[0] - rootT->position.x, newTr[1] - rootT->position.y, newTr[2] - rootT->position.z };
                            for (Entity e : group) {
                                if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                                auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                                t->position.x += offset.x;
                                t->position.y += offset.y;
                                t->position.z += offset.z;
                            }
                        }
                        else {
                            rootT->position = { newTr[0], newTr[1], newTr[2] };
                            rootT->size.x = newSc[0];
                            rootT->size.y = newSc[1];

                            if (g_gizmoOp == ImGuizmo::ROTATE) {
                                float gizmoDeg = newRt[2];
                                float stepDeg = gizmoDeg - s_prevGizmoDeg;
                                if (stepDeg > 180.0f) stepDeg -= 360.0f;
                                if (stepDeg < -180.0f) stepDeg += 360.0f;

                                s_parentAccumRotDeg += stepDeg;
                                rootT->rotation = s_parentAccumRotDeg;
                                s_prevGizmoDeg = gizmoDeg;

                                for (Entity e : group) {
                                    if (e == root) continue;
                                    if (!ecs.HasComponent<Component::ParentComponent>(e)) continue;
                                    auto* ppc = ecs.GetComponent<Component::ParentComponent>(e);
                                    ppc->localRot += stepDeg;
                                }
                            }
                        }
                    }
                    else if (dragging && !undoTriggeredThisFrame)
                    {
                        dragging = false;
                        undoTriggeredThisFrame = true;

                        std::vector<CrashoutsEngine::TransformState> endStates;
                        endStates.reserve(movedEntities.size());

                        for (auto e : movedEntities) {
                            auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                            if (t) {
                                CrashoutsEngine::TransformState s;
                                s.position = t->position;
                                s.rotation = t->rotation;
                                s.scale = t->size;
                                endStates.push_back(s);
                            }
                        }

                        bool changed = false;
                        for (size_t i = 0; i < startStates.size(); ++i) {
                            if (startStates[i].position != endStates[i].position ||
                                startStates[i].rotation != endStates[i].rotation ||
                                startStates[i].scale != endStates[i].scale)
                            {
                                changed = true;
                                break;
                            }
                        }

                        if (changed) {
                            if (movedEntities.size() > 1) {
                                UndoManager::GetInstance().AddCommand(
                                    std::make_unique<CrashoutsEngine::TransformGroupCommand>(movedEntities, startStates, endStates));
                            }
                        }
                        movedEntities.clear();
                        startStates.clear();
                    }

                    if (!ImGuizmo::IsUsing()) undoTriggeredThisFrame = false;
                }
            }

            // 6) mouse picking
            ImVec2 mouse = ImGui::GetMousePos();
            const bool over = (mouse.x >= imgMin.x && mouse.x <= imgMax.x && mouse.y >= imgMin.y && mouse.y <= imgMax.y);

            float u = 0.f, v = 0.f;
            if (over) {
                u = (mouse.x - imgMin.x) / (imgMax.x - imgMin.x);
                v = 1.f - (mouse.y - imgMin.y) / (imgMax.y - imgMin.y);
            }

            if (gameWinFocused) {
                if (over && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
                {
                    CEVec3 worldPt = ViewUV_toWorld(u, v);
                    Entity best = 0;
                    float  bestZ = -FLT_MAX;

                    auto& tArr = ecs.GetComponentArray<Component::TransformComponent>();
                    for (Entity e : tArr.indexToEntity)
                    {
                        if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                        auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                        float hx = std::max(0.5f * t->size.x, 0.5f);
                        float hy = std::max(0.5f * t->size.y, 0.5f);
                        bool  hit = false;

                        if (auto* sc = ecs.GetComponent<Component::ShapeComponent>(e))
                        {
                            using Component::ShapeType;
                            switch (sc->type)
                            {
                            case ShapeType::Circle: {
                                const float r = 0.5f * std::min(t->size.x, t->size.y);
                                const float dx = worldPt.x - t->position.x;
                                const float dy = worldPt.y - t->position.y;
                                hit = (dx * dx + dy * dy) <= r * r;
                                break;
                            }
                            case ShapeType::Line: {
                                const float minx = std::min(sc->p1.x, sc->p2.x);
                                const float maxx = std::max(sc->p1.x, sc->p2.x);
                                const float miny = std::min(sc->p1.y, sc->p2.y);
                                const float maxy = std::max(sc->p1.y, sc->p2.y);
                                hx = std::max(hx, std::max(0.5f * (maxx - minx), sc->thickness * 0.5f));
                                hy = std::max(hy, std::max(0.5f * (maxy - miny), sc->thickness * 0.5f));
                                [[fallthrough]];
                            }
                            case ShapeType::Rectangle:
                            case ShapeType::Triangle:
                            default: {
                                const float minx = t->position.x - hx, maxx = t->position.x + hx;
                                const float miny = t->position.y - hy, maxy = t->position.y + hy;
                                hit = (worldPt.x >= minx && worldPt.x <= maxx && worldPt.y >= miny && worldPt.y <= maxy);
                                break;
                            }
                            }
                        }
                        else {
                            const float minx = t->position.x - hx, maxx = t->position.x + hx;
                            const float miny = t->position.y - hy, maxy = t->position.y + hy;
                            hit = (worldPt.x >= minx && worldPt.x <= maxx && worldPt.y >= miny && worldPt.y <= maxy);
                        }

                        if (hit && t->position.z >= bestZ) { bestZ = t->position.z; best = e; }
                    }

                    if (best) selected = (selected == best) ? (Entity)0 : best;
                }

                if (over && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    if (!ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) selected = 0;
                }
                if (selected && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGuizmo::IsUsing() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                    ecs.DestroyEntity(selected);
                    selected = 0;
                }

                CEVec3 worldPt = ViewUV_toWorld(u, v);
                InputManager::g_mouseWorldPos = CEVec2{ worldPt.x, worldPt.y };
            }

            // 7) drag and drop
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
                    if (payload->Data && payload->DataSize == sizeof(AssetDragPayload)) {
                        const auto* p = static_cast<const AssetDragPayload*>(payload->Data);
                        CEVec3 dropWorld = ViewUV_toWorld(u, v);

                        using CrashoutsEngine::Assets::AssetManager;
                        using CrashoutsEngine::Assets::TextureAsset;

                        if (p->kind == AssetKind::Texture) {
                            if (auto* tex = AssetManager::GetInstance().Get<TextureAsset>(p->name)) {
                                Entity e = ecs.CreateEntity();
                                ecs.AddComponent<Component::NameComponent>(e, std::string("Tex_") + p->name);
                                ecs.AddComponent<Component::TransformComponent>(e);
                                ecs.AddComponent<Component::ShapeComponent>(e);
                                auto* sc = ecs.GetComponent<Component::ShapeComponent>(e);
                                sc->enableTexture = true;
                                sc->textureName = p->name;
                                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) {
                                    t->position = dropWorld;
                                    t->size = { 50.0f, 50.0f };
                                }
                            }
                        }
                        else if (p->kind == AssetKind::Prefab) {
                            Entity e = Prefab::Create(ecs, std::string(p->name));
                            if (e && ecs.HasComponent<Component::TransformComponent>(e))
                                ecs.GetComponent<Component::TransformComponent>(e)->position = dropWorld;
                        }
                        else if (p->kind == AssetKind::Audio) {
                            // ... Audio spawn logic ...
                            std::string rel = p->relPath;
                            const std::string prefix = "audio/";
                            auto pos = rel.find(prefix);
                            if (pos != std::string::npos) rel = rel.substr(pos + prefix.size());
                            if (auto dot = rel.find_last_of('.'); dot != std::string::npos) rel.erase(dot);
                            std::string eventLike = std::string("event:/") + rel;

                            Entity target = selected;
                            if (!target) {
                                target = ecs.CreateEntity();
                                ecs.AddComponent<Component::NameComponent>(target, std::string("Audio_") + p->name);
                                ecs.AddComponent<Component::TransformComponent>(target);
                                if (auto* t = ecs.GetComponent<Component::TransformComponent>(target)) {
                                    t->position = dropWorld;
                                    t->size = { 10.0f, 10.0f };
                                }
                                ecs.AddComponent<Component::ShapeComponent>(target);
                                if (auto* sc = ecs.GetComponent<Component::ShapeComponent>(target)) {
                                    sc->enableTexture = false;
                                    sc->textureName = "audio_icon";
                                }
                            }
                            else {
                                if (auto* t = ecs.GetComponent<Component::TransformComponent>(target)) t->position = dropWorld;
                            }

                            if (!ecs.HasComponent<Component::AudioEmitterComponent>(target)) ecs.AddComponent<Component::AudioEmitterComponent>(target);
                            if (auto* ac = ecs.GetComponent<Component::AudioEmitterComponent>(target)) ac->eventPath = eventLike;
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
#endif
}