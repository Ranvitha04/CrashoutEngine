/*!
@file   buttoncomponent.h
@author lee.r@digipen.edu
@date   02/11/2025

@brief	This file defines the ButtonComponent

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <CEColor.h>

/*
----------------------------------------------------------------------------- */
namespace Component {

	enum class ButtonState : unsigned int {
		NORMAL = 0,
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
		CEColor shapeNormalColor = { 1.f, 1.f, 1.f, 1.f };
		CEColor shapeHoveredColor = { 1.f, 1.f, 1.f, 0.5f };
		CEColor shapePressedColor = { 0.7f, 0.7f, 0.7f, 0.7f };

		// State Colours for Text Component;
		CEColor textHoveredColor = { 0.f, 0.f, 0.f, 0.f };
		CEColor textPressedColor = { 0.f, 0.f, 0.f, 0.5f };
		CEColor textNormalColor = { 0.3f, 0.3f, 0.3f, 0.7f };
	};
}

