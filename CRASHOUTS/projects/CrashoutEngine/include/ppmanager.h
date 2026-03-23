/*!
@file   ppmanager.h
@author Shahrin Idzuan
@author Ranvitha
@author Renfred Lee
@date   05/11/2025

@brief	This file contains values and functions to manage post-processing.

All content ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¯ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¿ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â½ 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
		extern float g_ambient;
		extern bool g_bloomEnabled;
		extern float g_bloomThreshold;
		extern float g_bloomIntensity;
		extern int g_bloomIterations;		
		extern bool active;
		extern Entity g_ppSourceEntity;
		extern Entity g_ppOverrideEntity;
		extern bool g_heatEnabled;
		extern bool g_heatDebugMarker;
		extern float g_heatStrength;
		extern float g_heatSpeed;
		extern float g_heatScale;
		extern bool g_distortionEnabled;
		extern bool g_distortionDebugMarker;
		extern float g_distortionStrength;
		extern float g_distortionSpeed;
		extern float g_distortionWaveScale1;
		extern float g_distortionWaveScale2;
		extern float g_distortionNoiseStrength;

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
