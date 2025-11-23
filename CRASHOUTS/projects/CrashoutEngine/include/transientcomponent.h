/*!
@file   transientcomponent.h
@author lee.r@digipen.edu
@date   23/11/2025

@brief	This file contains the transientcomponent flag, which tells others that
		this is a runtime only entity.
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
	struct TransientComponent {
		// No data needed. Just a flag.
	};

}

