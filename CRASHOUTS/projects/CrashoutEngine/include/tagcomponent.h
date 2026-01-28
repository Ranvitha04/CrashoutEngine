/*!
@file       tagcomponent.h
@author     miratristan.d@digipen.edu
@date       31/10/2025
@brief      This file defines the TagComponent, used to store an entity's tag 
			bitflag for collision filtering.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "state.h"

namespace Component {
	//struct Tag {
	//	std::string id;
	//};

	/*!
	@struct	TagComponent
	@brief	Provides a human-readable tag name & bit-flag.
	*/
	struct TagComponent {
		static std::map<std::string, unsigned int> tags;
		unsigned int tagMask;
	};
}