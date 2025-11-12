/*!
@file   layercomponent.h
@author lee.r@digipen.edu
@date   05/09/2025

@brief	This file defines the LayerComponent, a simple data structure used to 
		store an entity's layer name and bitflag.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <string>
#include <map>

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	LayerComponent
	@brief	A component that provides a human-readable layer name & bit-flag 
			for an entity, as well as layerMask (layers to ignore).
	*/
	struct LayerComponent{
		static std::map<std::string, unsigned int> layers;
		unsigned int currentLayer;
	};

}

