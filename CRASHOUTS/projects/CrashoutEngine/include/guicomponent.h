/*!
@file   guicomponent.h
@author Ranvitha (51%), Renfred (RTIS) (37%), Renfred Lee (12%)
@date   02/11/2025

@brief  This file defines the GuiComponent

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <type_traits>

/*
----------------------------------------------------------------------------- */
namespace Component {

	enum class UIType : unsigned int {
		NONE = 0,
		IMAGE = 1 << 0,
		TEXT = 1 << 1,
		BUTTON = 1 << 2,
		INPUTTEXT = 1 << 3
	};

	// Overload operators to allow bitwise operations
	inline UIType operator|(UIType a, UIType b) {
		return static_cast<UIType>(
			static_cast<std::underlying_type_t<UIType>>(a) |
			static_cast<std::underlying_type_t<UIType>>(b)
			);
	}

	inline UIType operator&(UIType a, UIType b) {
		return static_cast<UIType>(
			static_cast<std::underlying_type_t<UIType>>(a) &
			static_cast<std::underlying_type_t<UIType>>(b)
			);
	}

	/*!
	@struct	GuiComponent
	@brief	A component that provides access to data required by the GUI component
	*/
	/** @brief GuiComponent */
	struct GuiComponent {
		UIType uiType{ UIType::NONE };
	};
}
