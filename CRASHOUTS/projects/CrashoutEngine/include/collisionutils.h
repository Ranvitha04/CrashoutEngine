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