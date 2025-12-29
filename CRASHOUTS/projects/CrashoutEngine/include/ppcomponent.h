/*!
@file   ppcomponent.h
@author lee.r@digipen.edu
@date   05/11/2025

@brief	This file defines the post-processing component. 

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	PPComponent
	@brief	A component that holds the holds modifiable data for post-processing.
	*/
	struct PPComponent {
		// Active bool
		bool active{ true };
		// Brightness
		float brightness{ 0.f };
		// Contrast
		float contrast{ 0.f };
		// Saturation
		float saturation{ 0.f };
	};
}

