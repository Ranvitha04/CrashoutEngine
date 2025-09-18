/*!
@file   transformcomponent.h
@author lee.r@digipen.edu
@date   05/09/2025

@brief	This file defines the TransformComponent, a simple data structure 
		used to store an entity's 3D position, rotation, and scale.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "math/CEVec3.h"
#include "math/CEVec2.h"

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	TransformComponent
	@brief	A component that holds an entity's position, rotation, and scale.
	*/
	struct TransformComponent {
		// Rotational transform
		float rotation;
		// Scaling transform
		CEVec2 size{};
		// X Y Z position
		CEVec3 position{};
	};

}

