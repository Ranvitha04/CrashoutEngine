/*!
@file        sceneutils.cpp
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements function calls that run every frame for the scenes

This file implements
*/

#include "rendering.h"
#include "textrenderer.h"
#include "textureasset.h"
#include "assetmanager.h"
#include "inputmanager.h"
#include "camera.h"
#include "settings.h"
#include "rendererconnector.h"
#include "sceneutils.h"

namespace Scene {
    void RenderScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;
        using Component::Rendering;

        Rendering& renderer = Rendering::Get();
        if (!renderer.shdr_pgm) return;

        renderer.shdr_pgm->Use();

        renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
        renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

        // Collect all non-textured squares here
        std::vector<CEVec3> instancedPos;
        std::vector<CEVec3> instancedColor;
        std::vector<float> instancedRot;
        std::vector<float> instancedScales;

        // Pre-allocate memory
        const ECS::ComponentArray<ShapeComponent>& shapeArray = globalECS.GetComponentArray<ShapeComponent>();
        //std::cout << shapeArray.dense.size() << std::endl;
        // Reserve based on the total number of shapes as a rough estimate
        size_t shapeSize{ shapeArray.dense.size() };
        instancedPos.reserve(shapeSize);
        instancedColor.reserve(shapeSize);
        instancedRot.reserve(shapeSize);
        instancedScales.reserve(shapeSize);
        
        for (const Entity& id : shapeArray.indexToEntity) {
            // Get entity number
            TransformComponent* tf = globalECS.GetComponent<TransformComponent>(id);
            ShapeComponent* sc = globalECS.GetComponent<ShapeComponent>(id);
            // Can add more data to create rect like color etc
            if (globalECS.HasComponent<ShapeComponent>(id)) {

                CEVec3 pos{ 0,0,0 };
                CEVec2 scale{ 1.f, 1.f };
                float rot{ 0.f };
                if (tf) {
                    pos = tf->position;
                    scale = tf->size;
                    rot = tf->rotation;
                }

                // Strictly respect the ECS toggle
                bool useTexture = sc->enableTexture;

                if (useTexture) {
                    using namespace Assets;
                    TextureAsset* tex = Assets::AssetManager::GetInstance().Get<TextureAsset>(sc->textureName);
                    if (tex) {
                        tex->SetTextureMode(
                            static_cast<TextureMode>(sc->wrapModeU),
                            static_cast<TextureMode>(sc->wrapModeV)
                        );
                    }
                }

                switch (sc->type) {
                // ==========================================RECTANGLE===================================================
                case ShapeType::Rectangle:
                    if (useTexture) {
                        Shapes::CreateTexturedRectangle(tf->size,
                            sc->textureName, pos, rot, sc->color * (1 / 255.f), sc->uvSize);
                        break;
                    } else {
                        Shapes::CreateRectangle(tf->size,
                            pos, rot, sc->color * (1 / 255.f)
                        );
                        break;
                    }
                // ============================================LINE======================================================
                case ShapeType::Line:
                    Shapes::CreateLine(tf->size, sc->p1 + pos, sc->p2 + pos, sc->thickness, rot, sc->color * (1 / 255.f)); break;
                // ==========================================TRIANGLE====================================================
                case ShapeType::Triangle:
                    if (useTexture) {
                        Shapes::CreateTexturedTriangle(tf->size, sc->textureName,
                            pos, rot, sc->color * (1 / 255.f), sc->uvSize
                        );
                        break;
                    } else {
                        Shapes::CreateTriangle(tf->size, pos,
                            rot, sc->color * (1 / 255.f)
                        );
                        break;
                    }
                // ==========================================CIRCLE======================================================
                case ShapeType::Circle:
                    Shapes::CreateCircle(tf->size, tf->position, 32, tf->rotation, sc->color * (1 / 255.f)); break;
                }
            }
        }
    
        // Perform instancing
        int instancedCount = static_cast<int>(instancedPos.size());
        if (instancedCount > 0) {
           
        }

