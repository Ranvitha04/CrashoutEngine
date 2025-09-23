/*!
@file   textures.h
@author lee.r@digipen.edu
@date   16/09/2025

@brief	This file contains the declaration of the Texture class, a concrete
		asset type that encapsulates a texture's raw data and dimensions. The class
		is designed to be used with the AssetManager and provides a static factory
		method for safe creation and resource management.
//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */
//#include "managers/resourcemanager.h"
//#include <stb_image.h>
//#include <GL/glew.h>	
//
///*
//----------------------------------------------------------------------------- */
//namespace CrashoutsEngine {
//	namespace Assets {
//
//		/*!
//		@class Texture
//		@brief A concrete asset class for managing texture data.
//
//		This class inherits from IAsset and encapsulates the raw pixel data,
//		dimensions, and lifetime management of a texture. It uses a private
//		constructor and a public static factory method to ensure that all
//		Texture objects are created correctly and safely.
//		*/
//		class Texture : public IAsset {
//
//		using texdata = unsigned char*;
//
//		private:
//			int width, height;
//			texdata data;
//
//			// Private constructor so only our factory can create it 
//			Texture(int w, int h, texdata inputData) :
//				width{ w }, height{ h }, data{ inputData } {}
//
//			// The copy constructor and assignment operators are correctly deleted.
//			Texture(const Texture&) = delete;
//			Texture& operator=(const Texture&) = delete;
//
//			/*!
//			@brief Static factory method to create a new Texture asset from a file.
//
//			This function encapsulates all the image loading details, including
//			memory allocation and error handling, returning a fully constructed
//			Texture object or nullptr if loading fails.
//
//			@param filePath The path to the image file.
//			@return A unique_ptr to the new Texture object, or nullptr on failure.
//			*/
//			static texdata LoadImageData(const std::string& filePath, int& width, int& height) {
//				int channels;
//				// Use stb_image to load the image data
//				texdata data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
//				// Force 4 channels (RGBA) for consistency
//				return data;
//			}
//
//		public:
//			/*!
//			@brief Static factory method to create a new Texture asset from a file.
//
//			This function loads image data from the specified file path, handling the
//			memory allocation and error checking. It then returns a fully constructed
//			`Texture` object wrapped in a `std::unique_ptr` to ensure proper resource management.
//
//			@param filePath The path to the image file to load.
//
//			@return A `std::unique_ptr` to the new `Texture` object if loading is
//			successful, or `nullptr` if the file cannot be loaded.
//			*/
//			static std::unique_ptr<Texture> Create(const std::string& filePath) {
//				int w, h;
//				texdata loadedData = LoadImageData(filePath, w, h);
//				if (!loadedData) return nullptr;
//				
//				return std::unique_ptr<Texture>(new Texture(w, h, loadedData));
//			}
//
//			// Destructor
//			~Texture() override { stbi_image_free(data); }
//
//			// Accessors for texture data and dimensions
//			int GetWidth() const { return width; }
//			int GetHeight() const { return height; }
//			texdata GetData() const { return data; }
//
//			// Move Constructor for the Assetmanager std::move
//			Texture(Texture&& other) noexcept :
//				width(other.width), height(other.height), data(other.data) {
//				// Nullify the other object 
//				other.data = nullptr;
//				other.width = 0;
//				other.height = 0;
//			}
//
//			// Move Assignment Operator
//			Texture& operator=(Texture&& other) noexcept{
//				if (this != &other) {
//					// delete existing data
//					stbi_image_free(data);
//
//					// Steal resources
//					width = other.width; 
//					height = other.height;
//					data = other.data;
//
//					// Nullify the other object 
//					other.data = nullptr;
//					other.width = 0;
//					other.height = 0;
//				}
//				return *this;
//			}
//		};
//	}
//}


