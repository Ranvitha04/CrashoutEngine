/*!
@file        sceneutils.cpp
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements function calls that run every frame for the scenes
*/

#include "rendering.h"
#include "textrenderer.h"
#include "textureasset.h"
#include "assetmanager.h"
#include "inputmanager.h"
#include "frameratecontroller.h"
#include "camera.h"
#include "settings.h"
#include "rendererconnector.h"
#include "sceneutils.h"
#include "engine.h"
#include "instanceSystem.h"
#include "glapp.h"

#include "shapescomponent.h"
#include "transformcomponent.h"
#include "namecomponent.h"
#include "textcomponent.h"
#include "shapes.h"
#include "uilayoutcomponent.h"
#include "textinputcomponent.h"
#include "buttoncomponent.h"
#include "tilemapcomponent.h"
#include "camera.h"

#define FONT_SCALE 0.05f

namespace Scene {

    using namespace Component;
    using namespace CrashoutsEngine;
    using namespace CrashoutsEngine::Assets;

    // ============================================================================
    // UI SUB-NAMESPACE
    // ============================================================================
    namespace UI {

        /*!
        @brief      Renders all text components and handles the blinking cursor for focused inputs.
        @param      globalECS Reference to the ECS system to retrieve components.
        */
        void RenderTextAndCursor(ECS& globalECS) {

            // 1. Reset Shader
            Component::Rendering& renderer = Component::Rendering::Get();
            renderer.shdr_pgm->Use(); // Switch back to standard shader
            renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
            renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

            // 2. Reset Blending (The fix for your current issue)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_DEPTH_TEST);
            // 3. Reset Culling (Text is often double-sided or winding order might vary)
            glDisable(GL_CULL_FACE);

            // For Drawing Text Components
            const ECS::ComponentArray<TextComponent>& textArray = globalECS.GetComponentArray<TextComponent>();

            // Ensure renderer exists
            if (!gTextRenderer) {
                gTextRenderer = std::make_unique<TextRenderer>();
            }

            for (const Entity& id : textArray.indexToEntity) {
                TextComponent* tc = globalECS.GetComponent<TextComponent>(id);
                float depthZ = 0.0f;
                if (globalECS.HasComponent<TransformComponent>(id)) {
                    TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);
                    depthZ = t->position.z;
                    CEVec2 newOffset = CEVec2{ t->position.x + tc->offset.x, t->position.y + tc->offset.y };

                    // Modify Offset if UILayout Present
                    UILayoutComponent* u = globalECS.GetComponent<UILayoutComponent>(id);
                    if (u) {
                        // --- 1. Handle Vertical Alignment & Padding ---
                        if ((u->uiLayout & UILayout::TOP) != UILayout::NONE) {
                            newOffset.y += t->size.y * 0.5f;
                            newOffset.y -= u->paddingXY.y; // Padding pushes *down* from the top
                        }
                        else if ((u->uiLayout & UILayout::DOWN) != UILayout::NONE) {
                            newOffset.y -= t->size.y * 0.5f;
                            newOffset.y += u->paddingXY.y; // Padding pushes *up* from the bottom
                        }

                        // --- 2. Handle Horizontal Alignment & Padding ---
                        if ((u->uiLayout & UILayout::LEFT) != UILayout::NONE) {
                            newOffset.x -= t->size.x * 0.5f;
                            newOffset.x += u->paddingXY.x; // Padding pushes *right* from the left
                        }
                        else if ((u->uiLayout & UILayout::RIGHT) != UILayout::NONE) {
                            newOffset.x += t->size.x * 0.5f;
                            newOffset.x -= u->paddingXY.x; // Padding pushes *left* from the right
                        }
                    }

                    float finalScale = tc->scale;

                    std::string font = tc->font;
                    if (font == "") font = "Exo2-Regular";

                    if (tc->isWorldSpace) {
                        // Case 1: World Space Text (Behaves like a sprite)
                        // DO NOT divide by zoom. Let the View Matrix handle the zoom scaling naturally.
                        finalScale = tc->scale;
                    }
                    else {
                        // Case 2: UI Text (Screen Space)
                        // RenderText now handles the projection matrix switch internally,
                        // so we pass the raw scale.
                        finalScale = tc->scale;
                    }

                    if (tc->centered)
                        gTextRenderer->RenderCenteredText(tc->text, newOffset.x, newOffset.y, depthZ, finalScale, tc->color, font, tc->isWorldSpace);
                    else
                        gTextRenderer->RenderText(tc->text, newOffset.x, newOffset.y, depthZ, finalScale, tc->color, font, tc->isWorldSpace);

                    TextInputComponent* input = globalECS.GetComponent<TextInputComponent>(id);
                    // Check if this entity is a focused text input
                    if (input) {

                        // Only draw cursor if this component is focused
                        if (input->isFocused && engineState != EngineState::STOP) {

                            // 3. (Optional) Make it blink
                            static float blinkTimer = 0.0f;
                            blinkTimer += (float)FrameRateController::GetInstance().DT(); // Your frame delta time
                            bool showCursor = fmod(blinkTimer, 1.0f) < 0.5f; // On for 0.5s, off for 0.5s

                            if (showCursor) {
                                // 4. Get the components again (or just reuse them)
                                auto* text = globalECS.GetComponent<TextComponent>(id);
                                auto* transform = globalECS.GetComponent<TransformComponent>(id);
                                GLfloat startX = transform->position.x - (transform->size.x * 0.5f);

                                // 5. Calculate the cursor's baseline position
                                CEVec2 cursorPos = gTextRenderer.get()->GetCursorPosition(
                                    text->text,
                                    input->cursorPos,
                                    startX,
                                    transform->position.y,
                                    1.f, // Assuming scale is in x
                                    font
                                );

                                // 6. Define cursor properties
                                GLfloat cursorThickness = .25f; // How wide the cursor is
                                GLfloat cursorHeight = gTextRenderer.get()->GetLineHeight(font, transform->size.x);
                                CEColor cursorColor = { .0f, .0f, .0f }; // White

                                Assets::FontAsset* fontAsset = Assets::AssetManager::GetInstance().Get<Assets::FontAsset>(font);
                                float ascender = 0.0f;
                                if (fontAsset && fontAsset->characters.count('H')) {
                                    // Get 'H's bearing (distance from baseline to top of glyph) and scale it
                                    ascender = (float)fontAsset->characters.at('H').Bearing.y * (transform->size.x * FONT_SCALE);
                                }
                                else {
                                    // Fallback: approximate ascender as 75% of total height
                                    ascender = cursorHeight * 0.75f;
                                }

                                // 7. Define the start and end of the vertical cursor line
                                // 'cursorPos.y' is the baseline (top of the line).
                                // We'll draw it from the baseline down by the line height.
                                // (Assuming Z-coordinate is 0 for 2D text)
                                CEVec3 startPoint = { cursorPos.x, cursorPos.y + ascender * FONT_SCALE * .5f, 0.0f };
                                CEVec3 endPoint = { cursorPos.x, cursorPos.y - cursorHeight * FONT_SCALE * .5f, 0.0f };

                                // 8. Draw the cursor using your Shapes::CreateLine function
                                Shapes::CreateLine(
                                    startPoint,           // start
                                    endPoint,             // end
                                    cursorThickness,      // thickness
                                    cursorColor,          // color
                                    tc->isWorldSpace      // Respect World vs UI space
                                );
                            }
                        }
                    }
                }
            }
        }

