/*!
@file       collisionutils.h
@author     Shahrin Idzuan
@author     tristan
@author     Ranvitha
@date       29/11/2025
@brief      Collider component to hold custom colliders for entities

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
#pragma once
#include "entitymanager.h"
#include "colliders.h"
#include <utility>

namespace Collision {

	using ContactPair = std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity>;

	struct ContactInformation {
		ContactPair contacts;
		CollisionManifold colManifold;
	};

	using ContactCollection = std::vector<ContactInformation>;

	inline bool isPairInCollection(ContactPair const &pair, ContactCollection const& collection) {
		for (ContactInformation const& info : collection) {
			if (info.contacts == pair) {
				return true;
			}
		}

		return false;
	}


}