#ifndef TEXTURES_H
#define TEXTURES_H

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "managers/entitymanager.h"
#include "rendering.h"
#include "math/CEVec2.h"
#include "math/CEVec3.h"

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {
        class IAsset {
        public:
            virtual ~IAsset() = default;
        };

        /*!
        @class Texture
        @brief A concrete asset class for managing texture data.
        This class inherits from IAsset and encapsulates the raw pixel data,
        dimensions, and lifetime management of a texture.
        */
        class Texture : public IAsset {
            using texdata = unsigned char*;

        private:
            int width, height;
            texdata data;

            // Private constructor - only factory method can create instances
            Texture(int w, int h, texdata inputData);

            // Deleted copy operations
            Texture(const Texture&) = delete;
            Texture& operator=(const Texture&) = delete;

            /*!
            @brief Internal method to load image data from file
            @param filePath Path to the image file
            @param width Output parameter for image width
            @param height Output parameter for image height
            @return Pointer to loaded image data or nullptr on failure
            */
            static texdata LoadImageData(const std::string& filePath, int& width, int& height);

        public:
            /*!
            @brief Factory method to create a new Texture asset from file
            @param filePath Path to the image file to load
            @return unique_ptr to Texture object or nullptr on failure
            */
            static std::unique_ptr<Texture> Create(const std::string& filePath);

            // Destructor
            virtual ~Texture();

            // Accessors
            int GetWidth() const { return width; }
            int GetHeight() const { return height; }
            texdata GetData() const { return data; }

            // Move constructor
            Texture(Texture&& other) noexcept;

            // Move assignment operator
            Texture& operator=(Texture&& other) noexcept;
        };
    }
}

namespace Component {
    /*!
    @class Texture
    @brief Component class for efficient texture rendering with caching and shared resources
    */
    class Texture {
    private:
        // Static shared resources for all texture rendering
        static GLuint sharedVAO, sharedVBO, sharedEBO;
        static bool initialized;
        static std::unordered_map<std::string, GLuint> textureCache;

        /*!
        @brief Initialize shared quad geometry for all texture rendering
        */
        static void InitializeSharedQuad();

        /*!
        @brief Load texture from binary file (.tex format)
        @param pathname Path to binary texture file
        @return OpenGL texture handle or 0 on failure
        */
        static GLuint LoadBinaryTexture(const std::string& pathname);

        /*!
        @brief Load texture using the asset system
        @param pathname Path to texture file
        @return OpenGL texture handle or 0 on failure
        */
        static GLuint LoadTextureFromAsset(const std::string& pathname);

        /*!
        @brief Determine texture format and load appropriately
        @param pathname Path to texture file
        @return OpenGL texture handle or 0 on failure
        */
        static GLuint LoadTexture(const std::string& pathname);

        /*!
        @brief Core rendering function for textured quads
        */
        static void RenderQuad(GLuint textureHandle, const CEVec3& position, const CEVec2& size,
            float rotation, float scale, const CEVec3& tint, bool modulate);

    public:
        /*!
        @brief Initialize the texture system
        */
        static void Initialize();

        /*!
        @brief Get or load texture with caching
        @param texturePath Path to texture file
        @return OpenGL texture handle or 0 on failure
        */
        static GLuint GetOrLoadTexture(const std::string& texturePath);

        /*!
        @brief Create and render a textured quad from file path
        */
        static void CreateTexturedQuad(const std::string& texturePath, const CEVec3& position,
            const CEVec2& size, float rotation = 0.0f, float scale = 1.0f,
            const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f), bool modulate = false);

        /*!
        @brief Create and render a textured quad from existing texture handle
        */
        static void CreateTexturedQuad(GLuint textureHandle, const CEVec3& position,
            const CEVec2& size, float rotation = 0.0f, float scale = 1.0f,
            const CEVec3& tint = CEVec3(1.0f, 1.0f, 1.0f), bool modulate = false);

        /*!
        @brief Preload a texture into cache
        @param texturePath Path to texture file
        @return true if successful, false otherwise
        */
        static bool PreloadTexture(const std::string& texturePath);

        /*!
        @brief Unload a specific texture from cache
        @param texturePath Path to texture to unload
        */
        static void UnloadTexture(const std::string& texturePath);

        /*!
        @brief Clear all cached textures
        */
        static void ClearTextureCache();

        /*!
        @brief Clean up all texture system resources
        */
        static void Cleanup();
    };

} // namespace Component

#endif // TEXTURE_H