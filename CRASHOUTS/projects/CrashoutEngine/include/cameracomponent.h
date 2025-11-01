/*!
@file   cameracomponent.h
@author lee.r@digipen.edu
@date   17/09/2025

@brief	This file defines the CameraComponent
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
	struct CameraComponent {
		CEVec3 position{ 0.f, 0.f, 0.f };
		float zoom{ 1.f };
		//float targetAR{ 16.f / 9.f };

		CEVec2 xBounds{ 0.f, 0.f };
		CEVec2 yBounds{ 0.f, 0.f };
		
	};

}

