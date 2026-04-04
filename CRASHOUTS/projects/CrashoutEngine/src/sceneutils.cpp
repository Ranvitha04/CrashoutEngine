/*!
@file        sceneutils.cpp
@author      Renfred Lee
@date        29/09/2025

@brief       Implements function calls that run every frame for the scenes for shapes,
			 and texts. As well as the Update functionalities for GUI.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.


*/

#include "rendering.h"
#include "textrenderer.h"
#include "textureasset.h"
#include "assetmanager.h"
#include "inputmanager.h"
#include "frameratecontroller.h"
#include "camera.h"
//#include "settings.h"
#include "rendererconnector.h"
#include "sceneutils.h"
#include "engine.h"
#include "instanceSystem.h"
#include "glapp.h"
#include "debugcollision.h"

#include "guicomponent.h"
#include "particlecomponent.h"
#include "shapescomponent.h"
#include "transformcomponent.h"
#include "textcomponent.h"
#include "shapes.h"
#include "uilayoutcomponent.h"
#include "textinputcomponent.h"
#include "buttoncomponent.h"
//#include "tilemapcomponent.h"
#include "particleSystem.h"
#include "spriteAnimation.h"
#include "jobsystem.h"
#include "hierarchycomponent.h"
//#include "camera.h"
#include "performanceSystem.h"
#include "CEMath.h"

#include <fstream>
#include <mutex>
#include <sstream>

#include <filesystem>

#define FONT_SCALE 0.05f

extern int gameTextureWidth;
extern int gameTextureHeight;

namespace Scene {

	using namespace Component;
	using namespace CrashoutsEngine;
	using namespace CrashoutsEngine::Assets;

	static std::unique_ptr<TextRenderer> gTextRenderer = nullptr;
    static Entity gForcedHoveredButton = 0;

	CEVec3 GetMouseInWorld() {

		double u, v;
		glfwGetCursorPos(GLApp::ptr_window, &u, &v);
		u /= GLApp::width;
		v /= GLApp::height;

		const float ndcX = (float)u * 2.f - 1.f;
		const float ndcY = 1.f - ((float)v * 2.f);
		const glm::mat4 VP = Component::Rendering::Get().projectionMatrix * Component::Rendering::Get().viewMatrix;
		const glm::mat4 invVP = glm::inverse(VP);
		glm::vec4 w = invVP * glm::vec4(ndcX, ndcY, 0.f, 1.f);
		if (w.w != 0.f) w /= w.w;
		return CEVec3{ (float)w.x, (float)w.y, (float)w.z };
	}

	// ============================================================================
	// UI SUB-NAMESPACE
	// ============================================================================
	namespace UI {

        void SetForcedHoveredButton(Entity entity) {
            gForcedHoveredButton = entity;
        }

        void ClearForcedHoveredButton() {
            gForcedHoveredButton = 0;
        }

		void LoadDialogue(TextComponent& tc) {
			if (tc.dialogueFilePath.empty() || tc.dialogueID.empty()) return;

			// Use the ResolvePath helper so dialogue files can live in 
			// the Game folder OR fallback to the Core Engine folder.
			std::string fullPath = CrashoutsEngine::Assets::ResolvePath(tc.dialogueFilePath);

			// If ResolvePath couldn't find it, we stop here
			if (fullPath.empty()) {
				std::cerr << "[SceneUtils] Failed to resolve dialogue file: " << tc.dialogueFilePath << "\n";
				return;
			}

			std::ifstream file(fullPath);
			if (!file.is_open()) return;

			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string content = buffer.str();
			file.close();

			rapidjson::Document doc;
			doc.Parse(content.c_str());

			if (!doc.HasParseError()) {
				if (doc.HasMember(tc.dialogueID.c_str())) {
					const auto& val = doc[tc.dialogueID.c_str()];
					if (val.IsString()) {
						tc.text = val.GetString();
					}
				}
			}
		}

		static void InitText(ECS& globalECS) {
			const ECS::ComponentArray<TextComponent>& textArray = globalECS.GetComponentArray<TextComponent>();
			for (const Entity& id : textArray.indexToEntity) {
				if (!IsEntityActive(globalECS, id)) continue;
				TextComponent* tc = globalECS.GetComponent<TextComponent>(id);
				if (tc) {
					LoadDialogue(*tc);
				}
			}
		}

		/*
		@brief      HELPER: Calculates the final UI position (Center-Origin Units)
		@param      This ensures Render and Collision logic always agree on where an object is.
		*/
		static CEVec2 CalculateLayoutPosition(TransformComponent* t, UILayoutComponent* u, CEVec2 manualOffset) {
			CEVec2 finalPos = { t->position.x + manualOffset.x, t->position.y + manualOffset.y };

			// If Layout component exists, apply anchors based on CURRENT size
			if (u) {
				// --- 1. Handle Vertical Alignment & Padding ---
				if ((u->uiLayout & UILayout::TOP) != UILayout::NONE) {
					finalPos.y += t->size.y * 0.5f;
					finalPos.y -= u->paddingXY.y; // Padding pushes *down* from the top
				}
				else if ((u->uiLayout & UILayout::DOWN) != UILayout::NONE) {
					finalPos.y -= t->size.y * 0.5f;
					finalPos.y += u->paddingXY.y; // Padding pushes *up* from the bottom
				}

				// --- 2. Handle Horizontal Alignment & Padding ---
				if ((u->uiLayout & UILayout::LEFT) != UILayout::NONE) {
					finalPos.x -= t->size.x * 0.5f;
					finalPos.x += u->paddingXY.x; // Padding pushes *right* from the left
				}
				else if ((u->uiLayout & UILayout::RIGHT) != UILayout::NONE) {
					finalPos.x += t->size.x * 0.5f;
					finalPos.x -= u->paddingXY.x; // Padding pushes *left* from the right
				}
			}
			return finalPos;
		}

		/*!
		@brief       Renders a single text component and handles the blinking cursor.
		*/
		static void RenderSingleText(ECS& globalECS, Entity id) {

			// 1. Reset Shader
			Component::Rendering& renderer = Component::Rendering::Get();
			renderer.shdr_pgm->Use(); // Switch back to standard shader
			renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
			renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

			// 2. Reset Blending
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_CULL_FACE);

			// --- DEFINE UI COORDINATE SYSTEM ---
			// Use strict FBO dimensions and a strict 16:9 aspect ratio
			float winW = (float)gameTextureWidth;
			float winH = (float)gameTextureHeight;
			float aspect = winW / winH;

			float uiHeight = 100.0f;
			float uiWidth = uiHeight * aspect;

			// Lambda to convert Center-Origin Units -> Bottom-Left Origin Pixels
			auto UnitsToPixels = [&](float uX, float uY) -> CEVec2 {
				float normX = (uX + (uiWidth * 0.5f)) / uiWidth;
				float normY = (uY + (uiHeight * 0.5f)) / uiHeight;
				return { normX * winW, normY * winH };
				};

			// Ensure renderer exists
			if (!gTextRenderer) {
				gTextRenderer = std::make_unique<TextRenderer>();
			}

			TextComponent* tc = globalECS.GetComponent<TextComponent>(id);
			if (!tc) return;

			float depthZ = 0.0f;

			if (globalECS.HasComponent<TransformComponent>(id)) {
				TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);
				UILayoutComponent* u = globalECS.GetComponent<UILayoutComponent>(id);
				depthZ = t->position.z;

