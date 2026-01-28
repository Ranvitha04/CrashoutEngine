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
//#include "particleSystem.h"
//#include "camera.h"

#include <fstream>
#include <sstream>

#define FONT_SCALE 0.05f

namespace Scene {

	using namespace Component;
	using namespace CrashoutsEngine;
	using namespace CrashoutsEngine::Assets;

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
		@brief       Renders all text components and handles the blinking cursor for focused inputs.
		@param       globalECS Reference to the ECS system to retrieve components.
		*/
		static void RenderTextAndCursor(ECS& globalECS) {

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
			glEnable(GL_DEPTH_TEST);

			// --- DEFINE UI COORDINATE SYSTEM ---
			// We define UI Space as: Center Origin (0,0), Fixed Height = 100.0f.
			float winW = (float)GLApp::width;
			float winH = (float)GLApp::height;
			float aspect = winW / winH;
			float uiHeight = 100.0f;
			float uiWidth = uiHeight * aspect;

			// Lambda to convert Center-Origin Units -> Bottom-Left Origin Pixels
			auto UnitsToPixels = [&](float uX, float uY) -> CEVec2 {
				// 1. Map -50..50 to 0..1 (Normalized)
				float normX = (uX + (uiWidth * 0.5f)) / uiWidth;
				float normY = (uY + (uiHeight * 0.5f)) / uiHeight;

				// 2. Map 0..1 to Pixels
				return { normX * winW, normY * winH };
				};

			// For Drawing Text Components
			const ECS::ComponentArray<TextComponent>& textArray = globalECS.GetComponentArray<TextComponent>();

			// Ensure renderer exists
			if (!gTextRenderer) {
				gTextRenderer = std::make_unique<TextRenderer>();
			}

			for (const Entity& id : textArray.indexToEntity) {
				if (!IsEntityActive(globalECS, id)) continue;

				TextComponent* tc = globalECS.GetComponent<TextComponent>(id);
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
						// In STOP mode, reset or show full? Usually preview full or reset.
						// Let's reset to allow previewing "from start" when playing.
						tc->revealTimer = 0.0f;
						tc->revealedCharCount = 0;
						// If you want to see text in editor, maybe show full?
						// textToRender = tc->text; 
						// Or show empty? 
						// Let's show empty to respect the flag, or user can toggle flag off to see text.
						textToRender = "";
					}

					// --- COORDINATE SPACE CONVERSION ---
					if (!tc->isWorldSpace) {
						// Case: Screen Space UI
						// We must convert our Unit Position to Pixel Position because FreeType/TextRenderer
						// usually expects pixel coordinates starting from Bottom-Left.
						CEVec2 pixelPos = UnitsToPixels(finalX, finalY);
						finalX = pixelPos.x;
						finalY = pixelPos.y;
					}

					// Render Text

					if (tc->wrapText) {

						float maxWidth = t->size.x;

						float maxHeight = t->size.y;



						if (!tc->isWorldSpace) {

							// Convert dimensions from Units to Pixels (Full Size)

							maxWidth = (t->size.x / uiWidth) * winW;

							maxHeight = (t->size.y / uiHeight) * winH;

						}



						// Map UILayout to Text Alignment

						int align = 1; // CENTER

						int vAlign = 1; // MIDDLE



						if (u) {

							// Horizontal

							if (static_cast<bool>(u->uiLayout & UILayout::LEFT)) align = 0;

							else if (static_cast<bool>(u->uiLayout & UILayout::RIGHT)) align = 2;



							// Vertical

							if (static_cast<bool>(u->uiLayout & UILayout::TOP)) vAlign = 0;

							else if (static_cast<bool>(u->uiLayout & UILayout::DOWN)) vAlign = 2;

						}
						else {

							// Fallback to component internal values

							align = (int)tc->alignment;

							vAlign = (int)tc->verticalAlignment;

						}



						// Override finalX/Y for Bounded Text to use Shape Center

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

						// Blink Logic
						static float blinkTimer = 0.0f;
						blinkTimer += (float)FrameRateController::GetInstance().DT();
						bool showCursor = fmod(blinkTimer, 1.0f) < 0.5f;

						if (showCursor) {
							// For cursor calculation, we need the start X position.
							// If text is Centered, logic differs, but assuming Left-Aligned for input boxes usually.

							// We need the transform size in pixels if we are in Screen Mode to calculate line wrapping correctly,
							// but for simple single-line input:
							//float wrapWidth = (tc->isWorldSpace) ? t->size.x : (t->size.x / uiWidth * winW);

							// Get Cursor Pos from Renderer
							// Note: GetCursorPosition returns the offset relative to the text start.
							CEVec2 cursorPos = gTextRenderer->GetCursorPosition(
								tc->text,
								input->cursorPos,
								finalX, // The text starting X we calculated above
								finalY, // The text starting Y we calculated above
								1.0f,   // Scale handled inside GetCursorPosition usually, or pass finalScale
								font
							);

							// Define Cursor Geometry
							GLfloat cursorThickness = 0.25f;
							GLfloat cursorHeight = gTextRenderer->GetLineHeight(font, finalScale); // Pass scale
							CEColor cursorColor = { 0.f, 0.f, 0.f, 1.f };

							Assets::FontAsset* fontAsset = Assets::AssetManager::GetInstance().Get<Assets::FontAsset>(font);
							float ascender = cursorHeight * 0.75f;
							if (fontAsset && fontAsset->characters.count('H')) {
								// Rough estimate based on H
								ascender = (float)fontAsset->characters.at('H').Bearing.y * finalScale;
								// If isWorldSpace is false, we might need to scale this by pixel ratio
								if (!tc->isWorldSpace) ascender *= (winH / uiHeight * 0.02f); // Tweak factor
							}

							// Draw
							CEVec3 startPoint = { cursorPos.x, cursorPos.y + ascender * 0.5f, depthZ + 0.01f };
							CEVec3 endPoint = { cursorPos.x, cursorPos.y - ascender * 0.5f, depthZ + 0.01f };

							Shapes::CreateLine(
								startPoint, endPoint, cursorThickness,
								cursorColor, tc->isWorldSpace
							);
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
			// This matches the coordinate system used in RenderTextAndCursor logic (before pixel conversion)
			float winW = (float)GLApp::width;
			float winH = (float)GLApp::height;
			float aspect = winW / winH;
			float baseHeight = 100.0f;
			float baseWidth = baseHeight * aspect;

			CEVec2 rawMouse = InputManager::GetMousePosition(); // Top-Left Pixels

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
		static void UpdateInputs(ECS& globalECS) {
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
		@brief      Render loop for the new ParticleComponent.
		@details    Extracts data, splits Vec2 scales into float arrays, and handles Local Space transforms.
		*/
		static void RenderAllParticles(ECS& globalECS) {
			// Temp buffers to split CEVec2 scale into Width/Height arrays for the renderer
			static std::vector<float> s_scaleXBuffer;
			static std::vector<float> s_scaleYBuffer;
			static std::vector<float> s_uniformScaleBuffer; // Always 1.0f

			// Temp buffers for transforms
			static std::vector<CEVec3> s_posBuffer;
			static std::vector<CEVec2> s_uvOffsetBuffer;

			auto& particleArray = globalECS.GetComponentArray<ParticleComponent>();

			for (const Entity& id : particleArray.indexToEntity) {
				ParticleComponent* pc = globalECS.GetComponent<ParticleComponent>(id);
				TransformComponent* tc = globalECS.GetComponent<TransformComponent>(id);

				if (!pc || pc->data.activeCount == 0) continue;

				size_t count = pc->data.activeCount;

				// 1. Resize Temp Buffers
				if (s_scaleXBuffer.size() < count) s_scaleXBuffer.resize(count);
				if (s_scaleYBuffer.size() < count) s_scaleYBuffer.resize(count);
				if (s_uniformScaleBuffer.size() < count) s_uniformScaleBuffer.resize(count, 1.0f);
				if (s_uvOffsetBuffer.size() < count) s_uvOffsetBuffer.resize(count, { 0.f, 0.f });

				// Ensure uniform buffer is actually 1.0 (in case of resize dirty data)
				// We map 1.0 here so we can pass the actual dimensions into Dim1/Dim2
				std::fill(s_uniformScaleBuffer.begin(), s_uniformScaleBuffer.begin() + count, 1.0f);

				// 2. Setup Data Pointers
				CEVec3* posPtr = pc->data.positions.data();

				// 3. Handle Local/World Space & Split Scales
				// We need to loop anyway to split Vec2 -> Float, Float. 
				// We combine the Transform logic into this loop for efficiency.

				bool isLocal = (!pc->useWorldSpace && tc);
				CEVec3 parentPos = { 0,0,0 };
				float c = 1.f, s = 0.f;
				float parentScaleX = 1.0f;
				float parentScaleY = 1.0f;

				if (isLocal) {
					if (s_posBuffer.size() < count) s_posBuffer.resize(count);

					parentPos = tc->position;
					// If Local, particles should inherit Parent's size too
					parentScaleX = tc->size.x;
					parentScaleY = tc->size.y;

					float rad = tc->rotation * (3.14159f / 180.0f);
					c = cosf(rad);
					s = sinf(rad);
				}

				// Process every particle to prepare render buffers
				for (size_t k = 0; k < count; ++k) {
					// A. Split Scale (Vec2 -> Float, Float)
					// If Local, multiply by Parent Scale. If World, Parent Scale is 1.0.
					s_scaleXBuffer[k] = pc->data.scales[k].x * parentScaleX;
					s_scaleYBuffer[k] = pc->data.scales[k].y * parentScaleY;

					// B. Transform Position (if Local)
					if (isLocal) {
						CEVec3 local = pc->data.positions[k];
						float rotX = local.x * c - local.y * s;
						float rotY = local.x * s + local.y * c;

						s_posBuffer[k].x = rotX + parentPos.x;
						s_posBuffer[k].y = rotY + parentPos.y;
						s_posBuffer[k].z = local.z + parentPos.z;
					}
				}

				// If Local, redirect pointer to the transformed buffer
				if (isLocal) posPtr = s_posBuffer.data();

				// 4. Submit to Renderer
				InstancingSystem::InstancedParams params;
				params.textured = pc->useTexture;
				params.textureName = pc->textureName;
				params.uvSize = pc->textureUV;
				params.tint = { 1.0f, 1.0f, 1.0f };

				InstancingSystem::RenderInstancedRectanglesBatch(
					posPtr,                      // Correct World Position
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
	static void BatchEntities(
		const std::vector<Entity>& entities,
		ECS& globalECS,
		std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& rects,
		std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& tris,
		std::unordered_map<BatchKey, RenderBatch, BatchKeyHasher>& circs,
		std::vector<std::pair<TransformComponent*, ShapeComponent*>>& lines,
		bool enableCulling = false)
	{
		// Culling preperations

		RenderCamera::Camera& cam = (engineState == EngineState::STOP) ?
			RenderCamera::engineCamera : RenderCamera::gameCamera;
		CEVec3 camPos = cam.GetPosition();
		float camHeight = cam.GetVisibleWorldHeight();
		float camWidth = cam.GetVisibleWorldWidth();

		// Create a bounding box for the camera viewport
		float camMinX = camPos.x - (camWidth * 0.65f);
		float camMaxX = camPos.x + (camWidth * 0.65f);
		float camMinY = camPos.y - (camHeight * 0.65f);
		float camMaxY = camPos.y + (camHeight * 0.65f);

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

			f32 maxX = tf->position.x + tf->size.x;
			f32 minX = tf->position.x - tf->size.x;
			f32 maxY = tf->position.y + tf->size.y;
			f32 minY = tf->position.y - tf->size.y;

			if (enableCulling) {
				// If ANY of these are true, the object is completely outside the viewport
				if (maxX < camMinX || // Entirely to the left
					minX > camMaxX || // Entirely to the right
					maxY < camMinY || // Entirely below
					minY > camMaxY)   // Entirely above
				{
					continue;
				}
			}
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
				batch.Add(tf->position, colorNorm, tf->rotation, 1.0f, tf->size.x, tf->size.y, sc->uvOffset);
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
	static void FlushBatches(
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
				batch.scales.data(), batch.dim1.data(), batch.dim2.data(),
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

	void InitScene(CrashoutsEngine::ECS& globalECS) {
		Particles::InitAllParticles(globalECS);
		UI::InitText(globalECS);
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
			if (!IsEntityActive(globalECS, id)) continue;

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

		// ALSO Set standard shader matrices for non-instanced world objects
		renderer.shdr_pgm->Use();
		renderer.shdr_pgm->SetUniform("uView", renderer.viewMatrix);
		renderer.shdr_pgm->SetUniform("uProjection", renderer.projectionMatrix);

		// Store lines for later
		std::vector<std::pair<TransformComponent*, ShapeComponent*>> lineQueue;

		BatchEntities(worldEntities, globalECS, rectBatches, triBatches, circBatches, lineQueue, true);
		FlushBatches(rectBatches, triBatches, circBatches);

		// Render Particles (World Space)
		Particles::RenderAllParticles(globalECS);

		// Flush World Lines (Non-instanced)
		if (!lineQueue.empty()) {
			renderer.shdr_pgm->Use();
			// (Matrices already set above)

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

		// ALSO Set standard shader matrices for non-instanced UI objects
		renderer.shdr_pgm->Use();
		renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
		renderer.shdr_pgm->SetUniform("uProjection", uiProj);

		BatchEntities(screenEntities, globalECS, rectBatches, triBatches, circBatches, lineQueue, false);
		FlushBatches(rectBatches, triBatches, circBatches);

		// Flush Screen Lines
		if (!lineQueue.empty()) {
			renderer.shdr_pgm->Use();
			// (Matrices already set above)

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

		// Particle Updates (Physics/Lifecycle)
		Particles::UpdateAllParticles(globalECS);
	}
}