		// For Drawing Text Components
		const ECS::ComponentArray<TextComponent>& textArray = globalECS.GetComponentArray<TextComponent>();
        if (!gTextRenderer) {  
            gTextRenderer = std::make_unique<TextRenderer>();
        }
        
        for (const Entity& id : textArray.indexToEntity) {
            TextComponent* tc = globalECS.GetComponent<TextComponent>(id);
            if (globalECS.HasComponent<TransformComponent>(id)) {
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);

                CEVec2 newOffset = CEVec2{ t->position.x + tc->offset.x, t->position.y + tc->offset.y};
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

                if(tc->centered)
                    gTextRenderer->RenderCenteredText(tc->text, newOffset.x, newOffset.y, compensatedScale, tc->color, font, tc->isWorldSpace);
                else
					gTextRenderer->RenderText(tc->text, newOffset.x, newOffset.y, compensatedScale, tc->color, font, tc->isWorldSpace);
            }
        }

    }

    void UpdateScene(CrashoutsEngine::ECS& globalECS) {
        using namespace CrashoutsEngine;

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

            // std::cout << "aa is " << aa.x << ", " << aa.y << " bb is " << bb.x << ", " << bb.y << " mouse is " << mouseWorld.x << ", " << mouseWorld.y << std::endl;

            // Check if mouse pos within
            if (mouseWorld.x >= aa.x && mouseWorld.x <= bb.x && mouseWorld.y >= aa.y && mouseWorld.y <= bb.y) {
                if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    b->buttonState = ButtonState::PRESSED;
                    // std::cout << globalECS.GetComponent<NameComponent>(id)->name << " button is pressed " << std::endl;
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
   
        // Text Input objects
        static Entity selectedTextInput = (Entity)- 1;
        const ECS::ComponentArray<TextInputComponent>& textInputArray = globalECS.GetComponentArray<TextInputComponent>();

        bool clickedThisFrame = InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        Entity newSelection = (Entity)-1;
        
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

        // Now, update the focus state
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
    
        if (selectedTextInput != -1) {
            TextInputComponent* textInput = globalECS.GetComponent<TextInputComponent>(selectedTextInput);
            TextComponent* text = globalECS.GetComponent<TextComponent>(selectedTextInput);

            if (textInput && text) {
                // --- A. Handle Character Typing ---
                std::string typed = InputManager::GetTypedCharacters();
                if (!typed.empty()) {
                    if (text->text.length() + typed.length() <= textInput->maxCharacters) {
                        // Insert the typed chars at the cursor position
                        text->text.insert(textInput->cursorPos, typed);
                        textInput->cursorPos += typed.length();
                    }
                }

                // --- B. Handle Backspace ---
                if (InputManager::IsKeyPressed(GLFW_KEY_BACKSPACE)) {
                    if (textInput->cursorPos > 0 && !text->text.empty()) {
                        // Erase one character before the cursor
                        text->text.erase(textInput->cursorPos - 1, 1);
                        textInput->cursorPos--;
                    }
                }

                // --- C. Handle Delete Key ---
                if (InputManager::IsKeyPressed(GLFW_KEY_DELETE)) {
                    if (textInput->cursorPos < text->text.length()) {
                        // Erase one character at the cursor
                        text->text.erase(textInput->cursorPos, 1);
                    }
                }

                // --- D. Handle Arrow Keys ---
                if (InputManager::IsKeyPressed(GLFW_KEY_LEFT)) {
                    if (textInput->cursorPos > 0) textInput->cursorPos--;
                }
                if (InputManager::IsKeyPressed(GLFW_KEY_RIGHT)) {
                    if (textInput->cursorPos < text->text.length()) textInput->cursorPos++;
                }

                // --- E. Handle Enter Key for next line ---
                if (InputManager::IsKeyTriggered(GLFW_KEY_ENTER)) {
                    text->text.insert(textInput->cursorPos, "\n");
                    textInput->cursorPos += typed.length();
                }
            }
        }
    }

}