				// Calculate Position in "Units" (Center Origin)
				CEVec2 unitPos = CalculateLayoutPosition(t, u, tc->offset);

				float finalX = unitPos.x;
				float finalY = unitPos.y;

				// Override for Bounded Text: Use Shape Center (ignoring UILayout shift)
				if (tc->wrapText) {
					finalX = t->position.x + tc->offset.x;
					finalY = t->position.y + tc->offset.y;
				}

				float finalScale = tc->scale;
				std::string font = tc->font.empty() ? "Exo2-Regular" : tc->font;

				// --- RESOLUTION-AWARE TEXT SCALING ---
				// Shrink the absolute pixel scale of the text when the FBO shrinks.
				if (!tc->isWorldSpace) {
					// Set this to whatever height you authored your UI for in standalone!
					// (e.g., 1080.0f, 720.0f, or Settings::GetScreenHeight())
					float baseResolutionHeight = 1080.0f;
					finalScale *= (winH / baseResolutionHeight);
				}

				// --- TYPEWRITER LOGIC ---
				std::string textToRender = tc->text;
				if (tc->isRevealing && engineState == EngineState::PLAY) {
					tc->revealTimer += (float)FrameRateController::GetInstance().DT();
					tc->revealedCharCount = (int)(tc->revealTimer * tc->revealSpeed);
					if (tc->revealedCharCount < 0) tc->revealedCharCount = 0;
					if (tc->revealedCharCount > (int)tc->text.length()) tc->revealedCharCount = (int)tc->text.length();

					textToRender = tc->text.substr(0, tc->revealedCharCount);
				}
				else if (tc->isRevealing && engineState == EngineState::STOP) {
					textToRender = tc->text; // Show full text in editor so you can see it!
				}

				// --- COORDINATE SPACE CONVERSION ---
				if (!tc->isWorldSpace) {
					CEVec2 pixelPos = UnitsToPixels(finalX, finalY);
					finalX = pixelPos.x;
					finalY = pixelPos.y;
				}

				// Render Text
				if (tc->wrapText) {
					float maxWidth = t->size.x;
					float maxHeight = t->size.y;

					if (!tc->isWorldSpace) {
						maxWidth = (t->size.x / uiWidth) * winW;
						maxHeight = (t->size.y / uiHeight) * winH;
					}

					// Map UILayout to Text Alignment
					int align = 1; // CENTER
					int vAlign = 1; // MIDDLE

					if (u) {
						if (static_cast<bool>(u->uiLayout & UILayout::LEFT)) align = 0;
						else if (static_cast<bool>(u->uiLayout & UILayout::RIGHT)) align = 2;
						if (static_cast<bool>(u->uiLayout & UILayout::TOP)) vAlign = 0;
						else if (static_cast<bool>(u->uiLayout & UILayout::DOWN)) vAlign = 2;
					}
					else {
						align = (int)tc->alignment;
						vAlign = (int)tc->verticalAlignment;
					}

					float boundedX = t->position.x + tc->offset.x;
					float boundedY = t->position.y + tc->offset.y;

					if (!tc->isWorldSpace) {
						CEVec2 pixelPos = UnitsToPixels(boundedX, boundedY);
						boundedX = pixelPos.x;
						boundedY = pixelPos.y;
					}

					gTextRenderer->RenderTextBounded(
						textToRender, boundedX, boundedY, depthZ,
						maxWidth, maxHeight,
						finalScale, tc->color, font, tc->isWorldSpace,
						align, vAlign, tc->padding, tc->letterSpacing, tc->lineSpacing
					);
				}
				else if (tc->centered)
					gTextRenderer->RenderCenteredText(textToRender, finalX, finalY, depthZ, finalScale, tc->color, font, tc->isWorldSpace, tc->letterSpacing);
				else
					gTextRenderer->RenderText(textToRender, finalX, finalY, depthZ, finalScale, tc->color, font, tc->isWorldSpace, tc->letterSpacing);

