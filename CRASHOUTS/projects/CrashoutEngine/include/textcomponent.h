/*!
@file   textcomponent.h
@author lee.r@digipen.edu
@date   23/10/2025

@brief	This file defines the TextComponent
All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
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
		// centered bool
		bool centered{ true };
		// is world space bool
		bool isWorldSpace{ true };
	};
}

