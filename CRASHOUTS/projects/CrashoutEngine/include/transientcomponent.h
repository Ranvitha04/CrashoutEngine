/*!
@file   transientcomponent.h
@author Renfred Lee
@date   23/11/2025

@brief  This file contains the transientcomponent flag, which tells others that
		this is a runtime only entity.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <string>

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	TransientComponent
	@brief	A component that flags entity as runtime only.
	*/
	/** @brief TransientComponent */
	struct TransientComponent {
		// No data needed. Just a flag.
	};

}

