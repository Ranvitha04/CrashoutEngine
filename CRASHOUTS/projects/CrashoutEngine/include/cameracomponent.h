/*!
@file   cameracomponent.h
@author lee.r@digipen.edu
@date   17/09/2025

@brief	This file defines the CameraComponent
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "CEVec2.h"
#include "CEVec3.h"

#include <glm/gtc/matrix_transform.hpp>

/*
----------------------------------------------------------------------------- */
namespace Component {

	using Entity = uint32_t;

	/*!
	@struct	CameraComponent
	@brief	A component that provides
	*/
	struct CameraComponent {
		bool enabled{ false };

		CEVec3 position{ 0.f, 0.f, 0.f };
		float zoom{ 1.f };
		float targetAR{ 16.f / 9.f };

		Entity targetEntity;
	};

}

