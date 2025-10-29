#pragma once

#include "CEVec3.h"
#include "entitymanager.h"
#include "colliders.h"
#include <utility>
#include <array>


namespace DataStructure {
	class QuadTree {
		enum
		{
			NORTHEAST,
			NORTHWEST,
			SOUTHWEST,
			SOUTHEAST
		};

		static constexpr int maxDepth{ 6 };

		std::array<std::unique_ptr<QuadTree>, 4> childRegions;
		std::array<Collision::AABBRect, 4> childSizes;
		Collision::AABBRect boundary; //This current node's size

		int depth;
		bool divided;

		std::vector<std::pair<CrashoutsEngine::Entity, Collision::AABBRect>> objects;

	public:

		QuadTree(Collision::AABBRect const& boundary, int const& depth = 0);

		void Insert(CrashoutsEngine::Entity const& entity);

		/*For collision*/
		void Query(Collision::AABBRect& region, std::vector<CrashoutsEngine::Entity>& collection);

		void Clear();

		void Resize(Collision::AABBRect const& newBoundary);

		void Draw();

	};
}
