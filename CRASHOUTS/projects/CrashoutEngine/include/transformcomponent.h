/*!
@file       transformcomponent.h
@author     lee.r@digipen.edu
@date       05/09/2025

This file defines the TransformComponent, a simple data structure used to 
store an entity's 3D position, rotation, and scale.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	struct TransformComponent {
		// X Y Z position
		float posX, posY, posZ;
		// Rotational transform
		float rotX, rotY, rotZ;
		// Scaling transform
		float scaleX, scaleY, scaleZ;
	};

}

