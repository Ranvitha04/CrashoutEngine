/*!
@file       aicomponent.h
@author     miratristan.d@digipen.edu
@date       31/10/2025
@brief      Declaration for the AI component, used to provide custom AI logic to entities. Used in tandem with a Finite State Machine

//__________________________________________________________________________*/

#pragma once

#include "state.h"

namespace Component {
	struct AIComponent {
		State::FiniteStateMachine<State::StateType> fsm;
	};
}