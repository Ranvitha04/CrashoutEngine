/*!
@file   activecomponent.h
@author Renfred Lee
@author Shahrin Idzuan
@date   18/01/2025

@brief	This file defines the ActiveComponent, it determines if the gameobject.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
	struct ActiveComponent {
		bool isActive{ true };
		bool worldActive{ true }; // Cached result of hierarchy traversal
	};

}