/*!
@file   cameracomponent.h
@author Renfred Lee (45%), Renfred (RTIS) (33%), Ranvitha (22%)
@date   17/09/2025

@brief  This file defines the CameraComponent

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "CEVec3.h"
#include "CEVec2.h"

/*
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	CameraComponent
	@brief	A component that provides access to data required by the camera component
	*/
	/** @brief CameraComponent */
	struct CameraComponent {
		CEVec3 position{ 0.f, 0.f, 0.f };
		float zoom{ 1.f };
		//float targetAR{ 16.f / 9.f };

		CEVec2 xBounds{ 0.f, 0.f };
		CEVec2 yBounds{ 0.f, 0.f };
		
	};

}

