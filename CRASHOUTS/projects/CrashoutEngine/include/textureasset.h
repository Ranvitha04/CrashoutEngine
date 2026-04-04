/*!
@file   textureasset.h
@author Shahrin Idzuan (46%), Ranvitha (24%), Renfred Lee (30%)
@date   16/09/2025

@brief  This file contains the declaration of the TextureAsset class, a concrete
		asset type that encapsulates a texture's raw data and dimensions. The class
		is designed to be used with the AssetManager and provides a static factory
		method for safe creation and resource management.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <array>
#include "assetmanager.h"
#include "entitymanager.h"
#include "CEVec2.h"
#include "CEVec3.h"
#include "CEColor.h"


// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

        enum class TextureMode {
            ClampToEdge,
            Repeat,
            Mirror
        };

        /** @brief TextureAsset */
        class TextureAsset : public IAsset {
        public:
            using texdata = unsigned char*;

            // Public members (matching implementation)
            int width, height;
            texdata data;
            GLuint glHandle;
            bool uploaded;

            // Constructor (matching implementation pattern)
            TextureAsset(int w, int h, texdata inputData);

            // Deleted copy operations
            TextureAsset(const TextureAsset&) = delete;
            TextureAsset& operator=(const TextureAsset&) = delete;

            // Move constructor and assignment
            TextureAsset(TextureAsset&& other) noexcept;
            TextureAsset& operator=(TextureAsset&& other) noexcept;

            // Virtual destructor
            virtual ~TextureAsset();

            /*!
            @brief Internal method to load image data from file
            @param filePath Path to the image file
            @param width Output parameter for image width
            @param height Output parameter for image height
            @return Pointer to loaded image data or nullptr on failure
            */
            static texdata LoadImageData(const std::string& filePath, int& width, int& height);

            /*!
            @brief Factory method to create a new Asset from file
            @param filePath Path to the image file to load
            @return unique_ptr to TextureAsset object or nullptr on failure
            */
            static std::unique_ptr<TextureAsset> Create(const std::string& filePath);

            // Helper function to get TextureAsset mode
            static GLuint GetTextureMode(TextureMode mode);

            // Function to set texture mode at runtime
            void SetTextureMode(TextureMode modeS, TextureMode modeT);

            // GPU management methods
            bool UploadToGPU();
            void FreeGPU();

            // Accessors
            int GetWidth() const { return width; }
            int GetHeight() const { return height; }
            texdata GetData() const { return data; }
            GLuint GetGLHandle() const { return glHandle; }
            bool IsUploaded() const { return uploaded; }

        private:
            TextureMode currentModeS = TextureMode::ClampToEdge;
            TextureMode currentModeT = TextureMode::ClampToEdge;

        };

        // Backward-compatibility alias so old code using 'Texture' still compiles
        using Texture = TextureAsset;

    }
}


namespace Component {
    // Constants used in the implementation
    constexpr int VERTEX_SIZE = 8;  // 3 pos + 3 color + 2 texcoord
    constexpr int INDICES_COUNT = 6;

    // Default quad vertices (position, color, texcoord)
    constexpr std::array<float, 32> DEFAULT_VERTICES = {
        // positions        // colors         // texture coords
        -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f, // bottom left
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f, // bottom right
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // top right
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f  // top left
    };

    constexpr std::array<unsigned short, 6> DEFAULT_INDICES = {
        0, 1, 2,   // first triangle
        2, 3, 0    // second triangle
    };

    /*!
    @class TextureRenderer
    @brief Component class for efficient texture rendering with caching and shared resources
    */
    /** @brief TextureRenderer */
    class TextureRenderer {
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
        //static GLuint LoadBinaryTexture(const std::string& pathname);

        /*!
        @brief Load texture using the asset system (for image files)
        @param pathname Path to texture file
        @return OpenGL texture handle or 0 on failure
        */
        //static GLuint LoadTextureFromAsset(const std::string& pathname);

        /*!
        @brief Load image texture using stb_image (renamed for clarity)
        @param pathname Path to texture file
        @return OpenGL texture handle or 0 on failure
        */
        //static GLuint LoadImageTexture(const std::string& pathname);

        /*!
        @brief Determine texture format and load appropriately
        @param pathname Path to texture file
        @return OpenGL texture handle or 0 on failure
        */
        //static GLuint LoadTexture(const std::string& pathname);

        /*!
        @brief Core rendering function for textured quads
        */
        static void RenderQuad(GLuint textureHandle, const CEVec3& position, const CEVec2& size,
            float rotation, float scale, const CEColor& tint, bool modulate);

        /*!
        @brief Core rendering function for textured quads with custom UV coordinates
        */
        static void RenderQuadWithUV(GLuint textureHandle, const CEVec3& position, const CEVec2& size,
            float rotation, float scale, const CEColor& tint, bool modulate,
            float u1, float v1, float u2, float v2);

        /*!
        @brief Calculate UV coordinates for a sprite sheet frame
        @param frameX Frame column (0-based)
        @param frameY Frame row (0-based)
        @param totalCols Total columns in sprite sheet
        @param totalRows Total rows in sprite sheet
        @param u1, v1, u2, v2 Output UV coordinates
        */
        static void CalculateFrameUV(int frameX, int frameY, int totalCols, int totalRows,
            float& u1, float& v1, float& u2, float& v2);

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
        //static GLuint GetOrLoadTexture(const std::string& texturePath);

        /*!
        @brief Retrieves a cached texture handle using its asset name.
        @param assetName The unique name of the asset (from the manifest).
        @return OpenGL texture handle, or 0 on failure.
        */
        static GLuint GetTextureHandle(std::string const& assetName);

        /*!
        @brief Create and render a textured quad from asset name
        */
        static void CreateTexturedQuad(const std::string& textureName, const CEVec3& position,
            const CEVec2& size, float rotation = 0.0f, float scale = 1.0f,
            const CEColor& tint = CEColor(1.0f, 1.0f, 1.0f), bool modulate = false);

        /*!
        @brief Create and render a textured quad from existing texture handle
        */
        static void CreateTexturedQuad(GLuint textureHandle, const CEVec3& position,
            const CEVec2& size, float rotation = 0.0f, float scale = 1.0f,
            const CEColor& tint = CEColor(1.0f, 1.0f, 1.0f), bool modulate = false);

        /*!
        @brief Create and render a sprite sheet frame from file path
        */
        static void CreateSpriteFrame(const std::string& texturePath, const CEVec3& position,
            const CEVec2& size, int frameX, int frameY, int totalCols, int totalRows,
            float rotation = 0.0f, float scale = 1.0f,
            const CEColor& tint = CEColor(1.0f, 1.0f, 1.0f), bool modulate = false);

        /*!
        @brief Create and render a sprite sheet frame using existing texture handle
        */
        static void CreateSpriteFrame(GLuint textureHandle, const CEVec3& position,
            const CEVec2& size, int frameX, int frameY, int totalCols, int totalRows,
            float rotation = 0.0f, float scale = 1.0f,
            const CEColor& tint = CEColor(1.0f, 1.0f, 1.0f), bool modulate = false);

        /*!
        @brief Create and render a sprite sheet frame with horizontal flipping support
        */
        static void CreateSpriteFrameFlipped(GLuint textureHandle, const CEVec3& position,
            const CEVec2& size, int frameX, int frameY, int totalCols, int totalRows,
            bool flipHorizontal, float rotation = 0.0f, float scale = 1.0f,
            const CEColor& tint = CEColor(1.0f, 1.0f, 1.0f), bool modulate = false);

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

#endif // TEXTURES_H