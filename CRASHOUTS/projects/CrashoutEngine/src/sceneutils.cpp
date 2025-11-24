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

#include "shapescomponent.h"
#include "transformcomponent.h"
#include "namecomponent.h"
#include "textcomponent.h"
#include "shapes.h"
#include "uilayoutcomponent.h"
#include "textinputcomponent.h"
#include "buttoncomponent.h"
#include "tilemapcomponent.h"

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
                if (globalECS.HasComponent<TransformComponent>(id)) {
                    TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);

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

                    float compensatedScale = tc->scale / RenderCamera::Camera::g_zoom;

                    std::string font = tc->font;
                    if (font == "") font = "Exo2-Regular";

                    if (tc->centered)
                        gTextRenderer->RenderCenteredText(tc->text, newOffset.x, newOffset.y, compensatedScale, tc->color, font, tc->isWorldSpace);
                    else
                        gTextRenderer->RenderText(tc->text, newOffset.x, newOffset.y, compensatedScale, tc->color, font, tc->isWorldSpace);

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
                                    cursorColor           // color
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
            // Call UI items
            // Button objects
            const ECS::ComponentArray<ButtonComponent>& buttonArray = globalECS.GetComponentArray<ButtonComponent>();

            CEVec2 mouseWorld = InputManager::GetMouseWorldPos();

            for (const Entity& id : buttonArray.indexToEntity) {
                ButtonComponent* b = globalECS.GetComponent<ButtonComponent>(id);
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);

                // AABB the button
                float halfW = t->size.x * 0.5f;
                float halfH = t->size.y * 0.5f;
                CEVec2 aa{ t->position.x - halfW, t->position.y - halfH };
                CEVec2 bb{ t->position.x + halfW, t->position.y + halfH };

                // Check if mouse pos within
                if (mouseWorld.x >= aa.x && mouseWorld.x <= bb.x && mouseWorld.y >= aa.y && mouseWorld.y <= bb.y) {
                    if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                        b->buttonState = ButtonState::PRESSED;
                        ShapeComponent* s = globalECS.GetComponent<ShapeComponent>(id);
                        TextComponent* text = globalECS.GetComponent<TextComponent>(id);
                        if (s) s->color = b->shapePressedColor;
                        if (text) text->color = b->textPressedColor;

                        continue;
                    }
                    else {
                        b->buttonState = ButtonState::HOVERED;
                        ShapeComponent* s = globalECS.GetComponent<ShapeComponent>(id);
                        TextComponent* text = globalECS.GetComponent<TextComponent>(id);
                        if (s) s->color = b->shapeHoveredColor;
                        if (text) text->color = b->textHoveredColor;

                        continue;
                    }
                }
                else if (b->buttonState != ButtonState::NORMAL) {
                    b->buttonState = ButtonState::NORMAL;
                    ShapeComponent* s = globalECS.GetComponent<ShapeComponent>(id);
                    TextComponent* text = globalECS.GetComponent<TextComponent>(id);
                    if (s) s->color = b->shapeNormalColor;
                    if (text) text->color = b->textNormalColor;

                    continue;
                }
            }
        }

        /*!
        @brief      Updates text input focus logic and handles keyboard events for typing.
        @param      globalECS Reference to the ECS system.
        */
        void UpdateInputs(ECS& globalECS) {
            // Text Input objects
            static Entity selectedTextInput = (Entity)-1;
            const ECS::ComponentArray<TextInputComponent>& textInputArray = globalECS.GetComponentArray<TextInputComponent>();

            CEVec2 mouseWorld = InputManager::GetMouseWorldPos();
            bool clickedThisFrame = InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            Entity newSelection = (Entity)-1;

            // 1. Detect clicks on input fields
            for (const Entity& id : textInputArray.indexToEntity) {
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);

                // AABB check
                float halfW = t->size.x * 0.5f;
                float halfH = t->size.y * 0.5f;
                CEVec2 aa{ t->position.x - halfW, t->position.y - halfH };
                CEVec2 bb{ t->position.x + halfW, t->position.y + halfH };

                if (mouseWorld.x >= aa.x && mouseWorld.x <= bb.x && mouseWorld.y >= aa.y && mouseWorld.y <= bb.y) {
                    if (clickedThisFrame) {
                        newSelection = id; // We clicked this input!
                    }
                }
            }

            // 2. Update the focus state
            if (clickedThisFrame) {
                if (selectedTextInput != -1 && selectedTextInput != newSelection) {
                    // Unfocus the *old* one
                    if (auto* oldInput = globalECS.GetComponent<TextInputComponent>(selectedTextInput)) {
                        oldInput->isFocused = false;
                    }
                }
                if (newSelection != -1) {
                    // Focus the *new* one
                    globalECS.GetComponent<TextInputComponent>(newSelection)->isFocused = true;
                }
                selectedTextInput = newSelection;
            }

            // 3. Process Input for the selected field
            if (selectedTextInput != -1) {
                TextInputComponent* textInput = globalECS.GetComponent<TextInputComponent>(selectedTextInput);
                TextComponent* text = globalECS.GetComponent<TextComponent>(selectedTextInput);

                if (textInput && text) {
                    // --- A. Handle Character Typing ---
                    std::string typed = InputManager::GetTypedCharacters();
                    if (!typed.empty()) {
                        // Calculate how much space is left
                        size_t currentLength = text->text.length();
                        if (currentLength < textInput->maxCharacters) {
                            size_t remainingSpace = textInput->maxCharacters - currentLength;

                            // If the typed string is too long, truncate it
                            if (typed.length() > remainingSpace) {
                                typed = typed.substr(0, remainingSpace);
                            }

                            // Now, insert the (potentially truncated) string
                            text->text.insert(textInput->cursorPos, typed);
                            textInput->cursorPos += typed.length();
                        }
                        // If currentLength is already at max, do nothing.
                    }

                    // --- B. Handle Backspace ---
                    if (InputManager::IsKeyTriggered(GLFW_KEY_BACKSPACE)) {
                        if (textInput->cursorPos > 0 && !text->text.empty()) {
                            // Erase one character before the cursor
                            text->text.erase(textInput->cursorPos - 1, 1);
                            textInput->cursorPos--;
                        }
                    }

                    // --- C. Handle Delete Key ---
                    if (InputManager::IsKeyTriggered(GLFW_KEY_DELETE)) {
                        if (textInput->cursorPos < text->text.length()) {
                            // Erase one character at the cursor
                            text->text.erase(textInput->cursorPos, 1);
                        }
                    }

                    // --- D. Handle Arrow Keys ---
                    if (InputManager::IsKeyTriggered(GLFW_KEY_LEFT)) {
                        if (textInput->cursorPos > 0) textInput->cursorPos--;
                    }
                    if (InputManager::IsKeyTriggered(GLFW_KEY_RIGHT)) {
                        if (textInput->cursorPos < text->text.length()) textInput->cursorPos++;
                    }

                    // --- E. Handle Enter Key for next line ---
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

    // Helper to convert simple Integers to TextureMode
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
        std::vector<CEColor> colors; // Changed to CEColor to match new InstanceSystem
        std::vector<float> rotations;
        std::vector<float> scales;
        std::vector<float> dim1; // Width/Side/Radius
        std::vector<float> dim2; // Height

        CEVec2 uvSize{ 1.0f, 1.0f };
        std::string textureName = "";
        bool enableTexture = false;
        TextureMode wrapU = TextureMode::ClampToEdge;
        TextureMode wrapV = TextureMode::ClampToEdge;

        void Clear() {
            positions.clear(); colors.clear(); rotations.clear();
            scales.clear(); dim1.clear(); dim2.clear();
            uvSize = { 1.0f, 1.0f };
            textureName = "";
            enableTexture = false;
            wrapU = TextureMode::ClampToEdge;
            wrapV = TextureMode::ClampToEdge;
        }

        void Add(const CEVec3& pos, const CEColor& col, float rot, float scale, float d1, float d2) {
            positions.push_back(pos);
            colors.push_back(col);
            rotations.push_back(rot);
            scales.push_back(scale);
            dim1.push_back(d1);
            dim2.push_back(d2);
        }
    };

    std::unordered_map<std::string, RenderBatch> rectBatches;
    std::unordered_map<std::string, RenderBatch> triBatches;
    std::unordered_map<std::string, RenderBatch> circBatches;

    void RenderScene(CrashoutsEngine::ECS& globalECS) {
        /*
        using namespace CrashoutsEngine;
        using Component::Rendering;

        // For Drawing Text Components
        const ECS::ComponentArray<TextComponent>& textArray = globalECS.GetComponentArray<TextComponent>();
        if (!gTextRenderer) {
            gTextRenderer = std::make_unique<TextRenderer>();
        }

        for (const Entity& id : textArray.indexToEntity) {
            TextComponent* tc = globalECS.GetComponent<TextComponent>(id);
            if (globalECS.HasComponent<TransformComponent>(id)) {
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);

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

                float compensatedScale = tc->scale / RenderCamera::Camera::g_zoom;

                std::string font = tc->font;
                if (font == "") font = "Exo2-Regular";

                if (tc->centered)
                    gTextRenderer->RenderCenteredText(tc->text, newOffset.x, newOffset.y, compensatedScale, tc->color, font, tc->isWorldSpace);
                else
                    gTextRenderer->RenderText(tc->text, newOffset.x, newOffset.y, compensatedScale, tc->color, font, tc->isWorldSpace);

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
                                cursorColor           // color
                            );
                        }
                    }
                }

            }
        }
        */

        using namespace CrashoutsEngine;
        using Component::Rendering;

        Rendering& renderer = Rendering::Get();
        if (!renderer.shdr_pgm_instanced) return;

        // 1. Setup Shader
        renderer.shdr_pgm_instanced->Use();
        renderer.shdr_pgm_instanced->SetUniform("uView", renderer.viewMatrix);
        renderer.shdr_pgm_instanced->SetUniform("uProjection", renderer.projectionMatrix);

        // 2. Clear previous batches
        for (auto& kv : rectBatches) kv.second.Clear();
        for (auto& kv : triBatches)  kv.second.Clear();
        for (auto& kv : circBatches) kv.second.Clear();

        // 3. Iterate ECS
        const ECS::ComponentArray<ShapeComponent>& shapeArray = globalECS.GetComponentArray<ShapeComponent>();

        for (const Entity& id : shapeArray.indexToEntity) {
            if (!globalECS.HasComponent<TransformComponent>(id)) continue;

            TransformComponent* tf = globalECS.GetComponent<TransformComponent>(id);
            ShapeComponent* sc = globalECS.GetComponent<ShapeComponent>(id);

            std::string batchKey;
            if (sc->enableTexture) {
                batchKey = sc->textureName + "_" +
                    std::to_string(sc->uvSize.x) + "_" +
                    std::to_string(sc->uvSize.y) + "_" +
                    std::to_string(sc->wrapModeU) + "_" +
                    std::to_string(sc->wrapModeV);
            }
            else {
                batchKey = "NoTexture";
            }

            CEColor colorNorm(
                sc->color.x(),
                sc->color.y(),
                sc->color.z(),
                sc->color.w()
            );


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
            case ShapeType::Rectangle:
            {
                RenderBatch& batch = rectBatches[batchKey];
                ConfigureBatch(batch);
                batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x, tf->size.y);
                break;
            }
            case ShapeType::Triangle:
            {
                RenderBatch& batch = triBatches[batchKey];
                ConfigureBatch(batch);
                batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x, 0.0f);
                break;
            }
            case ShapeType::Circle:
            {
                RenderBatch& batch = circBatches[batchKey];
                ConfigureBatch(batch);
                batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x * 0.5f, 0.0f);
                break;
            }
            case ShapeType::Line:
                // Lines are not instanced in this system, render immediately
                renderer.shdr_pgm->Use();
                renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
                renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);
                Shapes::CreateLine(sc->p1 + tf->position, sc->p2 + tf->position, sc->thickness, sc->color, sc->isWorldSpace);
                renderer.shdr_pgm_instanced->Use(); // Switch back
                break;
            }
        }

        // 4. Render Batches

        // --- Rectangles ---
        for (auto& kv : rectBatches) {
            RenderBatch& batch = kv.second;
            if (batch.positions.empty()) continue;

            if (batch.enableTexture && !batch.textureName.empty()) {
                Assets::TextureAsset* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
                if (tex) {
                    tex->SetTextureMode(batch.wrapU, batch.wrapV);
                }
            }

            InstancingSystem::InstancedParams params;
            params.textured = batch.enableTexture;
            params.textureName = batch.textureName;
            params.tint = { 1,1,1 };
            params.uvSize = batch.uvSize;

            InstancingSystem::RenderInstancedRectanglesBatch(
                batch.positions.data(),
                batch.colors.data(), // Now passing CEColor*
                batch.rotations.data(),
                batch.scales.data(),
                batch.dim1.data(),
                batch.dim2.data(),
                static_cast<int>(batch.positions.size()),
                params
            );
        }

        // --- Triangles ---
        for (auto& kv : triBatches) {
            RenderBatch& batch = kv.second;
            if (batch.positions.empty()) continue;

            if (batch.enableTexture && !batch.textureName.empty()) {
                Assets::TextureAsset* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
                if (tex) {
                    tex->SetTextureMode(batch.wrapU, batch.wrapV);
                }
            }

            InstancingSystem::InstancedParams params;
            params.textured = batch.enableTexture;
            params.textureName = batch.textureName;
            params.tint = { 1,1,1 };
            params.uvSize = batch.uvSize;

            InstancingSystem::RenderInstancedTrianglesBatch(
                batch.positions.data(),
                batch.colors.data(),
                batch.rotations.data(),
                batch.scales.data(),
                batch.dim1.data(),
                static_cast<int>(batch.positions.size()),
                params
            );
        }

        // --- Circles ---
        for (auto& kv : circBatches) {
            RenderBatch& batch = kv.second;
            if (batch.positions.empty()) continue;

            if (batch.enableTexture && !batch.textureName.empty()) {
                Assets::TextureAsset* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
                if (tex) {
                    tex->SetTextureMode(batch.wrapU, batch.wrapV);
                }
            }

            InstancingSystem::InstancedParams params;
            params.textured = batch.enableTexture;
            params.textureName = batch.textureName;
            params.tint = { 1,1,1 };
            params.uvSize = batch.uvSize;

            InstancingSystem::RenderInstancedCirclesBatch(
                batch.positions.data(),
                batch.colors.data(),
                batch.rotations.data(),
                batch.scales.data(),
                batch.dim1.data(),
                static_cast<int>(batch.positions.size()),
                32,
                params
            );
        }

        // 5. Draw Text / UI (Now via UI namespace)
        Scene::UI::RenderTextAndCursor(globalECS);
    }

    void UpdateScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;
        Scene::UI::UpdateButtons(globalECS);
        Scene::UI::UpdateInputs(globalECS);
    }
}