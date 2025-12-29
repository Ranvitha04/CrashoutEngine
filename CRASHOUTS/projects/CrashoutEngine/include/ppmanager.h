/*!
@file   ppmanager.h
@author lee.r@digipen.edu
@date   05/11/2025

@brief	This file contains values and functions to manage post-processing.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

/*                                                                   
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {

	namespace PostProcessing {

		extern float g_brightness;
		extern float g_contrast;
		extern float g_saturation;
		
		extern bool active;

		// --- Declare OpenGL Handles ---
		extern GLuint quadvao; // The full-screen quad VAO
		extern GLuint quadvbo; // The VBO for the quad

		// These are ONLY used for the ImGui path
		extern GLuint ppFboID;
		extern GLuint ppTextureID;

		// X, Y		U, V
		const float quadVerticesNDC[]{
			-1.0f,  1.0f,   0.0f, 1.0f,
		   -1.0f, -1.0f,   0.0f, 0.0f,
			1.0f, -1.0f,   1.0f, 0.0f,

		   -1.0f,  1.0f,   0.0f, 1.0f,
			1.0f, -1.0f,   1.0f, 0.0f,
			1.0f,  1.0f,   1.0f, 1.0f
		};

		void InitPP();
		void ShutdownPP();
		void UpdatePP();
		void RenderPP(GLuint sceneTextureID);

	}

}
