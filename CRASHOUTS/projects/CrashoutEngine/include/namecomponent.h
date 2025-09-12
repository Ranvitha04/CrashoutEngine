/*!
@file       namecomponent.h
@author     lee.r@digipen.edu
@date       05/09/2025

This file defines the NameComponent, a simple data structure used to store an 
entity's name.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <vector>
#include <string>

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	NameComponent
	@brief	A component that provides a human-readable name for an entity.
	*/
	struct NameComponent {
		std::string name;
	};

}

