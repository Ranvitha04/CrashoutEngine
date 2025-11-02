/*!
@file   buttoncomponent.h
@author lee.r@digipen.edu
@date   02/11/2025

@brief	This file defines the ButtonComponent
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <type_traits>
#include <CEVec3.h>

/*
----------------------------------------------------------------------------- */
namespace Component {

	namespace UI {
		enum class ButtonState : unsigned int { 
			NORMAL	= 0, 
			HOVERED = 1 << 0, 
			PRESSED = 1 << 1
		};

		/*!
		@struct	ButtonComponent
		@brief	A component that provides access to data required by the button component
		*/
		struct ButtonComponent {
			ButtonState buttonState{ ButtonState::NORMAL };

			// Stick to CEVec3 for now since the rest doesn't take into account alpha

			// State Colours for Shape Component
			//float shapeNormalColor[4] = { 255.f, 255.f, 255.f, 255.f };
			//float shapeHoveredColor[4] = { 100.f, 100.f, 100.f, 255.f };
			//float shapePressedColor[4] = { 0.f, 0.f, 0.f, 255.f };
			CEVec3 shapeNormalColor  = { 255.f, 255.f, 255.f };
			CEVec3 shapeHoveredColor = { 100.f, 100.f, 100.f };
			CEVec3 shapePressedColor = { 0.f, 0.f, 0.f };

			// State Colours for Text Component
			//float textNormalColor[4] = { 255.f, 255.f, 255.f, 255.f };
			//float textHoveredColor[4] = { 100.f, 100.f, 100.f, 255.f };
			//float textPressedColor[4] = { 0.f, 0.f, 0.f, 255.f };
			CEVec3 textNormalColor	= { 255.f, 255.f, 255.f };
			CEVec3 textHoveredColor = { 100.f, 100.f, 100.f };
			CEVec3 textPressedColor = { 0.f, 0.f, 0.f };
		};
	}
}

