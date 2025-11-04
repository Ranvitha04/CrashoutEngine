/*!
@file       aimanager.h
@author     miratristan.d@digipen.edu
@date       31/10/2025
@brief      Declaration for the AI manager, used to update any entity with an attached state machine.

//__________________________________________________________________________*/

#pragma once

#include "entitymanager.h"

namespace AIUtils {
	bool IsPlatformInFront(CrashoutsEngine::Entity entity, float direction);
}


namespace CrashoutsEngine {
	class AIManager {
		public:
			AIManager() = default;
			void Update();
	};
}