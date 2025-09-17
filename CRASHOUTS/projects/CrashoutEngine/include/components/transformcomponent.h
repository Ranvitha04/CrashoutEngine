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

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	TransformComponent
	@brief	A component that holds an entity's position, rotation, and scale.
	*/
	struct TransformComponent {
		// X Y Z position
		float posX, posY, posZ;
		// Rotational transform
		float rotX, rotY, rotZ;
		// Scaling transform
		float scaleX, scaleY, scaleZ;
	};

}