        /*!
        @brief      Updates button states (Hover, Pressed, Normal) and visual feedback.
        @param      globalECS Reference to the ECS system.
        */
        void UpdateButtons(ECS& globalECS) {
            // 1. Get MOUSE in WORLD Space (For Game Objects)
            // (Assuming GetMouseWorldPos works correctly for your camera)
            CEVec2 mouseWorld = InputManager::GetMouseWorldPos();

            // 2. Calculate MOUSE in UI Space (Center Origin, Fixed Height 100.0f)
            // This logic ensures UI clicks work regardless of Camera Zoom/Position.
            CEVec2 mouseUI;
            CEVec2 rawMouse = InputManager::GetMousePosition(); // Pixels: 0..W, 0..H

            float winW = (float)GLApp::width;
            float winH = (float)GLApp::height;

            // Calculate NDC (-1 to 1)
            // Flip Y because Input is Top-Down, but OpenGL/World is Bottom-Up
            float ndcX = (rawMouse.x / winW) * 2.0f - 1.0f;
            float ndcY = 1.0f - (rawMouse.y / winH) * 2.0f;

            // Scale NDC to your Fixed UI Units (Base Height = 100.0f)
            // This MUST match the projection used in RenderScene!
            float baseHeight = 100.0f;
            float aspect = winW / winH;
            float baseWidth = baseHeight * aspect;

            mouseUI.x = ndcX * (baseWidth * 0.5f);
            mouseUI.y = ndcY * (baseHeight * 0.5f);

            const ECS::ComponentArray<ButtonComponent>& buttonArray = globalECS.GetComponentArray<ButtonComponent>();

            for (const Entity& id : buttonArray.indexToEntity) {
                ButtonComponent* b = globalECS.GetComponent<ButtonComponent>(id);
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);

                // --- DETERMINE COORDINATE SPACE ---
                ShapeComponent* s = globalECS.GetComponent<ShapeComponent>(id);
                TextComponent* txt = globalECS.GetComponent<TextComponent>(id);

                // Default to checking against World Mouse
                CEVec2 checkPos = mouseWorld;

                // Check if this entity is explicitly Screen Space
                bool isUI = false;
                if (s && !s->isWorldSpace) isUI = true;
                else if (txt && !txt->isWorldSpace) isUI = true;

                // If it's UI, switch to checking against the Scaled UI Mouse
                if (isUI) {
                    checkPos = mouseUI;
                }

                //std::cout << "Mouse pos is " << checkPos.x << ", " << checkPos.y << "Button Pos is" << t->position.x << ", " << t->position.y << std::endl;

                // --- COLLISION CHECK (AABB) ---
                float halfW = t->size.x * 0.5f;
                float halfH = t->size.y * 0.5f;
                CEVec2 aa{ t->position.x - halfW, t->position.y - halfH };
                CEVec2 bb{ t->position.x + halfW, t->position.y + halfH };

                bool isHovering = (checkPos.x >= aa.x && checkPos.x <= bb.x &&
                    checkPos.y >= aa.y && checkPos.y <= bb.y);

                if (isHovering) {
                    if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                        b->buttonState = ButtonState::PRESSED;
                        if (s) s->color = b->shapePressedColor;
                        if (txt) txt->color = b->textPressedColor;
                    }
                    else {
                        b->buttonState = ButtonState::HOVERED;
                        if (s) s->color = b->shapeHoveredColor;
                        if (txt) txt->color = b->textHoveredColor;
                    }
                }
                else if (b->buttonState != ButtonState::NORMAL) {
                    b->buttonState = ButtonState::NORMAL;
                    if (s) s->color = b->shapeNormalColor;
                    if (txt) txt->color = b->textNormalColor;
                }
            }
        }

        /*!
        @brief      Updates text input focus logic and handles keyboard events.
        @param      globalECS Reference to the ECS system.
        */
        void UpdateInputs(ECS& globalECS) {
            static Entity selectedTextInput = (Entity)-1;
            const ECS::ComponentArray<TextInputComponent>& textInputArray = globalECS.GetComponentArray<TextInputComponent>();

            // 1. Calculate Mouse Positions (Same logic as UpdateButtons)
            CEVec2 mouseWorld = InputManager::GetMouseWorldPos();

            CEVec2 mouseUI;
            CEVec2 rawMouse = InputManager::GetMousePosition();
            float winW = (float)GLApp::width;
            float winH = (float)GLApp::height;
            float ndcX = (rawMouse.x / winW) * 2.0f - 1.0f;
            float ndcY = 1.0f - (rawMouse.y / winH) * 2.0f;

            float baseHeight = 100.0f;
            float aspect = winW / winH;
            float baseWidth = baseHeight * aspect;

            mouseUI.x = ndcX * (baseWidth * 0.5f);
            mouseUI.y = ndcY * (baseHeight * 0.5f);

            bool clickedThisFrame = InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            Entity newSelection = (Entity)-1;

            // 2. Detect clicks on input fields
            for (const Entity& id : textInputArray.indexToEntity) {
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);
                TextComponent* txt = globalECS.GetComponent<TextComponent>(id);

                // Determine Space
                CEVec2 checkPos = mouseWorld;
                bool isUI = false;
                if (txt && !txt->isWorldSpace) isUI = true;

                // Also check ShapeComponent if present (often used for background box)
                if (!isUI && globalECS.HasComponent<ShapeComponent>(id)) {
                    if (!globalECS.GetComponent<ShapeComponent>(id)->isWorldSpace) isUI = true;
                }

                if (isUI) checkPos = mouseUI;

                // AABB Check
                float halfW = t->size.x * 0.5f;
                float halfH = t->size.y * 0.5f;
                CEVec2 aa{ t->position.x - halfW, t->position.y - halfH };
                CEVec2 bb{ t->position.x + halfW, t->position.y + halfH };

                if (checkPos.x >= aa.x && checkPos.x <= bb.x && checkPos.y >= aa.y && checkPos.y <= bb.y) {
                    if (clickedThisFrame) {
                        newSelection = id; // We clicked this input!
                    }
                }
            }

            // 3. Update the focus state
            if (clickedThisFrame) {
                if (selectedTextInput != -1 && selectedTextInput != newSelection) {
                    if (auto* oldInput = globalECS.GetComponent<TextInputComponent>(selectedTextInput)) {
                        oldInput->isFocused = false;
                    }
                }
                if (newSelection != -1) {
                    globalECS.GetComponent<TextInputComponent>(newSelection)->isFocused = true;
                }
                selectedTextInput = newSelection;
            }

            // 4. Process Input for the selected field
            if (selectedTextInput != -1) {
                TextInputComponent* textInput = globalECS.GetComponent<TextInputComponent>(selectedTextInput);
                TextComponent* text = globalECS.GetComponent<TextComponent>(selectedTextInput);

                if (textInput && text) {
                    // A. Character Typing
                    std::string typed = InputManager::GetTypedCharacters();
                    if (!typed.empty()) {
                        size_t currentLength = text->text.length();
                        if (currentLength < textInput->maxCharacters) {
                            size_t remainingSpace = textInput->maxCharacters - currentLength;
                            if (typed.length() > remainingSpace) {
                                typed = typed.substr(0, remainingSpace);
                            }
                            text->text.insert(textInput->cursorPos, typed);
                            textInput->cursorPos += typed.length();
                        }
                    }

                    // B. Backspace
                    if (InputManager::IsKeyTriggered(GLFW_KEY_BACKSPACE)) {
                        if (textInput->cursorPos > 0 && !text->text.empty()) {
                            text->text.erase(textInput->cursorPos - 1, 1);
                            textInput->cursorPos--;
                        }
                    }

                    // C. Delete
                    if (InputManager::IsKeyTriggered(GLFW_KEY_DELETE)) {
                        if (textInput->cursorPos < text->text.length()) {
                            text->text.erase(textInput->cursorPos, 1);
                        }
                    }

                    // D. Arrows
                    if (InputManager::IsKeyTriggered(GLFW_KEY_LEFT)) {
                        if (textInput->cursorPos > 0) textInput->cursorPos--;
                    }
                    if (InputManager::IsKeyTriggered(GLFW_KEY_RIGHT)) {
                        if (textInput->cursorPos < text->text.length()) textInput->cursorPos++;
                    }

                    // E. Enter
                    if (InputManager::IsKeyTriggered(GLFW_KEY_ENTER)) {
                        text->text.insert(textInput->cursorPos, "\n");
                        textInput->cursorPos += 1;
                    }
                }
            }
        }
    } // End namespace UI

    // ============================================================================
    // SCENE RENDERING
    // ============================================================================

    TextureMode GetWrapMode(int mode) {
        switch (mode) {
        case 0: return TextureMode::ClampToEdge;
        case 1: return TextureMode::Repeat;
        case 2: return TextureMode::Mirror;
        default: return TextureMode::ClampToEdge;
        }
    }

    struct RenderBatch {
        std::vector<CEVec3> positions;
        std::vector<CEColor> colors;
        std::vector<float> rotations;
        std::vector<float> scales;
        std::vector<float> dim1;
        std::vector<float> dim2;
        std::vector<CEVec2> uvOffsets;

        CEVec2 uvSize{ 1.0f, 1.0f };
        std::string textureName = "";
        bool enableTexture = false;
        TextureMode wrapU = TextureMode::ClampToEdge;
        TextureMode wrapV = TextureMode::ClampToEdge;

        void Clear() {
            positions.clear(); colors.clear(); rotations.clear();
            scales.clear(); dim1.clear(); dim2.clear(); uvOffsets.clear();
            uvSize = { 1.0f, 1.0f };
            textureName = "";
            enableTexture = false;
            wrapU = TextureMode::ClampToEdge;
            wrapV = TextureMode::ClampToEdge;
        }

        void Add(const CEVec3& pos, const CEColor& col, float rot, float scale, float d1, float d2, const CEVec2& offset) {
            positions.push_back(pos);
            colors.push_back(col);
            rotations.push_back(rot);
            scales.push_back(scale);
            dim1.push_back(d1);
            dim2.push_back(d2);
            uvOffsets.push_back(offset);
        }
    };

    struct BatchKey {
        std::string textureName;
        float uvX, uvY;
        int wrapU, wrapV;

        bool operator==(const BatchKey& other) const {
            return textureName == other.textureName &&
                uvX == other.uvX && uvY == other.uvY &&
                wrapU == other.wrapU && wrapV == other.wrapV;
        }
    };

    struct BatchKeyHasher {
        std::size_t operator()(const BatchKey& k) const {
            std::size_t h1 = std::hash<std::string>{}(k.textureName);
            std::size_t h2 = std::hash<float>{}(k.uvX);
            std::size_t h3 = std::hash<float>{}(k.uvY);
            std::size_t h4 = std::hash<int>{}(k.wrapU);
            std::size_t h5 = std::hash<int>{}(k.wrapV);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
        }
    };

    std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher> rectBatches;
    std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher> triBatches;
    std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher> circBatches;

    // --- HELPER: BatchEntities (Processes a specific list of entities) ---
    void BatchEntities(
        const std::vector<Entity>& entities,
        ECS& globalECS,
        std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& rects,
        std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& tris,
        std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& circs,
        std::vector<std::pair<TransformComponent*, ShapeComponent*>>& lines)
    {
        for (const Entity& id : entities) {
            if (!globalECS.HasComponent<TransformComponent>(id)) continue;

            auto* tf = globalECS.GetComponent<TransformComponent>(id);
            auto* sc = globalECS.GetComponent<ShapeComponent>(id);

            // Lines are not batched here
            if (sc->type == ShapeType::Line) {
                lines.push_back({ tf, sc });
                continue;
            }

            BatchKey key;
            if (sc->enableTexture) key = { sc->textureName, sc->uvSize.x, sc->uvSize.y, sc->wrapModeU, sc->wrapModeV };
            else key = { "NoTexture", 1.0f, 1.0f, 0, 0 };

            CEColor colorNorm(sc->color.x(), sc->color.y(), sc->color.z(), sc->color.w());

            auto ConfigureBatch = [&](RenderBatch& batch) {
                if (batch.positions.empty()) {
                    batch.uvSize = sc->uvSize;
                    batch.textureName = sc->textureName;
                    batch.enableTexture = sc->enableTexture;
                    batch.wrapU = GetWrapMode(sc->wrapModeU);
                    batch.wrapV = GetWrapMode(sc->wrapModeV);
                }
                };

            switch (sc->type) {
            case ShapeType::Rectangle: {
                RenderBatch& batch = rects[key];
                ConfigureBatch(batch);
                batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x, tf->size.y, sc->uvOffset);
                break;
            }
            case ShapeType::Triangle: {
                RenderBatch& batch = tris[key];
                ConfigureBatch(batch);
                batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x, 0.0f, sc->uvOffset);
                break;
            }
            case ShapeType::Circle: {
                RenderBatch& batch = circs[key];
                ConfigureBatch(batch);
                batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x * 0.5f, 0.0f, sc->uvOffset);
                break;
            }
            default: break;
            }
        }
    }

    // --- HELPER: FlushBatches (Draws and clears the maps) ---
    void FlushBatches(
        std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& rects,
        std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& tris,
        std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& circs)
    {
        // --- Rectangles ---
        for (auto& kv : rects) {
            RenderBatch& batch = kv.second;
            if (batch.positions.empty()) continue;

            if (batch.enableTexture && !batch.textureName.empty()) {
                auto* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
                if (tex) tex->SetTextureMode(batch.wrapU, batch.wrapV);
            }

            InstancingSystem::InstancedParams params;
            params.textured = batch.enableTexture;
            params.textureName = batch.textureName;
            params.tint = { 1,1,1 };
            params.uvSize = batch.uvSize;

            InstancingSystem::RenderInstancedRectanglesBatch(
                batch.positions.data(), batch.colors.data(), batch.rotations.data(),
                batch.scales.data(), batch.dim1.data(), batch.dim2.data(),
                batch.uvOffsets.data(),
                (int)batch.positions.size(), params);

            batch.Clear();
        }

        // --- Triangles ---
        for (auto& kv : tris) {
            RenderBatch& batch = kv.second;
            if (batch.positions.empty()) continue;

            if (batch.enableTexture && !batch.textureName.empty()) {
                auto* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
                if (tex) tex->SetTextureMode(batch.wrapU, batch.wrapV);
            }
            InstancingSystem::InstancedParams params;
            params.textured = batch.enableTexture;
            params.textureName = batch.textureName;
            params.tint = { 1,1,1 };
            params.uvSize = batch.uvSize;

            InstancingSystem::RenderInstancedTrianglesBatch(
                batch.positions.data(), batch.colors.data(), batch.rotations.data(),
                batch.scales.data(), batch.dim1.data(),
                batch.uvOffsets.data(),
                (int)batch.positions.size(), params);

            batch.Clear();
        }

        // --- Circles ---
        for (auto& kv : circs) {
            RenderBatch& batch = kv.second;
            if (batch.positions.empty()) continue;

            if (batch.enableTexture && !batch.textureName.empty()) {
                auto* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
                if (tex) tex->SetTextureMode(batch.wrapU, batch.wrapV);
            }
            InstancingSystem::InstancedParams params;
            params.textured = batch.enableTexture;
            params.textureName = batch.textureName;
            params.tint = { 1,1,1 };
            params.uvSize = batch.uvSize;

            InstancingSystem::RenderInstancedCirclesBatch(
                batch.positions.data(), batch.colors.data(), batch.rotations.data(),
                batch.scales.data(), batch.dim1.data(),
                batch.uvOffsets.data(),
                (int)batch.positions.size(), 32, params);

            batch.Clear();
        }
    }

    void RenderScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;
        using Component::Rendering;
        Rendering& renderer = Rendering::Get();
        if (!renderer.shdr_pgm_instanced) return;

        const auto& shapeArray = globalECS.GetComponentArray<ShapeComponent>();

        // 1. PRE-SORT into World vs Screen lists
        std::vector<Entity> worldEntities;
        std::vector<Entity> screenEntities;
        worldEntities.reserve(shapeArray.indexToEntity.size());
        screenEntities.reserve(shapeArray.indexToEntity.size());

        for (const Entity& id : shapeArray.indexToEntity) {
            auto* sc = globalECS.GetComponent<ShapeComponent>(id);
            if (sc->isWorldSpace) worldEntities.push_back(id);
            else                  screenEntities.push_back(id);
        }

        // -------------------------------------------------------
        // PASS 1: WORLD SPACE (Uses Camera Matrices)
        // -------------------------------------------------------
        renderer.shdr_pgm_instanced->Use();
        renderer.shdr_pgm_instanced->SetUniform("uView", renderer.viewMatrix);
        renderer.shdr_pgm_instanced->SetUniform("uProjection", renderer.projectionMatrix);

        // Store lines for later
        std::vector<std::pair<TransformComponent*, ShapeComponent*>> lineQueue;

        BatchEntities(worldEntities, globalECS, rectBatches, triBatches, circBatches, lineQueue);
        FlushBatches(rectBatches, triBatches, circBatches);

        // Flush World Lines (Non-instanced)
        if (!lineQueue.empty()) {
            renderer.shdr_pgm->Use();
            renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
            renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

            for (auto& pair : lineQueue) {
                TransformComponent* tf = pair.first;
                ShapeComponent* sc = pair.second;
                Shapes::CreateLine(sc->p1 + tf->position, sc->p2 + tf->position, sc->thickness, sc->color, sc->isWorldSpace);
            }
            lineQueue.clear();
        }

        // -------------------------------------------------------
        // PASS 2: SCREEN SPACE (Uses UI Matrices)
        // -------------------------------------------------------
        renderer.shdr_pgm_instanced->Use();
        // View is Identity (No camera movement)
        renderer.shdr_pgm_instanced->SetUniform("uView", glm::mat4(1.0f));

        // Fixed UI Projection (Centered Origin)
        float aspect = (float)GLApp::width / (float)GLApp::height;
        float h = 100.0f; // Base Height (Matches your engine standard)
        float w = h * aspect;
        glm::mat4 uiProj = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
        renderer.shdr_pgm_instanced->SetUniform("uProjection", uiProj);

        BatchEntities(screenEntities, globalECS, rectBatches, triBatches, circBatches, lineQueue);
        FlushBatches(rectBatches, triBatches, circBatches);

        // Flush Screen Lines
        if (!lineQueue.empty()) {
            renderer.shdr_pgm->Use();
            renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
            renderer.shdr_pgm->SetUniform("uProjection", uiProj);

            for (auto& pair : lineQueue) {
                TransformComponent* tf = pair.first;
                ShapeComponent* sc = pair.second;
                Shapes::CreateLine(sc->p1 + tf->position, sc->p2 + tf->position, sc->thickness, sc->color, sc->isWorldSpace);
            }
        }

        renderer.shdr_pgm_instanced->Use(); // Restore state

        // 5. Draw Text / UI (Now via UI namespace)
        Scene::UI::RenderTextAndCursor(globalECS);
    }

    void UpdateScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;
        Scene::UI::UpdateButtons(globalECS);
        Scene::UI::UpdateInputs(globalECS);
    }
}