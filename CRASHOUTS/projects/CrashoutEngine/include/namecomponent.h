/*!
@file   namecomponent.h
@author Renfred Lee
@author Ranvitha
@date   05/09/2025

@brief	This file defines the NameComponent, a simple data structure used to 
		store an entity's name.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
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

