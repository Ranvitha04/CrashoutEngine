/*!
@file   activecomponent.h
@author Shahrin Idzuan (94%), Renfred Lee (6%)
@date   18/01/2025

@brief  This file defines the ActiveComponent, it determines if the gameobject.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	ActiveComponent
	@brief	A component that determines if the "gameobject" is active.
	*/
	/** @brief ActiveComponent */
	struct ActiveComponent {
		bool isActive{ true };
		bool worldActive{ true }; // Cached result of hierarchy traversal
	};

}