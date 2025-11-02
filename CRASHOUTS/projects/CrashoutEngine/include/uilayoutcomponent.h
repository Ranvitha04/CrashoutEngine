/*!
@file   uilayoutcomponent.h
@author lee.r@digipen.edu
@date   02/11/2025

@brief	This file defines the UILayoutComponent
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "CEVec2.h"
#include <type_traits>

/*
----------------------------------------------------------------------------- */
namespace Component {

	namespace UI {

		enum class UILayout : unsigned int {
			TOP		= 0,
			DOWN	= 1 << 0,
			CENTER	= 1 << 1,
			LEFT	= 1 << 2,
			RIGHT	= 1 << 3
		};

		// Overload operators to allow bitwise operations
		inline UILayout operator|(UILayout a, UILayout b) {
			return static_cast<UILayout>(
				static_cast<std::underlying_type_t<UILayout>>(a) |
				static_cast<std::underlying_type_t<UILayout>>(b)
				);
		}

		inline UILayout operator&(UILayout a, UILayout b) {
			return static_cast<UILayout>(
				static_cast<std::underlying_type_t<UILayout>>(a) &
				static_cast<std::underlying_type_t<UILayout>>(b)
				);
		}

		/*!
		@struct	UILayoutComponent
		@brief	A component that provides access to data required by the UI layout component
		*/
		struct UILayoutComponent {
			CEVec2 paddingXY{ 0.f, 0.f };
			UILayout uiLayout{ UILayout::CENTER };
		};
	}
}

