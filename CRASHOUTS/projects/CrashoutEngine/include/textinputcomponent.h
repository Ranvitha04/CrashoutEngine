/*!
@file   textinputcomponent.h
@author lee.r@digipen.edu
@date   02/11/2025

@brief	This file defines the TextInputComponent
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

/*
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	TextInputComponent
	@brief	A component that provides access to data required by the text input component
	*/
	struct TextInputComponent {
		bool isFocused{ false };
		size_t cursorPos{ 0 };
		size_t maxCharacters{ 255 };
	};
}

