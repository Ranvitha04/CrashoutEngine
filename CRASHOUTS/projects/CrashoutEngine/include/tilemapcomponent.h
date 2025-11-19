/*!
@file   tilemapcomponent.h
@author lee.r@digipen.edu
@date   13/11/2025

@brief	This file declares the TileMapComponent, which stores reference to a 
		tile-map asset, tile-map grid size, and other required data. 
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
	struct TileMapComponent {
		std::string tmName;					// Name of tile map asset 
		int row{ 15 }, col{ 15 };			// Number of rows and columns for tile
		std::vector<unsigned int> tileIds;	// Represents the '2D' grid 

		// RUNTIME CACHE
		// Maps Tile ID -> OpenGL Texture Handle
		std::map<unsigned int, GLuint> runtimeTextureCache;

		// Helper to check if cache is generated
		bool isCacheDirty = true;
	
		// Dimensions
		int tileSize = 32;

		bool pOpen = false;					// Decide if the editor window is opened

	};
}

namespace CrashoutsEngine {

	namespace TileMapUtils {

		/*!
		@brief	Adds a new texture definition to the underlying ASSET, and marks
				this component's cache as dirty.
		@param  map TileMapComponent to reference.
		@param  textureName Add a new texture to the tile map asset.
		*/
		void AddTileDefinitionToAsset(Component::TileMapComponent& map, const std::string& textureName);

		/*!
		@brief	Removes a definition from the underlying ASSET
				and marks this component's cache as dirty.
		@param  map TileMapComponent to reference.
		@param  tileId The ID of texture asset to remove from tile map asset
		*/
		void RemoveTileDefinitionFromAsset(Component::TileMapComponent& map, unsigned int tileId);

		/*!
		@brief	Rebuilds the GLuint cache (called in Update if dirty)
		@param  map TileMapComponent to reference.
		*/
		void RebuildCache(Component::TileMapComponent& map);

		/*!
		@brief	Resizes the TileID 2D Arrow "MAP" based on new count of rows and cols
		@param  map		TileMapComponent to reference.
		@param  newRows Number of rows.
		@param  newCols	Number of cols.
		*/
		void ResizeMap(Component::TileMapComponent& map, int newRows, int newCols);

	};

	namespace Editor {

		/*!
		@brief	Draws the tile map editor window.
		@param  tmComp	TileMapComponent to reference.
		@param  pOpen	Bool to decide if window is created or not.
		*/
		void DrawTileMapEditor(Component::TileMapComponent* tmComp, bool* pOpen);

	};

};

