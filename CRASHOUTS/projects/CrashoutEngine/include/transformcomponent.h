/*!
@file   transformcomponent.h
@author Renfred Lee
@author Ranvitha
@author tristan
@date   05/09/2025

@brief	This file defines the TransformComponent, a simple data structure 
		used to store an entity's 3D position, rotation, and scale.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
	@struct	TransformComponent
	@brief	A component that holds an entity's position, rotation, and scale.
	*/
	struct TransformComponent {
		// Rotational transform
		float rotation = 0.f;
		// Scaling transform
		CEVec2 size{1.f, 1.f};
		// X Y Z position
		CEVec3 position{ 0.f,0.f,0.f };
		CEVec3 previousPosition{ 0.f, 0.f, 0.f };
	};
}

