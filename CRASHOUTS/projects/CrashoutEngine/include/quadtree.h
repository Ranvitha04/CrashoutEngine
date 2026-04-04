/*!
@file   quadtree.h
@author tristan 
@date   27/10/2025

@brief  Class declaration for the quadtree data structure, used for partitioning the screen into sections to speed up some processes (Broad phase collision)

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include "entitymanager.h"
#include "colliders.h"
#include <utility>
#include <array>


namespace DataStructure {
	/** @brief QuadTree */
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
		

		int depth;
		bool divided;

		std::vector<std::pair<CrashoutsEngine::Entity, Collision::AABBRect>> objects;

	public:
		Collision::AABBRect boundary; //This current node's size

		QuadTree(Collision::AABBRect const& boundary = Collision::AABBRect{ CEVec3{}, CEVec3{} }, int const& depth = 0);

		QuadTree(QuadTree&&) = default;

		~QuadTree();

		void Insert(CrashoutsEngine::Entity const& entity);

		/*For collision*/
		void Query(Collision::AABBRect& region, std::vector<CrashoutsEngine::Entity>& collection);

		void Clear();

		void Resize(Collision::AABBRect const& newBoundary);

		void Draw();

	};
}
