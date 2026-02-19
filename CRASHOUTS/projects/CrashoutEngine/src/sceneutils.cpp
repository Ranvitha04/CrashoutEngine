/*!
@file        sceneutils.cpp
@author      lee.r@digipen.edu
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

#include "guicomponent.h"
#include "particleComponent.h"
#include "shapescomponent.h"
#include "transformcomponent.h"
//#include "namecomponent.h"
#include "textcomponent.h"
#include "shapes.h"
#include "uilayoutcomponent.h"
#include "textinputcomponent.h"
#include "buttoncomponent.h"
//#include "tilemapcomponent.h"
#include "activecomponent.h"
#include "particleSystem.h"
#include "spriteAnimation.h"
#include "hierarchycomponent.h"
//#include "camera.h"
#include "performanceSystem.h"
#include "CEMath.h"

#include <fstream>
#include <sstream>

#define FONT_SCALE 0.05f

namespace Scene {

	using namespace Component;
	using namespace CrashoutsEngine;
	using namespace CrashoutsEngine::Assets;

	static std::unique_ptr<TextRenderer> gTextRenderer = nullptr;

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

		void LoadDialogue(TextComponent& tc) {
			if (tc.dialogueFilePath.empty() || tc.dialogueID.empty()) return;

			std::string fullPath = CrashoutsEngine::Assets::rootPath + tc.dialogueFilePath;
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
		static void RenderSingleText(ECS& globalECS, Entity id, const glm::mat4& viewMat, const glm::mat4& projMat) {

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
			// [FIX] Use Game View dimensions for consistency with Input/Editor

			CEVec2 gameSize = InputManager::GetGameWindowSize();
			float winW = gameSize.x;
			float winH = gameSize.y;
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
					bool showCursor = fmod(blinkTimer, 1.0f) < 0.5f;

					if (showCursor) {
						CEVec2 cursorPos = gTextRenderer->GetCursorPosition(
							tc->text, input->cursorPos, finalX, finalY, 1.0f, font);

						GLfloat cursorThickness = 0.25f;
						GLfloat cursorHeight = gTextRenderer->GetLineHeight(font, finalScale);
						CEColor cursorColor = { 0.f, 0.f, 0.f, 1.f };

						Assets::FontAsset* fontAsset = Assets::AssetManager::GetInstance().Get<Assets::FontAsset>(font);
						float ascender = cursorHeight * 0.75f;
						if (fontAsset && fontAsset->characters.count('H')) {
							ascender = (float)fontAsset->characters.at('H').Bearing.y * finalScale;
							if (!tc->isWorldSpace) ascender *= (winH / uiHeight * 0.02f);
						}

						CEVec3 startPoint = { cursorPos.x, cursorPos.y + ascender * 0.5f, depthZ + 0.01f };
						CEVec3 endPoint = { cursorPos.x, cursorPos.y - ascender * 0.5f, depthZ + 0.01f };

						Shapes::CreateLine(startPoint, endPoint, cursorThickness, cursorColor, tc->isWorldSpace);
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
			CEVec2 gameSize = InputManager::GetGameWindowSize();
			float winW = gameSize.x;
			float winH = gameSize.y;

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

				// Base Scale (assumes 1.0 is default/normal scale multiplier)
				float targetScale = 1.0f;
				std::string targetTexture = "";

				// Resolve target texture logic
				// 1. Start with Normal
				targetTexture = b->textureNormal;

				if (isHovering) {
					if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
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
			CEVec2 gameSize = InputManager::GetGameWindowSize();
			float winW = gameSize.x;
			float winH = gameSize.y;

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
			for (const Entity& id : particleArray.indexToEntity) {
				if (!IsEntityActive(globalECS, id)) continue;

				ParticleComponent* pc = globalECS.GetComponent<ParticleComponent>(id);
				if (pc) {
					// Call the Update function declared in particleComponent.h
					if (engineState == EngineState::PLAY || pc->playOnStop) ParticleSystem::ParticleUpdate(id, *pc, dt);
					else pc->data.activeCount = 0;
				}
			}
		}

		/*!
		@brief      Renders a single particle system, applying the computed layer depth.
		*/
		static void RenderParticleSystem(ECS& globalECS, Entity id) {
			// Temp buffers to split CEVec2 scale into Width/Height arrays for the renderer
			static std::vector<float> s_scaleXBuffer;
			static std::vector<float> s_scaleYBuffer;
			static std::vector<float> s_uniformScaleBuffer; // Always 1.0f

			// Temp buffers for transforms
			static std::vector<CEVec3> s_posBuffer;
			static std::vector<CEVec2> s_uvOffsetBuffer;

			auto& particleArray = globalECS.GetComponentArray<ParticleComponent>();

			ParticleComponent* pc = globalECS.GetComponent<ParticleComponent>(id);
			TransformComponent* tc = globalECS.GetComponent<TransformComponent>(id);

			if (!pc || pc->data.activeCount == 0) return;

			size_t count = pc->data.activeCount;

			// 1. Resize Temp Buffers
			if (s_scaleXBuffer.size() < count) s_scaleXBuffer.resize(count);
			if (s_scaleYBuffer.size() < count) s_scaleYBuffer.resize(count);
			if (s_uniformScaleBuffer.size() < count) s_uniformScaleBuffer.resize(count, 1.0f);
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
				s_scaleXBuffer[k] = pc->data.scales[k].x * parentScaleX;
				s_scaleYBuffer[k] = pc->data.scales[k].y * parentScaleY;

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

			// Bind Texture if needed
			if (batch.enableTexture && !batch.textureName.empty()) {
				auto* tex = Assets::AssetManager::GetInstance().Get<Assets::TextureAsset>(batch.textureName);
				if (tex) tex->SetTextureMode(batch.wrapU, batch.wrapV);
			}

			InstancingSystem::InstancedParams params;
			params.textured = batch.enableTexture;
			params.textureName = batch.textureName;
			params.textureHandle = batch.textureHandle; // Pass handle
			params.tint = { 1,1,1 };
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
			// Special Case: Lines cannot be instanced in the same batch
			if (sc->type == ShapeType::Line) {
				Flush(); // Break current batch
				Component::Shapes::CreateLine(sc->p1 + tc->position, sc->p2 + tc->position, sc->thickness, sc->color, sc->isWorldSpace);
				return;
			}

			// Check for State Changes
			bool typeChanged = (sc->type != currentShapeType);
			bool textureChanged = false;

			// Only check texture state if we aren't starting fresh
			if (!batch.positions.empty()) {
				if (sc->enableTexture != batch.enableTexture) textureChanged = true;
				else if (sc->enableTexture) {
					// Check Name OR Handle
					if (sc->textureName != batch.textureName) textureChanged = true;
					else if (sc->textureHandle != batch.textureHandle) textureChanged = true; // Handle Check
					else if (sc->uvSize != batch.uvSize) textureChanged = true; // UV Size is uniform
					// Note: Wrap modes usually don't change often, but if they do, we must flush
					else if (GetWrapMode(sc->wrapModeU) != batch.wrapU) textureChanged = true;
				}
			}

			// Flush if state doesn't match
			if (textureChanged || typeChanged) {
				Flush();
			}

			// Init Batch properties if empty
			if (batch.positions.empty()) {
				currentShapeType = sc->type;
				batch.textureName = sc->textureName;
				batch.textureHandle = sc->textureHandle; // Capture handle
				batch.enableTexture = sc->enableTexture;
				batch.uvSize = sc->uvSize;
				batch.wrapU = GetWrapMode(sc->wrapModeU);
				batch.wrapV = GetWrapMode(sc->wrapModeV);
			}

			// Add Data
			CEColor colorNorm(sc->color.x(), sc->color.y(), sc->color.z(), sc->color.w());
			float dim1 = tc->size.x;
			float dim2 = tc->size.y;
			if (sc->type == ShapeType::Circle) { dim1 = tc->size.x * 0.5f; dim2 = 0.0f; }

			batch.Add(tc->position, colorNorm, tc->rotation, 1.0f, dim1, dim2, sc->uvOffset);
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

		struct RenderEntry {
			Entity entity;
			float depth;
			RenderType type;
		};

		static std::vector<RenderEntry> worldEntries;
		static std::vector<RenderEntry> screenEntries;
		worldEntries.clear();
		screenEntries.clear();

		size_t reserveSize = shapeArray.indexToEntity.size() + textArray.indexToEntity.size();
		if (worldEntries.capacity() < reserveSize) worldEntries.reserve(reserveSize);
		if (screenEntries.capacity() < reserveSize) screenEntries.reserve(reserveSize);

		// 2a. PRE-SORT SHAPES
		for (const Entity& id : shapeArray.indexToEntity) {
			if (!IsEntityActive(globalECS, id)) continue;
			auto* tc = globalECS.GetComponent<TransformComponent>(id);
			if (!tc) continue;
			auto* sc = globalECS.GetComponent<ShapeComponent>(id);

			// Compute Depth
			float depth = 0.0f;
			if (globalECS.HasComponent<Component::LayerComponent>(id)) {
				auto* z = globalECS.GetComponent<Component::LayerComponent>(id);
				if (Component::LayerComponent::layerVisibility.count(z->layerName)) {
					if (!Component::LayerComponent::layerVisibility[z->layerName]) continue;
				}
				DebugZOverride* dbg = globalECS.HasComponent<DebugZOverride>(id) ? globalECS.GetComponent<DebugZOverride>(id) : nullptr;
				depth = Component::Rendering::ComputeDepth(tc->position, *z, dbg);
			}
			else {
				Component::LayerComponent defaultZ{ "Default", 0 };
				if (Component::LayerComponent::layerVisibility.count("Default")) {
					if (!Component::LayerComponent::layerVisibility["Default"]) continue;
				}
				depth = Component::Rendering::ComputeDepth(tc->position, defaultZ, nullptr);
			}

			if (sc->isWorldSpace) worldEntries.push_back({ id, depth, RenderType::Shape });
			else                  screenEntries.push_back({ id, depth, RenderType::Shape });
		}

		// 2b. PRE-SORT TEXT
		for (const Entity& id : textArray.indexToEntity) {
			if (!IsEntityActive(globalECS, id)) continue;
			auto* tc = globalECS.GetComponent<TransformComponent>(id);
			if (!tc) continue;
			auto* txt = globalECS.GetComponent<TextComponent>(id);

			// Compute Depth
			float depth = 0.0f;
			if (globalECS.HasComponent<Component::LayerComponent>(id)) {
				auto* z = globalECS.GetComponent<Component::LayerComponent>(id);
				if (Component::LayerComponent::layerVisibility.count(z->layerName)) {
					if (!Component::LayerComponent::layerVisibility[z->layerName]) continue;
				}
				DebugZOverride* dbg = globalECS.HasComponent<DebugZOverride>(id) ? globalECS.GetComponent<DebugZOverride>(id) : nullptr;
				depth = Component::Rendering::ComputeDepth(tc->position, *z, dbg);
			}
			else {
				Component::LayerComponent defaultZ{ "Default", 0 };
				if (Component::LayerComponent::layerVisibility.count("Default")) {
					if (!Component::LayerComponent::layerVisibility["Default"]) continue;
				}
				depth = Component::Rendering::ComputeDepth(tc->position, defaultZ, nullptr);
			}

			if (txt->isWorldSpace) worldEntries.push_back({ id, depth, RenderType::Text });
			else                   screenEntries.push_back({ id, depth, RenderType::Text });
		}

		// 2c. PRE-SORT PARTICLES
		const auto& particleArray = globalECS.GetComponentArray<ParticleComponent>();
		for (const Entity& id : particleArray.indexToEntity) {
			if (!IsEntityActive(globalECS, id)) continue;
			auto* pc = globalECS.GetComponent<ParticleComponent>(id);
			if (!pc || pc->data.activeCount == 0) continue;

			auto* tc = globalECS.GetComponent<TransformComponent>(id);
			CEVec3 basePos = tc ? tc->position : CEVec3(0, 0, 0);

			// Compute Depth
			float depth = 0.0f;
			if (globalECS.HasComponent<Component::LayerComponent>(id)) {
				auto* z = globalECS.GetComponent<Component::LayerComponent>(id);
				if (Component::LayerComponent::layerVisibility.count(z->layerName)) {
					if (!Component::LayerComponent::layerVisibility[z->layerName]) continue;
				}
				DebugZOverride* dbg = globalECS.HasComponent<DebugZOverride>(id) ? globalECS.GetComponent<DebugZOverride>(id) : nullptr;
				depth = Component::Rendering::ComputeDepth(basePos, *z, dbg);
			}
			else {
				Component::LayerComponent defaultZ{ "Default", 0 };
				if (Component::LayerComponent::layerVisibility.count("Default")) {
					if (!Component::LayerComponent::layerVisibility["Default"]) continue;
				}
				depth = Component::Rendering::ComputeDepth(basePos, defaultZ, nullptr);
			}

			if (pc->useWorldSpace) worldEntries.push_back({ id, depth, RenderType::Particle });
			else                   screenEntries.push_back({ id, depth, RenderType::Particle });
		}

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
					Scene::UI::RenderSingleText(globalECS, item.entity, viewMat, projMat);

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
		// Screen space UI shouldn't be culled by the World Camera bounds
		float aspect = (float)GLApp::width / (float)GLApp::height;
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

		Component::RenderSpriteAnimationsECS();
	}

	void UpdateScene(CrashoutsEngine::ECS& globalECS) {
		using namespace CrashoutsEngine;
		if (engineState != EngineState::STOP) {
			Scene::UI::UpdateButtons(globalECS);
			Scene::UI::UpdateInputs(globalECS);
		}

		// Particle Updates (Physics/Lifecycle)
		Particles::UpdateAllParticles(globalECS);
		Component::UpdateSpriteAnimationsECS();
	}
}
