/*!
@file   textures.h
@author lee.r@digipen.edu
@date   16/09/2025

@brief	This file contains the declaration of the Texture class, a concrete
		asset type that encapsulates a texture's raw data and dimensions. The class
		is designed to be used with the AssetManager and provides a static factory
		method for safe creation and resource management.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "managers/resourcemanager.h"
#include <stb_image.h>
#include <GL/glew.h>	

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {
	namespace Assets {

		/*!
		@class Texture
		@brief A concrete asset class for managing texture data.

		This class inherits from IAsset and encapsulates the raw pixel data,
		dimensions, and lifetime management of a texture. It uses a private
		constructor and a public static factory method to ensure that all
		Texture objects are created correctly and safely.
		*/
		class Texture : public IAsset {

		using texdata = unsigned char*;

		private:
			int width, height;
			texdata data;

			// Private constructor so only our factory can create it 
			Texture(int w, int h, texdata inputData) :
				width{ w }, height{ h }, data{ inputData } {}

			// The copy constructor and assignment operators are correctly deleted.
			Texture(const Texture&) = delete;
			Texture& operator=(const Texture&) = delete;

			/*!
			@brief Static factory method to create a new Texture asset from a file.

			This function encapsulates all the image loading details, including
			memory allocation and error handling, returning a fully constructed
			Texture object or nullptr if loading fails.

			@param filePath The path to the image file.
			@return A unique_ptr to the new Texture object, or nullptr on failure.
			*/
			static texdata LoadImageData(const std::string& filePath, int& width, int& height) {
				int channels;
				// Use stb_image to load the image data
				texdata data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
				// Force 4 channels (RGBA) for consistency
				return data;
			}

		public:
			/*!
			@brief Static factory method to create a new Texture asset from a file.

			This function loads image data from the specified file path, handling the
			memory allocation and error checking. It then returns a fully constructed
			`Texture` object wrapped in a `std::unique_ptr` to ensure proper resource management.

			@param filePath The path to the image file to load.

			@return A `std::unique_ptr` to the new `Texture` object if loading is
			successful, or `nullptr` if the file cannot be loaded.
			*/
			static std::unique_ptr<Texture> Create(const std::string& filePath) {
				int w, h;
				texdata loadedData = LoadImageData(filePath, w, h);
				if (!loadedData) return nullptr;
				
				return std::unique_ptr<Texture>(new Texture(w, h, loadedData));
			}

			// Destructor
			~Texture() override { stbi_image_free(data); }

			// Accessors for texture data and dimensions
			int GetWidth() const { return width; }
			int GetHeight() const { return height; }
			texdata GetData() const { return data; }

			// Move Constructor for the Assetmanager std::move
			Texture(Texture&& other) noexcept :
				width(other.width), height(other.height), data(other.data) {
				// Nullify the other object 
				other.data = nullptr;
				other.width = 0;
				other.height = 0;
			}

			// Move Assignment Operator
			Texture& operator=(Texture&& other) noexcept{
				if (this != &other) {
					// delete existing data
					stbi_image_free(data);

					// Steal resources
					width = other.width; 
					height = other.height;
					data = other.data;

					// Nullify the other object 
					other.data = nullptr;
					other.width = 0;
					other.height = 0;
				}
				return *this;
			}
		};
	}
}