/*!
@file   textinputcomponent.h
@author Renfred Lee 
@date   02/11/2025

@brief  This file defines the TextInputComponent

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
	/** @brief TextInputComponent */
	struct TextInputComponent {
		bool isFocused{ false };
		size_t cursorPos{ 0 };
		size_t maxCharacters{ 255 };
	};
}

