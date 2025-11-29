/*!
@file   ppmanager.cpp
@author lee.r@digipen.edu
@date   05/11/2025

@brief	This file contains values and functions to manage post-processing.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "entitymanager.h"
#include "ppcomponent.h"
#include "assetmanager.h"
#include "shaderasset.h"
#include "ppmanager.h"
#include "engine.h"
#include <iostream>

/*                                                                   
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {

	namespace PostProcessing {

		using namespace Component;

		float g_brightness = 0.f;
		float g_contrast = 0.f;
		float g_saturation = 0.f;
		bool active = false;
		GLuint quadvao = 0;
		GLuint quadvbo = 0;
		GLuint ppFboID = 0;
		GLuint ppTextureID = 0;

		void InitPP() {
			// --- Create the Full-Screen Quad ---
			glGenVertexArrays(1, &quadvao);
			glGenBuffers(1, &quadvbo);

			glBindVertexArray(quadvao);
			glBindBuffer(GL_ARRAY_BUFFER, quadvbo);

			// Pass the vertices from your header
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticesNDC), &quadVerticesNDC, GL_STATIC_DRAW);

			// Attribute 0: Position (vec2)
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

			// Attribute 1: TexCoords (vec2)
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			glBindVertexArray(0); // Unbind

#if USE_IMGUI_EDITOR
			// --- Create the Post-Processing FBO (for ImGui only) ---
			// This is the *second* FBO. It catches the final image.
			glGenFramebuffers(1, &ppFboID);
			glBindFramebuffer(GL_FRAMEBUFFER, ppFboID);

			glGenTextures(1, &ppTextureID);
			glBindTexture(GL_TEXTURE_2D, ppTextureID);
			// Use the same size as the game texture
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gameTextureWidth, gameTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// Attach texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppTextureID, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR: Post-Processing Framebuffer is not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
		}

		void ShutdownPP() {
			glDeleteVertexArrays(1, &quadvao);
			glDeleteBuffers(1, &quadvbo);

#if USE_IMGUI_EDITOR
			glDeleteFramebuffers(1, &ppFboID);
			glDeleteTextures(1, &ppTextureID);
#endif
		}

		void UpdatePP() {
			

			// Get reference
			ECS& gECS = *ECS::getInstance();
			ECS::ComponentArray<PPComponent>& ppArray = gECS.GetComponentArray<PPComponent>();

			if (ppArray.indexToEntity.size() > 0) {
				PPComponent& p = ppArray.dense.back();
				if (!p.active) {
					if (active != p.active) active = p.active;
					return;
				}

				// Update values if changed
				if (g_brightness != p.brightness) g_brightness = p.brightness;
				if (g_contrast != p.contrast) g_contrast = p.contrast;
				if (g_saturation != p.saturation) g_saturation = p.saturation;
				if (active != p.active) active = p.active;
			}

#if USE_IMGUI_EDITOR

#else

#endif

		}

		void RenderPP(GLuint sceneTextureID) {


			GLSLShader& ppShader = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("PPShader")->shaderObject;
			ppShader.Use();

			if (active) {
				// Set uniforms
				glUniform1i(glGetUniformLocation(ppShader.GetHandle(), "u_active"), true);
				glUniform1f(glGetUniformLocation(ppShader.GetHandle(), "u_brightness"), g_brightness);
				glUniform1f(glGetUniformLocation(ppShader.GetHandle(), "u_contrast"), g_contrast + 1.f);
				glUniform1f(glGetUniformLocation(ppShader.GetHandle(), "u_saturation"), g_saturation + 1.f);
			}
			else {
				glUniform1i(glGetUniformLocation(ppShader.GetHandle(), "u_active"), false);
			}

			// Tell shader to read from texture unit 0
			glUniform1i(glGetUniformLocation(ppShader.GetHandle(), "u_sceneTexture"), 0);

			// Bind texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, sceneTextureID);

			// Draw full-screen quad
			glBindVertexArray(quadvao);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Unbind
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

	}

}
