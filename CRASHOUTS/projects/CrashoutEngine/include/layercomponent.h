/*!
@file   layercomponent.h
@author lee.r@digipen.edu
@date   05/09/2025

@brief	This file defines the layercomponent used for zordering + game enabling/ grouping

All content  2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <string>
#include <map>

/*
----------------------------------------------------------------------------- */
namespace Component {

	// Each drawable entity has this struct
	struct LayerComponent {
		std::string layerName; // Which group/layer
		int z;                 // Fine ordering within the layer

		// Static configuration for all layers
		static std::map<std::string, int> layerMap;
		static std::map<std::string, bool> layerVisibility;
	};

}
