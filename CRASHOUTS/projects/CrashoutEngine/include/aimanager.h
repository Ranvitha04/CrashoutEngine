/*!
@file       aimanager.h
@author     miratristan.d@digipen.edu
@date       31/10/2025
@brief      Declaration for the AI manager, used to update any entity with an attached state machine.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "entitymanager.h"

namespace AIUtils {
	template <typename T> int sign(T val) { /*RETURN SIGN OF DOT PRODUCT*/
		return (T(0) < val) - (val < T(0));
	}

	bool IsPlatformInFront(CrashoutsEngine::Entity entity, float direction, std::string tag = "P");
	bool IsObstacleInFront(CrashoutsEngine::Entity entity, float direction, std::string tag = "W");
	bool IsObstacleIntersecting(CrashoutsEngine::Entity entity, std::string tag = "W");
	bool IsIntersecting(CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2);
	bool EntityInRange(CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2, float tolerance);
}


namespace CrashoutsEngine {
	class AIManager {
	public:
		AIManager() = default;
		void Update();
	};
}