				// --- CURSOR LOGIC ---
				TextInputComponent* input = globalECS.GetComponent<TextInputComponent>(id);
				if (input && input->isFocused && engineState != EngineState::STOP) {
					static float blinkTimer = 0.0f;
					blinkTimer += (float)FrameRateController::GetInstance().DT();

					if (fmod(blinkTimer, 1.0f) < 0.5f) {

						// --- 1. ALIGNMENT FIX FOR CURSOR START X ---
						float cursorStartX = finalX;

						if (tc->wrapText) {
							cursorStartX = t->position.x + tc->offset.x - (t->size.x * 0.5f) + tc->padding.x;
							if (!tc->isWorldSpace) {
								CEVec2 p = UnitsToPixels(cursorStartX, finalY);
								cursorStartX = p.x;
							}
						}
						else if (tc->centered) {
							float textWidth = gTextRenderer->GetTextWidth(tc->text, font, finalScale, tc->letterSpacing);
							cursorStartX = finalX - (textWidth * 0.5f);
						}

						// --- 2. GET EXACT CURSOR POSITION ---
						CEVec2 cursorPos = gTextRenderer->GetCursorPosition(
							tc->text, input->cursorPos, cursorStartX, finalY, finalScale, font, tc->letterSpacing);

						// --- 3. CALCULATE HEIGHT ---
						float lineH = gTextRenderer->GetLineHeight(font, finalScale);

						// Because the text uses the EXACT same vertex scaling for both Screen and World space,
						// we DO NOT need to multiply by yRatio. That is what made it gigantic!

						// The text is rendered sitting on a baseline. We want the cursor to drop 
						// slightly below the baseline (15%) and go up to the height of a capital letter (75%).
						float bottomY = cursorPos.y - (lineH * 0.75f);
						float topY = cursorPos.y + (lineH * 0.75f);

						float cursorZ = depthZ - 0.1f;
						CEColor cursorColor = { 1.f, 0.f, 0.f, 1.f }; // RED for testing

						CEVec3 startPoint = { cursorPos.x, bottomY, cursorZ };
						CEVec3 endPoint = { cursorPos.x, topY, cursorZ };

						// --- 4. IMMEDIATE DRAW (BYPASS EVERYTHING) ---
						// We draw the line RIGHT NOW. No batchers, no debug queues.

						glLineWidth(tc->isWorldSpace ? 3.0f : 2.0f);

						renderer.shdr_pgm->Use();
						// Use the exact matrices your text is currently using
						renderer.shdr_pgm->SetUniform("uView", tc->isWorldSpace ? renderer.viewMatrix : glm::mat4(1.0f));
						renderer.shdr_pgm->SetUniform("uProjection", tc->isWorldSpace ? renderer.projectionMatrix : glm::ortho(0.0f, winW, 0.0f, winH, -1.0f, 1.0f));
						renderer.shdr_pgm->SetUniform("uColor", glm::vec4(cursorColor.r, cursorColor.g, cursorColor.b, cursorColor.a));

						unsigned int VBO, VAO;
						float vertices[] = {
							startPoint.x, startPoint.y, startPoint.z,
							endPoint.x,   endPoint.y,   endPoint.z
						};

						glGenVertexArrays(1, &VAO);
						glGenBuffers(1, &VBO);
						glBindVertexArray(VAO);

						glBindBuffer(GL_ARRAY_BUFFER, VBO);
						glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

						glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
						glEnableVertexAttribArray(0);

						glDrawArrays(GL_LINES, 0, 2);

						glDeleteVertexArrays(1, &VAO);
						glDeleteBuffers(1, &VBO);

						// Restore Instanced Shader for any shapes that might draw after this
						if (renderer.shdr_pgm_instanced) {
							renderer.shdr_pgm_instanced->Use();
						}
					}
				}
			}
		}


		/*!
		@brief      Updates button states (Hover, Pressed, Normal) and visual feedback.
		@param      globalECS Reference to the ECS system.
		*/
		static void UpdateButtons(ECS& globalECS) {

			// 1. Get MOUSE in WORLD Space
			CEVec2 mouseWorld = InputManager::GetMouseWorldPos();

			// 2. Calculate MOUSE in UI Space (Center Origin, Fixed Height 100.0f)
			// [FIX] Use Game View dimensions/mouse for Editor compatibility
			float winW = (float)gameTextureWidth;
			float winH = (float)gameTextureHeight;

			float aspect = winW / winH;
			float baseHeight = 100.0f;
			float baseWidth = baseHeight * aspect;

			CEVec2 rawMouse = InputManager::GetGameMousePos(); // Top-Left Pixels (Relative to Game View)

			// Normalize Mouse to -1..1
			float ndcX = (rawMouse.x / winW) * 2.0f - 1.0f;
			float ndcY = 1.0f - (rawMouse.y / winH) * 2.0f; // Flip Y for OpenGL coords

			CEVec2 mouseUI;
			mouseUI.x = ndcX * (baseWidth * 0.5f);
			mouseUI.y = ndcY * (baseHeight * 0.5f);

			const ECS::ComponentArray<ButtonComponent>& buttonArray = globalECS.GetComponentArray<ButtonComponent>();
            bool isMouseHoveringAnyButton = false;

            for (const Entity& id : buttonArray.indexToEntity) {
                if (!IsEntityActive(globalECS, id)) continue;

                GuiComponent* g = globalECS.GetComponent<GuiComponent>(id);
                ButtonComponent* b = globalECS.GetComponent<ButtonComponent>(id);
                TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);
                if (g == nullptr || b == nullptr || t == nullptr) continue;

                UILayoutComponent* u = globalECS.GetComponent<UILayoutComponent>(id);
                ShapeComponent* s = globalECS.GetComponent<ShapeComponent>(id);
                TextComponent* txt = globalECS.GetComponent<TextComponent>(id);

                CEVec2 checkPos = mouseWorld;
                bool isUI = false;
                if (s && !s->isWorldSpace) isUI = true;
                else if (txt && !txt->isWorldSpace) isUI = true;
                if (isUI) {
                    checkPos = mouseUI;
                }

                CEVec2 finalPos = CalculateLayoutPosition(t, u, CEVec2{ 0.0f, 0.0f });
                float halfW = t->size.x * 0.5f;
                float halfH = t->size.y * 0.5f;
                CEVec2 aa{ finalPos.x - halfW, finalPos.y - halfH };
                CEVec2 bb{ finalPos.x + halfW, finalPos.y + halfH };

                if (checkPos.x >= aa.x && checkPos.x <= bb.x &&
                    checkPos.y >= aa.y && checkPos.y <= bb.y) {
                    isMouseHoveringAnyButton = true;
                    break;
                }
            }

			for (const Entity& id : buttonArray.indexToEntity) {
				if (!IsEntityActive(globalECS, id)) continue;

				// Some weird bug here, ignore button interaction if no GUI component
				GuiComponent* g = globalECS.GetComponent<GuiComponent>(id);
				if (g == nullptr) continue;

				ButtonComponent* b = globalECS.GetComponent<ButtonComponent>(id);
				TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);
				UILayoutComponent* u = globalECS.GetComponent<UILayoutComponent>(id);

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

				// --- CALCULATE HIT BOX POSITION ---
				// Use the same helper as the renderer!
				// This ensures if scale changes, the layout shifts, and the hitbox shifts with it.
				CEVec2 finalPos = CalculateLayoutPosition(t, u, CEVec2{ 0.0f, 0.0f });

				// --- COLLISION CHECK (AABB) ---
				float halfW = t->size.x * 0.5f;
				float halfH = t->size.y * 0.5f;

				// Use finalPos (Calculated) instead of t->position (Raw)
				CEVec2 aa{ finalPos.x - halfW, finalPos.y - halfH };
				CEVec2 bb{ finalPos.x + halfW, finalPos.y + halfH };

				bool isHovering = (checkPos.x >= aa.x && checkPos.x <= bb.x &&
					checkPos.y >= aa.y && checkPos.y <= bb.y);
				bool actualMouseHover = isHovering;
                if (!isHovering && !isMouseHoveringAnyButton && gForcedHoveredButton == id) {
                    isHovering = true;
                }

				// Base Scale (assumes 1.0 is default/normal scale multiplier)
				float targetScale = 1.0f;
				std::string targetTexture = "";

				// Resolve target texture logic
				// 1. Start with Normal
				targetTexture = b->textureNormal;

				if (isHovering) {
					if (actualMouseHover && InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
						b->buttonState = ButtonState::PRESSED;
						if (s) s->color = b->shapePressedColor;
						if (txt) txt->color = b->textPressedColor;
						targetScale = b->pressedScale;

						// Use Pressed texture if available, else Hovered, else Normal
						if (!b->texturePressed.empty()) targetTexture = b->texturePressed;
						else if (!b->textureHovered.empty()) targetTexture = b->textureHovered;
					}
					else {
						b->buttonState = ButtonState::HOVERED;
						if (s) s->color = b->shapeHoveredColor;
						if (txt) txt->color = b->textHoveredColor;
						targetScale = b->hoverScale;

						// Use Hovered texture if available, else Normal
						if (!b->textureHovered.empty()) targetTexture = b->textureHovered;
					}
				}
				else if (b->buttonState != ButtonState::NORMAL) {
					b->buttonState = ButtonState::NORMAL;
					if (s) s->color = b->shapeNormalColor;
					if (txt) txt->color = b->textNormalColor;
					targetScale = 1.0f;
					// Revert to Normal texture
					targetTexture = b->textureNormal;
				}

				// Apply Texture Swap if target is valid and different
				if (s && !targetTexture.empty() && s->textureName != targetTexture) {
					s->textureName = targetTexture;
					s->enableTexture = true; // Ensure texture rendering is on
				}

				if (b->originalSize.x == 0.0f && b->originalSize.y == 0.0f) {
					b->originalSize = t->size;
				}

				// Apply target scale to Shape size
				t->size.x = b->originalSize.x * targetScale;
				t->size.y = b->originalSize.y * targetScale;

				// Apply target scale to Text scale
				if (txt) {
					// Capture original text scale if not yet set
					if (b->originalTextScale == 0.0f) {
						b->originalTextScale = txt->scale;
					}
					// Only apply if we have a valid original scale (avoid shrinking to 0 if init failed)
					if (b->originalTextScale > 0.0f) {
						txt->scale = b->originalTextScale * targetScale;
					}
				}
			}
		}

		/*!
		@brief      Updates text input focus logic and handles keyboard events.
		@param      globalECS Reference to the ECS system.
		*/
		static void UpdateInputs(ECS& globalECS) {
			static Entity selectedTextInput = (Entity)-1;
			const ECS::ComponentArray<TextInputComponent>& textInputArray = globalECS.GetComponentArray<TextInputComponent>();

			// 1. Calculate Mouse Positions (Same logic as UpdateButtons)
			CEVec2 mouseWorld = InputManager::GetMouseWorldPos();

			CEVec2 mouseUI;

			// [FIX] Use Game View dimensions/mouse for Editor compatibility
			float winW = (float)gameTextureWidth;
			float winH = (float)gameTextureHeight;

			CEVec2 rawMouse = InputManager::GetGameMousePos(); // Top-Left Pixels (Relative to Game View)

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
				if (!IsEntityActive(globalECS, id)) continue;

				TransformComponent* t = globalECS.GetComponent<TransformComponent>(id);
				UILayoutComponent* u = globalECS.GetComponent<UILayoutComponent>(id);
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

				CEVec2 finalPos = CalculateLayoutPosition(t, u, CEVec2{ 0.0f, 0.0f });

				// AABB Check
				float halfW = t->size.x * 0.5f;
				float halfH = t->size.y * 0.5f;
				CEVec2 aa{ finalPos.x - halfW, finalPos.y - halfH };
				CEVec2 bb{ finalPos.x + halfW, finalPos.y + halfH };

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
					TextInputComponent* input = globalECS.GetComponent<TextInputComponent>(newSelection);
					TextComponent* txt = globalECS.GetComponent<TextComponent>(newSelection);

					input->isFocused = true;

					// When clicked, immediately put the cursor at the end of the text!
					if (txt) {
						input->cursorPos = txt->text.length();
					}
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
	}

	// ============================================================================
	// PARTICLE HELPERS 
	// ============================================================================
	namespace Particles {

		/*!
		@brief      Init for the new ParticleComponent.
		*/
		static void InitAllParticles(ECS& globalECS) {
			// Iterate over the single ParticleComponent type
			auto& particleArray = globalECS.GetComponentArray<ParticleComponent>();
			for (const Entity& id : particleArray.indexToEntity) {
				if (!IsEntityActive(globalECS, id)) continue;

				ParticleComponent* pc = globalECS.GetComponent<ParticleComponent>(id);
				if (pc) {
					ParticleSystem::ParticleInit(*pc);
				}
			}
		}

		/*!
		@brief      Update loop for the new ParticleComponent.
		*/
		static void UpdateAllParticles(ECS& globalECS) {
			float dt = (float)FrameRateController::GetInstance().DT();

			// Iterate over the single ParticleComponent type
			auto& particleArray = globalECS.GetComponentArray<ParticleComponent>();
			CrashoutsEngine::JobSystem::Get().ParallelFor(
				static_cast<unsigned int>(particleArray.indexToEntity.size()),
				1u,
				[&](unsigned int start, unsigned int end) {
					for (unsigned int i = start; i < end; ++i) {
						const Entity& id = particleArray.indexToEntity[i];
						if (!IsEntityActive(globalECS, id)) continue;

						ParticleComponent* pc = globalECS.GetComponent<ParticleComponent>(id);
						if (!pc) continue;

						if (engineState == EngineState::PLAY || pc->playOnStop) {
							ParticleSystem::ParticleUpdate(id, *pc, dt);
						}
						else {
							pc->data.activeCount = 0;
						}
					}
				});
		}

		/*!
		@brief      Renders a single particle system, applying the computed layer depth.
		*/
		static void RenderParticleSystem(ECS& globalECS, Entity id) {
			// Temp buffers to split CEVec2 scale into Width/Height arrays for the renderer
			static std::vector<float> s_scaleXBuffer;
			static std::vector<float> s_scaleYBuffer;
			static std::vector<float> s_uniformScaleBuffer; // Always 1.0f
			static std::vector<float> s_radiusBuffer;

			// Temp buffers for transforms
			static std::vector<CEVec3> s_posBuffer;
			static std::vector<CEVec2> s_uvOffsetBuffer;

			ParticleComponent* pc = globalECS.GetComponent<ParticleComponent>(id);
			TransformComponent* tc = globalECS.GetComponent<TransformComponent>(id);

			if (!pc || pc->data.activeCount == 0) return;

			size_t count = pc->data.activeCount;

			if (pc->gpuSimulation && pc->preset != ParticleSystem::ParticlePreset::SuperheroLanding) {
				if (s_scaleXBuffer.size() < count) s_scaleXBuffer.resize(count);
				if (s_scaleYBuffer.size() < count) s_scaleYBuffer.resize(count);
				if (s_uvOffsetBuffer.size() < count) s_uvOffsetBuffer.resize(count, { 0.f, 0.f });

				float worldScaleX = 1.0f;
				float worldScaleY = 1.0f;
				if (pc->useWorldSpace && tc) {
					worldScaleX = tc->size.x;
					worldScaleY = tc->size.y;
				}

				for (size_t k = 0; k < count; ++k) {
					s_scaleXBuffer[k] = pc->data.scales[k].x * worldScaleX;
					s_scaleYBuffer[k] = pc->data.scales[k].y * worldScaleY;
					s_uvOffsetBuffer[k] = { pc->data.velocities[k].x, pc->data.velocities[k].y };
				}

				InstancingSystem::InstancedParams params;
				params.textured = pc->useTexture;
				params.textureName = pc->textureName;
				params.uvSize = pc->textureUV;
				params.tint = { 1.0f, 1.0f, 1.0f, 1.0f };
				params.additiveBlend = pc->additiveBlend;

				InstancingSystem::ParticleShaderParams p;
				p.time = pc->runtimeTime;
				p.gravity = pc->gravity;
				p.startColor = pc->startcolor;
				p.endColor = pc->endColor;
				p.sizeOverTime = pc->sizeOverTime;
				p.rotationOverTime = pc->rotationOverTime;
				p.flowStrength = pc->flowStrength;
				p.flowSpeed = pc->flowSpeed;
				p.flowFrequency = pc->flowFrequency;
				p.renderShape = static_cast<int>(pc->renderShape);
				p.squareSizeMultiplier = pc->squareSizeMultiplier;
				p.circleRadiusMultiplier = pc->circleRadiusMultiplier;
				p.triangleWidthMultiplier = pc->triangleWidthMultiplier;
				p.triangleHeightMultiplier = pc->triangleHeightMultiplier;

				bool isLocal = (!pc->useWorldSpace && tc);
				p.useLocalSpace = isLocal ? 1 : 0;
				if (isLocal) {
					p.parentPos = tc->position;
					p.parentRotDeg = tc->rotation;
					p.parentScale = tc->size;
				}

				if (pc->renderShape == ParticleSystem::ParticleRenderShape::Circle) {
					int segments = pc->circleSegments;
					if (segments < 3) segments = 3;
					if (segments > 128) segments = 128;
					InstancingSystem::RenderParticleCirclesBatch(
						pc->data.positions.data(),
						pc->data.spawnTimes.data(),
						pc->data.maxLives.data(),
						pc->data.rotations.data(),
						s_scaleXBuffer.data(),
						s_scaleYBuffer.data(),
						s_uvOffsetBuffer.data(),
						static_cast<int>(count),
						segments,
						params,
						p
					);
				}
				else if (pc->renderShape == ParticleSystem::ParticleRenderShape::Triangle) {
					InstancingSystem::RenderParticleTrianglesBatch(
						pc->data.positions.data(),
						pc->data.spawnTimes.data(),
						pc->data.maxLives.data(),
						pc->data.rotations.data(),
						s_scaleXBuffer.data(),
						s_scaleYBuffer.data(),
						s_uvOffsetBuffer.data(),
						static_cast<int>(count),
						params,
						p
					);
				}
				else {
					InstancingSystem::RenderParticleRectanglesBatch(
						pc->data.positions.data(),
						pc->data.spawnTimes.data(),
						pc->data.maxLives.data(),
						pc->data.rotations.data(),
						s_scaleXBuffer.data(),
						s_scaleYBuffer.data(),
						s_uvOffsetBuffer.data(),
						static_cast<int>(count),
						params,
						p
					);
				}

				return;
			}

			// 1. Resize Temp Buffers
			if (s_scaleXBuffer.size() < count) s_scaleXBuffer.resize(count);
			if (s_scaleYBuffer.size() < count) s_scaleYBuffer.resize(count);
			if (s_uniformScaleBuffer.size() < count) s_uniformScaleBuffer.resize(count, 1.0f);
			if (s_radiusBuffer.size() < count) s_radiusBuffer.resize(count);
			if (s_uvOffsetBuffer.size() < count) s_uvOffsetBuffer.resize(count, { 0.f, 0.f });
			if (s_posBuffer.size() < count) s_posBuffer.resize(count);

			// Ensure uniform buffer is actually 1.0 (in case of resize dirty data)
			// We map 1.0 here so we can pass the actual dimensions into Dim1/Dim2
			std::fill(s_uniformScaleBuffer.begin(), s_uniformScaleBuffer.begin() + count, 1.0f);

			// 2. Handle Local/World Space & Split Scales

			bool isLocal = (!pc->useWorldSpace && tc);
			CEVec3 parentPos = { 0,0,0 };
			float c = 1.f, s = 0.f;
			float parentScaleX = 1.0f;
			float parentScaleY = 1.0f;

			if (isLocal) {
				parentPos = tc->position;
				// If Local, particles should inherit Parent's size too
				parentScaleX = tc->size.x;
				parentScaleY = tc->size.y;

				float rad = tc->rotation * (CEMath::PI / 180.0f);
				c = cosf(rad);
				s = sinf(rad);
			}

			// Process every particle to prepare render buffers
			for (size_t k = 0; k < count; ++k) {
				float baseW = pc->data.scales[k].x * parentScaleX;
				float baseH = pc->data.scales[k].y * parentScaleY;

				if (pc->renderShape == ParticleSystem::ParticleRenderShape::Square) {
					s_scaleXBuffer[k] = baseW * pc->squareSizeMultiplier;
					s_scaleYBuffer[k] = baseH * pc->squareSizeMultiplier;
				}
				else if (pc->renderShape == ParticleSystem::ParticleRenderShape::Triangle) {
					s_scaleXBuffer[k] = baseW * pc->triangleWidthMultiplier;
					s_scaleYBuffer[k] = baseH * pc->triangleHeightMultiplier;
				}
				else {
					s_scaleXBuffer[k] = baseW;
					s_scaleYBuffer[k] = baseH;
					float minDim = (baseW < baseH) ? baseW : baseH;
					s_radiusBuffer[k] = 0.5f * minDim * pc->circleRadiusMultiplier;
				}

				// Apply positions and ENFORCE the computed depth from the layer system
				if (isLocal) {
					CEVec3 local = pc->data.positions[k];
					float rotX = local.x * c - local.y * s;
					float rotY = local.x * s + local.y * c;
					s_posBuffer[k].x = rotX + parentPos.x;
					s_posBuffer[k].y = rotY + parentPos.y;
					s_posBuffer[k].z = local.z + parentPos.z;
				}
				else {
					s_posBuffer[k].x = pc->data.positions[k].x;
					s_posBuffer[k].y = pc->data.positions[k].y;
					s_posBuffer[k].z = pc->data.positions[k].z;
				}
			}

			// 3. Submit to Renderer
			InstancingSystem::InstancedParams params;
			params.textured = pc->useTexture;
			params.textureName = pc->textureName;
			params.uvSize = pc->textureUV;
			params.tint = { 1.0f, 1.0f, 1.0f };

			if (pc->renderShape == ParticleSystem::ParticleRenderShape::Circle) {
				int segments = pc->circleSegments;
				if (segments < 3) segments = 3;
				if (segments > 128) segments = 128;

				InstancingSystem::RenderInstancedCirclesBatch(
					s_posBuffer.data(),
					pc->data.colors.data(),
					pc->data.rotations.data(),
					s_uniformScaleBuffer.data(),
					s_radiusBuffer.data(),
					s_uvOffsetBuffer.data(),
					(int)count,
					segments,
					params
				);
			}
			else if (pc->renderShape == ParticleSystem::ParticleRenderShape::Triangle) {
				InstancingSystem::RenderInstancedTrianglesBatch(
					s_posBuffer.data(),
					pc->data.colors.data(),
					pc->data.rotations.data(),
					s_uniformScaleBuffer.data(),
					s_scaleXBuffer.data(),
					s_scaleYBuffer.data(),
					s_uvOffsetBuffer.data(),
					(int)count,
					params
				);
			}
			else {
				InstancingSystem::RenderInstancedRectanglesBatch(
					s_posBuffer.data(),		     // Correct World Position
					pc->data.colors.data(),      // Colors
					pc->data.rotations.data(),   // Rotations
					s_uniformScaleBuffer.data(), // Uniform Scale (1.0)
					s_scaleXBuffer.data(),       // Widths (X) 
					s_scaleYBuffer.data(),       // Heights (Y)
					s_uvOffsetBuffer.data(),     // UV Offsets (0,0)
					(int)count,
					params
				);
			}

		}
	}

	// ============================================================================
	// SCENE RENDERING
	// ============================================================================

	static TextureMode GetWrapMode(int mode) {
		switch (mode) {
		case 0: return TextureMode::ClampToEdge;
		case 1: return TextureMode::Repeat;
		case 2: return TextureMode::Mirror;
		default: return TextureMode::ClampToEdge;
		}
	}

	// Data Holder
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
		GLuint textureHandle = 0; // Added handle
		bool enableTexture = false;
		TextureMode wrapU = TextureMode::ClampToEdge;
		TextureMode wrapV = TextureMode::ClampToEdge;

		void Clear() {
			positions.clear(); colors.clear(); rotations.clear();
			scales.clear(); dim1.clear(); dim2.clear(); uvOffsets.clear();
			textureHandle = 0;
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

	// Linear Batcher Class
	// Manages the batching states
	struct LinearBatcher {
		RenderBatch batch;
		ShapeType currentShapeType = ShapeType::Rectangle;

		// Cleans the render batch data
		void Flush() {
			if (batch.positions.empty()) return;

			GLuint currentHandle = 0;
			bool useTextureForThisBatch = batch.enableTexture;

			// We use a separate string for the instancer to prevent caching a broken path
			std::string finalTexNameForInstancer = "";

			// --> Check for raw runtime handles first (Videos, Framebuffers) <--
			if (batch.textureHandle != 0) {
				currentHandle = batch.textureHandle;
				finalTexNameForInstancer = batch.textureName; // Pass the dummy name
			}
			// Otherwise, do the standard AssetManager lookup
			else if (!batch.textureName.empty()) {
				std::string lookupName = batch.textureName;
				std::filesystem::path p(lookupName);
				if (p.has_stem()) lookupName = p.stem().string();

				auto* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(lookupName);

				if (tex) {
					tex->SetTextureMode(batch.wrapU, batch.wrapV);
					currentHandle = tex->GetGLHandle();

					if (currentHandle == 0 || !tex->IsUploaded()) {
						useTextureForThisBatch = false;
					}
					else {
						finalTexNameForInstancer = lookupName;
					}
				}
				else {
					useTextureForThisBatch = false;
				}
			}

			InstancingSystem::InstancedParams params;
			params.textured = useTextureForThisBatch;

			// 3. PASS THE CORRECTED DATA TO THE SHADER
			params.textureName = finalTexNameForInstancer;
			params.textureHandle = currentHandle;

			params.tint = { 1, 1, 1 };
			params.uvSize = batch.uvSize;

			int count = (int)batch.positions.size();

			if (currentShapeType == ShapeType::Rectangle) {
				InstancingSystem::RenderInstancedRectanglesBatch(
					batch.positions.data(), batch.colors.data(), batch.rotations.data(),
					batch.scales.data(), batch.dim1.data(), batch.dim2.data(),
					batch.uvOffsets.data(), count, params);
			}
			else if (currentShapeType == ShapeType::Triangle) {
				InstancingSystem::RenderInstancedTrianglesBatch(
					batch.positions.data(), batch.colors.data(), batch.rotations.data(),
					batch.scales.data(), batch.dim1.data(), batch.dim2.data(),
					batch.uvOffsets.data(), count, params);
			}
			else if (currentShapeType == ShapeType::Circle) {
				InstancingSystem::RenderInstancedCirclesBatch(
					batch.positions.data(), batch.colors.data(), batch.rotations.data(),
					batch.scales.data(), batch.dim1.data(),
					batch.uvOffsets.data(), count, 32, params);
			}

			batch.Clear();
		}

		// Send new batching data
		void Submit(TransformComponent* tc, ShapeComponent* sc) {
			if (sc->type == ShapeType::Line) {
				Flush();
				Component::Shapes::CreateLine(sc->p1 + tc->position, sc->p2 + tc->position, sc->thickness, sc->color, sc->isWorldSpace);
				return;
			}

			bool typeChanged = (sc->type != currentShapeType);
			bool textureChanged = false;

			if (!batch.positions.empty()) {
				if (sc->enableTexture != batch.enableTexture) textureChanged = true;
				else if (sc->enableTexture) {
					if (sc->textureHandle != 0 && sc->textureHandle != batch.textureHandle) textureChanged = true;
					// ONLY compare the name! The handle is evaluated at Flush() time.
					if (sc->textureName != batch.textureName) textureChanged = true;
					else if (sc->uvSize != batch.uvSize) textureChanged = true;
					else if (GetWrapMode(sc->wrapModeU) != batch.wrapU) textureChanged = true;
				}
			}

			if (textureChanged || typeChanged) {
				Flush();
			}

			if (batch.positions.empty()) {
				currentShapeType = sc->type;
				batch.textureName = sc->textureName;
				batch.textureHandle = sc->textureHandle;
				batch.enableTexture = sc->enableTexture;
				batch.uvSize = sc->uvSize;
				batch.wrapU = GetWrapMode(sc->wrapModeU);
				batch.wrapV = GetWrapMode(sc->wrapModeV);
			}

			CEColor colorNorm(sc->color.x(), sc->color.y(), sc->color.z(), sc->color.w());
			float dim1 = tc->size.x;
			float dim2 = tc->size.y;
			if (sc->type == ShapeType::Circle) { dim1 = tc->size.x * 0.5f; dim2 = 0.0f; }

			batch.Add(tc->position, colorNorm, tc->rotation, 1.0f, dim1, dim2, sc->uvOffset);
		}
	
		// Submit data just for debug lines
		void SubmitLineAsBatchedRect(const CollisionGizmos::DebugLineCmd& line) {
			// 1. Ensure we are batching Rectangles and textures are off for debug lines
			if (currentShapeType != ShapeType::Rectangle || batch.enableTexture) {
				Flush();
				currentShapeType = ShapeType::Rectangle;
				batch.enableTexture = false;
				batch.textureName = "";
				batch.textureHandle = 0;
			}

			// 2. The Math Hack: Turn the line into a centered rectangle
			CEVec3 diff = line.p2 - line.p1;
			float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);

			// Ignore zero-length lines
			if (length <= 0.0001f) return;

			// Calculate angle in Degrees (since your engine seems to use degrees for transforms)
			float angleDeg = std::atan2(diff.y, diff.x) * (180.0f / CEMath::PI);

			// Center point of the line
			CEVec3 center = line.p1 + (diff * 0.5f);
			center.z = line.p1.z; // Maintain your debug Z-depth

			// 3. Add to the standard rectangle batch!
			// pos, color, rotation, scale, dim1(width), dim2(height), uvOffset
			batch.Add(center, line.color, angleDeg, 1.0f, length, line.thickness, { 0.0f, 0.0f });
		}
	};

	void InitScene(CrashoutsEngine::ECS& globalECS) {
		Particles::InitAllParticles(globalECS);
		UI::InitText(globalECS);
	}

	/*!
	@brief   Render scene shapes with z-depth sorting (world and screen space).
	@param   globalECS Reference to ECS registry.
	*/
	void RenderScene(CrashoutsEngine::ECS& globalECS) {
		FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "RenderScene");
		using namespace CrashoutsEngine;
		using Component::Rendering;
		Rendering& renderer = Rendering::Get();
		if (!renderer.shdr_pgm_instanced) return;

		if (!gTextRenderer) {
			gTextRenderer = std::make_unique<TextRenderer>();
		}

		const auto& shapeArray = globalECS.GetComponentArray<ShapeComponent>();
		const auto& textArray = globalECS.GetComponentArray<TextComponent>();
		const auto& particleArray = globalECS.GetComponentArray<ParticleComponent>();

		// 1. Setup Camera & Culling Bounds
		// ---------------------------------------------------------
		RenderCamera::Camera& cam = (engineState == EngineState::STOP) ?
			RenderCamera::engineCamera : RenderCamera::gameCamera;

		CEVec3 camPos = cam.GetPosition();
		float camHeight = cam.GetVisibleWorldHeight();
		float camWidth = cam.GetVisibleWorldWidth();

		// Add a margin (0.65f factor from your old code, or just add buffer)
		// This prevents objects popping out at the very edge of the screen.
		float cullMinX = camPos.x - (camWidth * 0.6f);
		float cullMaxX = camPos.x + (camWidth * 0.6f);
		float cullMinY = camPos.y - (camHeight * 0.6f);
		float cullMaxY = camPos.y + (camHeight * 0.6f);
		// ---------------------------------------------------------

		enum class RenderType { Shape, Text, Particle };

		/** @brief RenderEntry */
		struct RenderEntry {
			Entity entity;
			float depth;
			RenderType type;
		};

		static std::vector<RenderEntry> worldEntries;
		static std::vector<RenderEntry> screenEntries;
		worldEntries.clear();
		screenEntries.clear();

		size_t reserveSize = shapeArray.indexToEntity.size() + textArray.indexToEntity.size() + particleArray.indexToEntity.size();
		if (worldEntries.capacity() < reserveSize) worldEntries.reserve(reserveSize);
		if (screenEntries.capacity() < reserveSize) screenEntries.reserve(reserveSize);
		std::mutex renderEntriesMutex;

		auto mergeEntries = [&](std::vector<RenderEntry>& localWorld, std::vector<RenderEntry>& localScreen) {
			if (localWorld.empty() && localScreen.empty()) return;
			std::lock_guard<std::mutex> lock(renderEntriesMutex);
			if (!localWorld.empty()) {
				worldEntries.insert(worldEntries.end(), localWorld.begin(), localWorld.end());
			}
			if (!localScreen.empty()) {
				screenEntries.insert(screenEntries.end(), localScreen.begin(), localScreen.end());
			}
		};

		// 2a. PRE-SORT SHAPES
		CrashoutsEngine::JobSystem::Get().ParallelFor(static_cast<unsigned int>(shapeArray.indexToEntity.size()), 32u,
			[&](unsigned int start, unsigned int end) {
				std::vector<RenderEntry> localWorld;
				std::vector<RenderEntry> localScreen;
				localWorld.reserve(end - start);
				localScreen.reserve(end - start);

				for (unsigned int i = start; i < end; ++i) {
					const Entity& id = shapeArray.indexToEntity[i];
					if (!IsEntityActive(globalECS, id)) continue;
					auto* tc = globalECS.GetComponent<TransformComponent>(id);
					if (!tc) continue;
					auto* sc = globalECS.GetComponent<ShapeComponent>(id);
					if (!sc || sc->preview) continue;

					float depth = 0.0f;
					if (globalECS.HasComponent<Component::LayerComponent>(id)) {
						auto* z = globalECS.GetComponent<Component::LayerComponent>(id);
						if (Component::LayerComponent::layerVisibility.count(z->layerName) &&
							!Component::LayerComponent::layerVisibility[z->layerName]) {
							continue;
						}
						DebugZOverride* dbg = globalECS.HasComponent<DebugZOverride>(id) ? globalECS.GetComponent<DebugZOverride>(id) : nullptr;
						depth = Component::Rendering::ComputeDepth(tc->position, *z, dbg);
					}
					else {
						Component::LayerComponent defaultZ{ "Default", 0 };
						if (Component::LayerComponent::layerVisibility.count("Default") &&
							!Component::LayerComponent::layerVisibility["Default"]) {
							continue;
						}
						depth = Component::Rendering::ComputeDepth(tc->position, defaultZ, nullptr);
					}

					if (sc->isWorldSpace) localWorld.push_back({ id, depth, RenderType::Shape });
					else                  localScreen.push_back({ id, depth, RenderType::Shape });
				}

				mergeEntries(localWorld, localScreen);
			});

		// 2b. PRE-SORT TEXT
		CrashoutsEngine::JobSystem::Get().ParallelFor(static_cast<unsigned int>(textArray.indexToEntity.size()), 32u,
			[&](unsigned int start, unsigned int end) {
				std::vector<RenderEntry> localWorld;
				std::vector<RenderEntry> localScreen;
				localWorld.reserve(end - start);
				localScreen.reserve(end - start);

				for (unsigned int i = start; i < end; ++i) {
					const Entity& id = textArray.indexToEntity[i];
					if (!IsEntityActive(globalECS, id)) continue;
					auto* tc = globalECS.GetComponent<TransformComponent>(id);
					auto* txt = globalECS.GetComponent<TextComponent>(id);
					if (!tc || !txt) continue;

					float depth = 0.0f;
					if (globalECS.HasComponent<Component::LayerComponent>(id)) {
						auto* z = globalECS.GetComponent<Component::LayerComponent>(id);
						if (Component::LayerComponent::layerVisibility.count(z->layerName) &&
							!Component::LayerComponent::layerVisibility[z->layerName]) {
							continue;
						}
						DebugZOverride* dbg = globalECS.HasComponent<DebugZOverride>(id) ? globalECS.GetComponent<DebugZOverride>(id) : nullptr;
						depth = Component::Rendering::ComputeDepth(tc->position, *z, dbg);
					}
					else {
						Component::LayerComponent defaultZ{ "Default", 0 };
						if (Component::LayerComponent::layerVisibility.count("Default") &&
							!Component::LayerComponent::layerVisibility["Default"]) {
							continue;
						}
						depth = Component::Rendering::ComputeDepth(tc->position, defaultZ, nullptr);
					}

					if (txt->isWorldSpace) localWorld.push_back({ id, depth, RenderType::Text });
					else                   localScreen.push_back({ id, depth, RenderType::Text });
				}

				mergeEntries(localWorld, localScreen);
			});

		// 2c. PRE-SORT PARTICLES
		CrashoutsEngine::JobSystem::Get().ParallelFor(static_cast<unsigned int>(particleArray.indexToEntity.size()), 16u,
			[&](unsigned int start, unsigned int end) {
				std::vector<RenderEntry> localWorld;
				std::vector<RenderEntry> localScreen;
				localWorld.reserve(end - start);
				localScreen.reserve(end - start);

				for (unsigned int i = start; i < end; ++i) {
					const Entity& id = particleArray.indexToEntity[i];
					if (!IsEntityActive(globalECS, id)) continue;
					auto* pc = globalECS.GetComponent<ParticleComponent>(id);
					if (!pc || pc->data.activeCount == 0) continue;

					auto* tc = globalECS.GetComponent<TransformComponent>(id);
					CEVec3 basePos = tc ? tc->position : CEVec3(0, 0, 0);

					float depth = 0.0f;
					if (globalECS.HasComponent<Component::LayerComponent>(id)) {
						auto* z = globalECS.GetComponent<Component::LayerComponent>(id);
						if (Component::LayerComponent::layerVisibility.count(z->layerName) &&
							!Component::LayerComponent::layerVisibility[z->layerName]) {
							continue;
						}
						DebugZOverride* dbg = globalECS.HasComponent<DebugZOverride>(id) ? globalECS.GetComponent<DebugZOverride>(id) : nullptr;
						depth = Component::Rendering::ComputeDepth(basePos, *z, dbg);
					}
					else {
						Component::LayerComponent defaultZ{ "Default", 0 };
						if (Component::LayerComponent::layerVisibility.count("Default") &&
							!Component::LayerComponent::layerVisibility["Default"]) {
							continue;
						}
						depth = Component::Rendering::ComputeDepth(basePos, defaultZ, nullptr);
					}

					if (pc->useWorldSpace) localWorld.push_back({ id, depth, RenderType::Particle });
					else                   localScreen.push_back({ id, depth, RenderType::Particle });
				}

				mergeEntries(localWorld, localScreen);
			});

		// 3. SORT
		auto sortFunc = [](const RenderEntry& a, const RenderEntry& b) {
			if (a.depth != b.depth) return a.depth < b.depth;
			// If depth is equal, draw Shape & Particle BEFORE Text (so text is on top)
			return (a.type == RenderType::Shape || a.type == RenderType::Particle) && b.type == RenderType::Text;
			};
		std::sort(worldEntries.begin(), worldEntries.end(), sortFunc);
		std::sort(screenEntries.begin(), screenEntries.end(), sortFunc);

		// --- LINEAR BATCHER ---
		static LinearBatcher batcher;
		batcher.batch.Clear();

		// Updated Processor
		auto ProcessSortedList = [&](const std::vector<RenderEntry>& entries, bool useCulling, const glm::mat4& viewMat, const glm::mat4& projMat) {
			// Ensure text renderer is ready if needed
			if (!gTextRenderer) gTextRenderer = std::make_unique<TextRenderer>();

			for (const auto& item : entries) {
				auto* tc = globalECS.GetComponent<TransformComponent>(item.entity);

				// Cull check (only for Shapes currently, as Text bounds are complex)
				// We can skip culling for text or implement it later if needed.
				if (item.type == RenderType::Shape && useCulling) {
					float halfW = tc->size.x * 0.5f;
					float halfH = tc->size.y * 0.5f;
					float objMaxX = tc->position.x + halfW;
					float objMinX = tc->position.x - halfW;
					float objMaxY = tc->position.y + halfH;
					float objMinY = tc->position.y - halfH;

					if (objMaxX < cullMinX || objMinX > cullMaxX ||
						objMaxY < cullMinY || objMinY > cullMaxY)
					{
						continue;
					}
				}

				if (item.type == RenderType::Shape) {
					auto* sc = globalECS.GetComponent<ShapeComponent>(item.entity);
					batcher.Submit(tc, sc);
				}
				else if (item.type == RenderType::Text) {
					// 1. Flush any pending shapes so they draw BEHIND this text
					batcher.Flush();

					// 2. Draw the text immediately using the new single-text function
					Scene::UI::RenderSingleText(globalECS, item.entity);

					// 3. Restore Instanced Shader & Uniforms for next shapes
					renderer.shdr_pgm_instanced->Use();
					renderer.shdr_pgm_instanced->SetUniform("uView", viewMat);
					renderer.shdr_pgm_instanced->SetUniform("uProjection", projMat);
				}
				else if (item.type == RenderType::Particle) {
					// Flush pending shapes so they draw BEHIND particles
					batcher.Flush();

					// Draw the particle system
					Particles::RenderParticleSystem(globalECS, item.entity);

					// Restore shader state since particles use instanced rendering
					renderer.shdr_pgm_instanced->Use();
					renderer.shdr_pgm_instanced->SetUniform("uView", viewMat);
					renderer.shdr_pgm_instanced->SetUniform("uProjection", projMat);
				}
			}
			batcher.Flush();
			};

		// 4. DRAW WORLD SPACE (Culling ENABLED)
		// Set uniforms for both shaders to be safe
		renderer.shdr_pgm->Use();
		renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
		renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

		if (renderer.shdr_pgm_instanced && renderer.shdr_pgm_instanced != renderer.shdr_pgm) {
			renderer.shdr_pgm_instanced->Use();
			renderer.shdr_pgm_instanced->SetUniform("uView", renderer.viewMatrix);
			renderer.shdr_pgm_instanced->SetUniform("uProjection", renderer.projectionMatrix);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Pass 'true' to enable culling for world objects
		ProcessSortedList(worldEntries, true, renderer.viewMatrix, renderer.projectionMatrix);

		// 5. DRAW SCREEN SPACE (Culling DISABLED)
		// FIX: Force strictly 16:9 aspect ratio so shapes don't stretch in the FBO
		float aspect = (float)gameTextureWidth / (float)gameTextureHeight; 
		float h = 100.0f;
		float w = h * aspect;
		glm::mat4 uiProj = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
		glm::mat4 uiView = glm::mat4(1.0f);

		renderer.shdr_pgm->Use();
		renderer.shdr_pgm->SetUniform("uView", uiView);
		renderer.shdr_pgm->SetUniform("uProjection", uiProj);

		if (renderer.shdr_pgm_instanced && renderer.shdr_pgm_instanced != renderer.shdr_pgm) {
			renderer.shdr_pgm_instanced->Use();
			renderer.shdr_pgm_instanced->SetUniform("uView", uiView);
			renderer.shdr_pgm_instanced->SetUniform("uProjection", uiProj);
		}

		// Pass 'false' to disable culling for UI
		ProcessSortedList(screenEntries, false, uiView, uiProj);

		// Restore State
		if (glIsEnabled(GL_DEPTH_TEST)) glEnable(GL_DEPTH_TEST);
		if (glIsEnabled(GL_CULL_FACE)) glEnable(GL_CULL_FACE);
		renderer.shdr_pgm_instanced->Use();

		if (!CollisionGizmos::g_DebugLineQueue.empty()) {
			// Set world camera matrices for debug lines
			renderer.shdr_pgm_instanced->SetUniform("uView", renderer.viewMatrix);
			renderer.shdr_pgm_instanced->SetUniform("uProjection", renderer.projectionMatrix);

			// Disable depth test if you want colliders drawn over everything
			glDisable(GL_DEPTH_TEST);

			for (const auto& cmd : CollisionGizmos::g_DebugLineQueue) {
				batcher.SubmitLineAsBatchedRect(cmd);
			}
			batcher.Flush(); // Force draw the debug batch

			CollisionGizmos::g_DebugLineQueue.clear(); // Clear for next frame

			if (glIsEnabled(GL_DEPTH_TEST) == GL_FALSE) glEnable(GL_DEPTH_TEST); // Restore
		}

		Component::RenderSpriteAnimationsECS();

		if (g_ShowFPS) {
			batcher.Flush();

			// FIX: Use the actual FBO dimensions so the text cannot render out of bounds!
			int fpsWindowW = gameTextureWidth;
			int fpsWindowH = gameTextureHeight;

			// FORCE FRONTMOST STATE:
			glViewport(0, 0, fpsWindowW, fpsWindowH);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glClear(GL_DEPTH_BUFFER_BIT);

			if (!gTextRenderer) gTextRenderer = std::make_unique<TextRenderer>();

			double fps = FrameRateController::GetInstance().FPS();
			std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));

			renderer.shdr_pgm->Use();

			// Use the true FBO dimensions for the projection
			glm::mat4 overlayProj = glm::ortho(0.0f, (float)fpsWindowW, 0.0f, (float)fpsWindowH);
			renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
			renderer.shdr_pgm->SetUniform("uProjection", overlayProj);

			float zFront = 0.0f;
			float xPos = 20.0f;
			// Now this reliably calculates from the top of the letterbox!
			float yPos = (float)fpsWindowH - 40.0f;

			gTextRenderer->RenderText(fpsText, xPos, yPos, zFront, 15.0f,
				CEColor(0.0f, 1.0f, 0.0f, 1.0f),
				"Exo2-Regular", false, 0.0f);

			// RESTORE VIEWPORT
			glViewport(0, 0, gameTextureWidth, gameTextureHeight);
		}

		if (InputManager::HasGlobalNotification()) {
			batcher.Flush();

			int overlayWindowW = gameTextureWidth;
			int overlayWindowH = gameTextureHeight;

			glViewport(0, 0, overlayWindowW, overlayWindowH);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glClear(GL_DEPTH_BUFFER_BIT);

			if (!gTextRenderer) gTextRenderer = std::make_unique<TextRenderer>();

			renderer.shdr_pgm->Use();
			glm::mat4 overlayProj = glm::ortho(0.0f, (float)overlayWindowW, 0.0f, (float)overlayWindowH);

			renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
			renderer.shdr_pgm->SetUniform("uProjection", overlayProj);

			float zFront = 0.0f;
			float xPos = 40.0f;
			float yPos = (float)overlayWindowH * 0.5f;

			gTextRenderer->RenderText(InputManager::GetGlobalNotificationText(), xPos, yPos, zFront, 12.0f,
				CEColor(1.0f, 0.95f, 0.65f, 1.0f),
				"Bellefair-Regular", false, 0.0f);

			glViewport(0, 0, gameTextureWidth, gameTextureHeight);
		}
	}

	void UpdateScene(CrashoutsEngine::ECS& globalECS) {
		using namespace CrashoutsEngine;

#if !USE_IMGUI_EDITOR && !defined(DEBUG)
		if (InputManager::IsKeyTriggered(Key::F3)) {
			g_ShowFPS = !g_ShowFPS;
		}
#endif

		if (engineState != EngineState::STOP) {
			Scene::UI::UpdateButtons(globalECS);
			Scene::UI::UpdateInputs(globalECS);
		}

		// Particle Updates (Physics/Lifecycle)
		Particles::UpdateAllParticles(globalECS);
		Component::UpdateSpriteAnimationsECS();
	}
}
