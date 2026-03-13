/*!
@file   textcomponent.h
@author Renfred Lee
@author Renfred (RTIS)
@author Ranvitha
@date   23/10/2025

@brief	This file defines the TextComponent
All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <CEVec2.h>
#include <CEColor.h>
#include <string>

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	enum class TextAlignment {
		LEFT,
		CENTER,
		RIGHT
	};

	enum class TextVerticalAlignment {
		TOP,
		MIDDLE,
		BOTTOM
	};

	/*!
	@struct	TextComponent
	@brief	A component that holds the font name, string to display,scale, offset position, color, and centered bool.
	*/
	struct TextComponent {
		// font name
		std::string font{ "Exo2-Regular" };
		// string to display
		std::string text{ "TEST" };
		// scale
		float scale{ 1.f };
		// offset position
		CEVec2 offset{ 0.f, 0.f };
		// color
		CEColor color{ 1.f, 1.f, 1.f, 1.f };
		// centered bool (DEPRECATED: Use alignment instead, but kept for compatibility)
		bool centered{ true };
		// is world space bool
		bool isWorldSpace{ true };
		
		// New Alignment Properties
		TextAlignment alignment{ TextAlignment::CENTER };
		TextVerticalAlignment verticalAlignment{ TextVerticalAlignment::MIDDLE };
		bool wrapText{ false };
		
		// Padding (X=Left/Right, Y=Top/Bottom) - applied when wrapped/bounded
		CEVec2 padding{ 0.f, 0.f };
		
		// Spacing
		float letterSpacing{ 0.0f };
		float lineSpacing{ 0.0f };

		// Typewriter Effect
		bool isRevealing{ false };
		float revealSpeed{ 10.0f }; // Characters per second
		float revealTimer{ 0.0f };
		int revealedCharCount{ 0 };

		// External Dialogue Loading
		std::string dialogueFilePath{ "" };
		std::string dialogueID{ "" };
	};
}

