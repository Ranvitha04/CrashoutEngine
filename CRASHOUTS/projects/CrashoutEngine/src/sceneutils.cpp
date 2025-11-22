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

namespace UI {
    using namespace Component;
    using namespace CrashoutsEngine;

    /*!
    @brief      Renders all text components and handles the blinking cursor for focused inputs.
    @param      globalECS Reference to the ECS system to retrieve components.
    */
    void RenderTextAndCursor(ECS& globalECS) {
        Component::Rendering& renderer = Component::Rendering::Get();

        renderer.shdr_pgm->Use(); // Switch back to standard shader for text/UI
        renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
        renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

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
                            CEVec3 cursorColor = { .0f, .0f, .0f }; // White

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
}

namespace Scene {

    using namespace Component;

    struct RenderBatch {
        std::vector<CEVec3> positions;
        std::vector<CEVec3> colors;
        std::vector<float> rotations;
        std::vector<float> scales;
        // Specifics per shape
        std::vector<float> dim1; // Width (Rect) or Side (Tri) or Radius (Circ)
        std::vector<float> dim2; // Height (Rect) - unused for Tri/Circ

        void Clear() {
            positions.clear(); colors.clear(); rotations.clear();
            scales.clear(); dim1.clear(); dim2.clear();
        }

        void Add(const CEVec3& pos, const CEVec3& col, float rot, float scale, float d1, float d2) {
            positions.push_back(pos);
            colors.push_back(col);
            rotations.push_back(rot);
            scales.push_back(scale);
            dim1.push_back(d1);
            dim2.push_back(d2);
        }
    };

    // Maps TextureName -> Batch Data
    std::unordered_map<std::string, RenderBatch> rectBatches;
    std::unordered_map<std::string, RenderBatch> triBatches;
    std::unordered_map<std::string, RenderBatch> circBatches;

    void RenderScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;
        using Component::Rendering;

        Rendering& renderer = Rendering::Get();
        if (!renderer.shdr_pgm) return;

        // 1. Setup Shader Global Uniforms
        renderer.shdr_pgm_instanced.Use(); // Make sure to use the instanced shader!
        renderer.shdr_pgm_instanced.SetUniform("uView", renderer.viewMatrix);
        renderer.shdr_pgm_instanced.SetUniform("uProjection", renderer.projectionMatrix);

        // 2. Clear previous frame batches
        for (auto& kv : rectBatches) kv.second.Clear();
        for (auto& kv : triBatches)  kv.second.Clear();
        for (auto& kv : circBatches) kv.second.Clear();

        // 3. Iterate ECS and Fill Batches
        const ECS::ComponentArray<ShapeComponent>& shapeArray = globalECS.GetComponentArray<ShapeComponent>();

        for (const Entity& id : shapeArray.indexToEntity) {
            if (!globalECS.HasComponent<TransformComponent>(id)) continue;

            TransformComponent* tf = globalECS.GetComponent<TransformComponent>(id);
            ShapeComponent* sc = globalECS.GetComponent<ShapeComponent>(id);

            // Determine Batch Key (Texture Name). Empty string "" means untextured.
            std::string batchKey = sc->enableTexture ? sc->textureName : "";

            // Prepare data
            CEVec3 colorNorm = sc->color * (1.0f / 255.0f);

            switch (sc->type) {
            case ShapeType::Rectangle:
                // If the key doesn't exist, map creates it automatically
                rectBatches[batchKey].Add(
                    tf->position, colorNorm, tf->rotation,
                    1.0f, // Pass 1.0 scale, we send Size into W/H
                    tf->size.x, tf->size.y // W, H
                );
                break;

            case ShapeType::Triangle:
                triBatches[batchKey].Add(
                    tf->position, colorNorm, tf->rotation,
                    1.0f,
                    tf->size.x, // Side length (assuming equilateral based on size.x)
                    0.0f
                );
                break;

            case ShapeType::Circle:
                circBatches[batchKey].Add(
                    tf->position, colorNorm, tf->rotation,
                    1.0f,
                    tf->size.x * 0.5f, // Radius (half of width)
                    0.0f
                );
                break;

            case ShapeType::Line:
                // Instancing lines is complex and not in your instanceSystem.
                // Fallback to immediate mode for lines.
                renderer.shdr_pgm->Use(); // Switch back to standard shader for lines
                renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
                renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);
                Shapes::CreateLine(sc->p1 + tf->position, sc->p2 + tf->position, sc->thickness, colorNorm, sc->isWorldSpace);
                renderer.shdr_pgm_instanced.Use(); // Switch back
                break;
            }
        }

        // 4. Render Batches

        // --- Rectangles ---
        for (auto& kv : rectBatches) {
            if (kv.second.positions.empty()) continue;

            InstancingSystem::InstancedParams params;
            params.textured = !kv.first.empty();
            params.textureName = kv.first;
            params.tint = { 1,1,1 }; // Default tint
            params.uvSize = { 1,1 };

            InstancingSystem::RenderInstancedRectanglesBatch(
                kv.second.positions.data(),
                kv.second.colors.data(),
                kv.second.rotations.data(),
                kv.second.scales.data(),
                kv.second.dim1.data(), // Widths
                kv.second.dim2.data(), // Heights
                static_cast<int>(kv.second.positions.size()),
                params
            );
        }

        // --- Triangles ---
        for (auto& kv : triBatches) {
            if (kv.second.positions.empty()) continue;

            InstancingSystem::InstancedParams params;
            params.textured = !kv.first.empty();
            params.textureName = kv.first;
            params.tint = { 1,1,1 };
            params.uvSize = { 1,1 };

            InstancingSystem::RenderInstancedTrianglesBatch(
                kv.second.positions.data(),
                kv.second.colors.data(),
                kv.second.rotations.data(),
                kv.second.scales.data(),
                kv.second.dim1.data(), // Side lengths
                static_cast<int>(kv.second.positions.size()),
                params
            );
        }

        // --- Circles ---
        for (auto& kv : circBatches) {
            if (kv.second.positions.empty()) continue;

            InstancingSystem::InstancedParams params;
            params.textured = !kv.first.empty();
            params.textureName = kv.first;
            params.tint = { 1,1,1 };
            params.uvSize = { 1,1 };

            InstancingSystem::RenderInstancedCirclesBatch(
                kv.second.positions.data(),
                kv.second.colors.data(),
                kv.second.rotations.data(),
                kv.second.scales.data(),
                kv.second.dim1.data(), // Radii
                static_cast<int>(kv.second.positions.size()),
                32, // Segments
                params
            );
        }

        // 5. Draw Text / UI 
        // Logic delegated to UI namespace for organization
        UI::RenderTextAndCursor(globalECS);
    }

    void UpdateScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;

        // Delegate Button Logic
        UI::UpdateButtons(globalECS);

        // Delegate Input Field Logic
        UI::UpdateInputs(globalECS);
    }

}