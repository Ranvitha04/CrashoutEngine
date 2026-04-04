/*!
@file   tilemapcomponent.h
@author Renfred Lee
@author Renfred (RTIS)
@author Shahrin Idzuan
@date   13/11/2025

@brief	This file declares the TileMapComponent, which stores reference to a 
		tile-map asset, tile-map grid size, and other required data. 

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <GLFW/glfw3.h>
#include "CEVec2.h"
#include <vector>
#include <string>
#include <map>

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

	/*!
	@struct	TileMapComponent
	@brief	A component that provides a some data for use with Tile-Maps in 
		    the scene.
	*/
	/** @brief TileMapComponent */
	struct TileMapComponent {
		std::vector<std::string> tileAssetNames;				// Vector of names of the Tile Assets to be used
		std::vector<CrashoutsEngine::Entity> segmentCache;						// For optimized LineSegments
		std::string activeAsset;								// The currently active / selected Tile Asset
		int tileSize = 32;										// Size of one tile in world units
		bool pOpen = false;										// Toggle editor window
		bool excludeOuterBoundary = false;						// Skip collider generation on outer boundary edges
		bool lightPerTile = true;// Apply tilemap light to each tile
		float emissiveBoost = 0.0f;// Boost tile brightness without lights

		// --- DATA (THE SPARSE GRID) ---
		// Key:   Pair of (GridX, GridY)
		// Value: The Tile data (Which tile map asset is painted here using rules)
		std::map<std::pair<int, int>, std::string> gridData;

		// This maps (GridX, GridY) -> TextureID to render
		std::map<std::pair<int, int>, GLuint> renderCache;

		// Stores Entity IDs for Colliders. 
		// We need this so if we erase a tile, we know which Entity to Destroy.
		std::map < std::pair<int, int>, uint32_t> tileCache;

		// --- RUNTIME CACHE ---
		// To avoid recalculating rules every frame, we mark this dirty when painting happens.
		bool isCacheDirty = true;

	};
}

namespace CrashoutsEngine {

	namespace TileMapUtils {

		/*!
		@brief  Converts World Mouse Coordinates to Grid Coordinates.
		@param  map TileMapComponent to reference.
		@param  worldX	The provided worldX position.
		@param  worldY	The provided worldY position.
		*/
		std::pair<int, int> WorldToGrid(const Component::TileMapComponent& map, float worldX, float worldY);

		/*!
		@brief  Paints a tile at the specific grid location.
				Automatically handles overwriting or new insertions.
		@param  map TileMapComponent to reference.
		@param  gridX		The provided gridX position.
		@param  gridY		The provided gridY position.
		@param	assetName	Texture Asset Name at location.
		*/
		void PaintTile(Component::TileMapComponent& map, int gridX, int gridY, const std::string& assetName);

		/*!
		@brief  Removes a tile at the specific grid location (Right Click).
		@param  map TileMapComponent to reference.
		@param  gridX		The provided gridX position.
		@param  gridY		The provided gridY position.
		*/
		void EraseTile(Component::TileMapComponent& map, int gridX, int gridY);

		/*!
		@brief  Iterates through the gridData and recalculates the Rule Bitmasks
				for every tile to Update the 'renderCache'.
		@param  map TileMapComponent to reference.
		*/
		void RebuildCache(Component::TileMapComponent& map, const Entity& parentEnt);

	};

	namespace Editor {

		/*!
		@brief	Draws the tile map asset selector.
		@param  tmComp	TileMapComponent to reference.
		@param  pOpen	Bool to decide if window is created or not.
		*/
		void DrawTileMapEditor(Component::TileMapComponent* tmComp, bool* pOpen);

	};

	namespace Rendering {

		/*!
		@brief	Renders all the tilemaps.
		@param  ecs EntityComponentSystem to reference.
		*/
		//void RenderTileMaps(ECS& ecs);
	};

};

