/*!
@file        textureasset.cpp
@author      Ranvitha (60%)
@author      Renfred Lee (40%)
@author      Shahrin Idzuan

@brief
This file implements the texture management and rendering functionality
for CrashoutsEngine. It contains:

- **CrashoutsEngine::Assets::TextureAsset**:
  Manages raw image data, GPU upload, and cleanup. Provides static helpers
  to load image data (via stb_image) and create asset objects.
- **Component::TextureRenderer**:
  Manages shared OpenGL quad geometry, texture caching, and efficient
  rendering of textured quads. Supports loading from multiple formats
  (.png, .jpg, .bmp, .tga, .gif, .tex) and maintains a global cache to
  avoid reloading the same texture.

The design emphasizes resource reuse, GPU efficiency (shared VAO/VBO/EBO),
and centralized texture lifecycle management.

Usage:
- Call `TextureRenderer::CreateTexturedQuad()` to draw textured quads.
- Call `TextureRenderer::PreloadTexture()` to load textures in advance.
- Use `TextureAsset::Create()` for lower-level texture asset creation.

Clean-up functions are provided to release GPU resources and clear caches
when shutting down the engine.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#define STB_IMAGE_IMPLEMENTATION
// includes section
#include "textureasset.h"
#include "entitymanager.h"
#include "glapp.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>  // Add this include
//#include <stb_image.h>
#include "rendering.h"
#include "stb_image.h"

namespace CrashoutsEngine {
    namespace Assets {

        /*!
        @brief Constructs an TextureAsset with given width, height, and raw pixel data.
        @param w      Width of the texture in pixels.
        @param h      Height of the texture in pixels.
        @param inputData Raw pointer to the texture data (RGBA).
        */

        TextureAsset::TextureAsset(int w, int h, texdata inputData) :
            width(w), height(h), data(inputData), glHandle(0), uploaded(false) {
        }

        /*!
        @brief Loads image data from file using stb_image.
        @param filePath Path to the image file.
        @param width    Output reference for image width.
        @param height   Output reference for image height.
        @return Raw pixel data pointer (RGBA), or nullptr on failure.
        */


        TextureAsset::texdata TextureAsset::LoadImageData(const std::string& filePath, int& width, int& height) {

            int channels;

            texdata data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (!data) {
                std::cerr << "Error loading image: " << filePath << " - " << stbi_failure_reason() << std::endl;
                return nullptr;
            }

            const int MAX_DIM = 4096;
            if (width > MAX_DIM || height > MAX_DIM) {
                std::cout << "FAILED: oversize image load (" << width << "x" << height << ") -> " << filePath << std::endl;
            }

            return data;
        }

        /*!
        @brief Creates a new TextureAsset from an image file.
        @param filePath Path to the image file.
        @return Unique pointer to a valid TextureAsset, or nullptr on failure.
        */

        std::unique_ptr<TextureAsset> TextureAsset::Create(const std::string& filePath) {
            int w, h;
            texdata loadedData = LoadImageData(filePath, w, h);
            if (!loadedData) {
                return nullptr;
            }

            const int MAX_DIM = 4096;
            if (w > MAX_DIM || h > MAX_DIM) {
                std::cerr << "Error: Texture exceeds 4k limit (" << w << "x" << h << ") -> " << filePath << std::endl;
                std::cout << "FAILED: oversize texture (" << w << "x" << h << ") -> " << filePath << std::endl;
                stbi_image_free(loadedData);
                return nullptr;
            }

            return std::unique_ptr<TextureAsset>(new TextureAsset(w, h, loadedData));
        }

        // Helper function to get TextureAsset mode
        GLuint TextureAsset::GetTextureMode(TextureMode mode) {
            switch (mode) {
            case TextureMode::Repeat:
                return GL_REPEAT;
            case TextureMode::Mirror:
                return GL_MIRRORED_REPEAT;
            case TextureMode::ClampToEdge:
                return GL_CLAMP_TO_EDGE;
            default:
                return GL_CLAMP_TO_EDGE;
            }
        }

        // Function to set texture mode at runtime
        void TextureAsset::SetTextureMode(TextureMode modeS, TextureMode modeT) {
            if (glHandle == 0) {
                std::cout << "Warning: Cannot set wrap mode on uninitialized texture." << std::endl;
                return;
            }

            // Apply the new modes using the Direct State Access function
            if (currentModeS != modeS) {
                glTextureParameteri(glHandle, GL_TEXTURE_WRAP_S, GetTextureMode(modeS));
                currentModeS = modeS;
            }

            // Compare to modeT, not modeS, so T-wrap updates correctly
            if (currentModeT != modeT) {
                glTextureParameteri(glHandle, GL_TEXTURE_WRAP_T, GetTextureMode(modeT));
                currentModeT = modeT;
            }
        }

        /*!
        @brief Destructor. Frees GPU resources and image memory.
        */

        TextureAsset::~TextureAsset() {
            FreeGPU();
            if (data) {
                stbi_image_free(data);
                data = nullptr;
            }
        }

        /*!
        @brief Move constructor. Transfers ownership of resources.
        */

        TextureAsset::TextureAsset(TextureAsset&& other) noexcept :
            width(other.width),
            height(other.height),
            data(other.data),
            glHandle(other.glHandle),
            uploaded(other.uploaded) {

            // Nullify the other object
            other.data = nullptr;
            other.width = 0;
            other.height = 0;
            other.glHandle = 0;
            other.uploaded = false;
        }

        /*!
        @brief Move assignment operator. Cleans up current resources and transfers ownership.
        */

        TextureAsset& TextureAsset::operator=(TextureAsset&& other) noexcept {
            if (this != &other) {
                // Clean up existing resources
                FreeGPU();
                if (data) {
                    stbi_image_free(data);
                }

                // Move resources
                width = other.width;
                height = other.height;
                data = other.data;
                glHandle = other.glHandle;
                uploaded = other.uploaded;

                // Nullify the other object
                other.data = nullptr;
                other.width = 0;
                other.height = 0;
                other.glHandle = 0;
                other.uploaded = false;
            }
            return *this;
        }

        /*!
        @brief Uploads the image data to the GPU as an OpenGL texture.
        @return True if successfully uploaded, false otherwise.
        */

        bool TextureAsset::UploadToGPU() {
            if (uploaded || !data) {
                return uploaded; // Already uploaded or no data
            }

            const int MAX_DIM = 4096;
            if (width > MAX_DIM || height > MAX_DIM) {
                std::cerr << "Error: Texture exceeds 4k limit (" << width << "x" << height << ")" << std::endl;
                std::cout << "FAILED: oversize texture upload (" << width << "x" << height << ")" << std::endl;
                return false;
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &glHandle);
            if (glHandle == 0) {
                std::cerr << "Failed to create OpenGL texture" << std::endl;
                return false;
            }

            glTextureStorage2D(glHandle, 1, GL_RGBA8, width, height);
            glTextureSubImage2D(glHandle, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

            // Set texture parameters for pixel-perfect sprite rendering
            glTextureParameteri(glHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(glHandle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(glHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(glHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            uploaded = true;
            return true;
        }

        /*!
        @brief Frees the GPU texture resource if allocated.
        */

        void TextureAsset::FreeGPU() {
            if (uploaded && glHandle != 0) {
                glDeleteTextures(1, &glHandle);
                glHandle = 0;
                uploaded = false;
            }
        }

    } // namespace Assets

}

namespace Component {

    /*                                                    Static member definitions
    ----------------------------------------------------------------------------- */
    std::unordered_map<std::string, GLuint> TextureRenderer::textureCache;
    GLuint TextureRenderer::sharedVAO = 0;
    GLuint TextureRenderer::sharedVBO = 0;
    GLuint TextureRenderer::sharedEBO = 0;
    bool TextureRenderer::initialized = false;

    /*                                                    TextureRenderer implementation
    ----------------------------------------------------------------------------- */

    /*!
    @brief Initializes the TextureRenderer shared quad and cache if not already done.
    */

    void TextureRenderer::Initialize() {
        if (initialized) return;

        InitializeSharedQuad();
        initialized = true;

        std::cout << "TextureRenderer initialized with shared resources." << std::endl;
    }

    /*!
    @brief Creates shared VAO/VBO/EBO for rendering quads. Called internally by Initialize().
    */

    void TextureRenderer::InitializeSharedQuad() {
        // Generate and bind VAO (reused for all texture rendering)
        glGenVertexArrays(1, &sharedVAO);
        glBindVertexArray(sharedVAO);

        // Generate and bind VBO
        glGenBuffers(1, &sharedVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(DEFAULT_VERTICES), DEFAULT_VERTICES.data(), GL_STATIC_DRAW);

        // Generate and bind EBO
        glGenBuffers(1, &sharedEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(DEFAULT_INDICES), DEFAULT_INDICES.data(), GL_STATIC_DRAW);

        // Position attribute (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // TextureAsset coordinate attribute (location 2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    /*!
    @brief Creates and renders a textured quad from a file path.
    @param texturePath Path to the texture file.
    @param position    3D position of the quad.
    @param size        2D size of the quad.
    @param rotation    Rotation in degrees (around Z axis).
    @param scale       Scale factor applied to size.
    @param tint        RGB tint applied to texture.
    @param modulate    Whether to modulate with vertex color.
    */

    void TextureRenderer::CreateTexturedQuad(const std::string& texturePath,
        const CEVec3& position,
        const CEVec2& size,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate) {

        // Auto-initialize if needed
        if (!initialized) {
            Initialize();
        }

        // Get or load texture (uses cache)
        //GLuint textureHandle = GetOrLoadTexture(texturePath);
        GLuint textureHandle = GetTextureHandle(texturePath);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }

        // Render using the cached texture
        RenderQuad(textureHandle, position, size, rotation, scale, tint, modulate);
    }

    /*!
    @brief Creates and renders a sprite sheet frame with horizontal flipping support.
    @param textureHandle OpenGL texture ID.
    @param position      3D position of the sprite.
    @param size          2D size of the sprite.
    @param frameX        Frame column (0-based).
    @param frameY        Frame row (0-based).
    @param totalCols     Total columns in sprite sheet.
    @param totalRows     Total rows in sprite sheet.
    @param flipHorizontal Whether to flip the sprite horizontally.
    @param rotation      Rotation in degrees (around Z axis).
    @param scale         Scale factor applied to size.
    @param tint          RGB tint applied to texture.
    @param modulate      Whether to modulate with vertex color.
    */
    void TextureRenderer::CreateSpriteFrameFlipped(GLuint textureHandle,
        const CEVec3& position,
        const CEVec2& size,
        int frameX, int frameY,
        int totalCols, int totalRows,
        bool flipHorizontal,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate) {

        // Auto-initialize if needed
        if (!initialized) {
            Initialize();
        }

        if (textureHandle == 0) {
            std::cerr << "Invalid texture handle!" << std::endl;
            return;
        }

        // Calculate UV coordinates for the specified frame
        float u1, v1, u2, v2;
        CalculateFrameUV(frameX, frameY, totalCols, totalRows, u1, v1, u2, v2);

        // Flip horizontally by swapping u1 and u2 if needed
        if (flipHorizontal) {
            float temp = u1;
            u1 = u2;
            u2 = temp;
        }

        // Render the quad with custom UV coordinates
        RenderQuadWithUV(textureHandle, position, size, rotation, scale, tint, modulate, u1, v1, u2, v2);
    }

    /*!
    @brief Creates and renders a sprite sheet frame from a file path.
    @param texturePath   Path to the sprite sheet texture file.
    @param position      3D position of the sprite.
    @param size          2D size of the sprite.
    @param frameX        Frame column (0-based).
    @param frameY        Frame row (0-based).
    @param totalCols     Total columns in sprite sheet.
    @param totalRows     Total rows in sprite sheet.
    @param rotation      Rotation in degrees (around Z axis).
    @param scale         Scale factor applied to size.
    @param tint          RGB tint applied to texture.
    @param modulate      Whether to modulate with vertex color.
    */
    void TextureRenderer::CreateSpriteFrame(const std::string& texturePath,
        const CEVec3& position,
        const CEVec2& size,
        int frameX, int frameY,
        int totalCols, int totalRows,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate) {

        //GLuint textureHandle = GetOrLoadTexture(texturePath);
        GLuint textureHandle = GetTextureHandle(texturePath);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }

        CreateSpriteFrame(textureHandle, position, size, frameX, frameY, totalCols, totalRows, rotation, scale, tint, modulate);
    }

    /*!
    @brief Creates and renders a sprite sheet frame using an existing OpenGL texture handle.
    @param textureHandle OpenGL texture ID.
    @param position      3D position of the sprite.
    @param size          2D size of the sprite.
    @param frameX        Frame column (0-based).
    @param frameY        Frame row (0-based).
    @param totalCols     Total columns in sprite sheet.
    @param totalRows     Total rows in sprite sheet.
    @param rotation      Rotation in degrees (around Z axis).
    @param scale         Scale factor applied to size.
    @param tint          RGB tint applied to texture.
    @param modulate      Whether to modulate with vertex color.
    */
    void TextureRenderer::CreateSpriteFrame(GLuint textureHandle,
        const CEVec3& position,
        const CEVec2& size,
        int frameX, int frameY,
        int totalCols, int totalRows,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate) {

        // Auto-initialize if needed
        if (!initialized) {
            Initialize();
        }

        if (textureHandle == 0) {
            std::cerr << "Invalid texture handle!" << std::endl;
            return;
        }

        // Calculate UV coordinates for the specified frame
        float u1, v1, u2, v2;
        CalculateFrameUV(frameX, frameY, totalCols, totalRows, u1, v1, u2, v2);

        // Render the quad with custom UV coordinates
        RenderQuadWithUV(textureHandle, position, size, rotation, scale, tint, modulate, u1, v1, u2, v2);
    }

    /*!
    @brief Creates and renders a textured quad using an existing OpenGL texture handle.
    @param textureHandle OpenGL texture ID.
    @param position      3D position of the quad.
    @param size          2D size of the quad.
    @param rotation      Rotation in degrees (around Z axis).
    @param scale         Scale factor applied to size.
    @param tint          RGB tint applied to texture.
    @param modulate      Whether to modulate with vertex color.
    */

    void TextureRenderer::CreateTexturedQuad(GLuint textureHandle,
        const CEVec3& position,
        const CEVec2& size,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate) {

        // Auto-initialize if needed
        if (!initialized) {
            Initialize();
        }

        if (textureHandle == 0) {
            std::cerr << "Invalid texture handle!" << std::endl;
            return;
        }

        RenderQuad(textureHandle, position, size, rotation, scale, tint, modulate);
    }

    /*!
    @brief Retrieves a cached texture handle or loads it if not cached.
    @param texturePath Path to the texture file.
    @return OpenGL texture handle, or 0 on failure.
    */


    //GLuint TextureRenderer::GetOrLoadTexture(const std::string& texturePath) {
    //    // Check cache first
    //    auto it = textureCache.find(texturePath);
    //    if (it != textureCache.end()) {
    //        return it->second; // Return cached texture
    //    }

    /*!
    @brief Retrieves a cached texture handle using its asset name.
    @param assetName The unique name of the asset (from the manifest).
    @return OpenGL texture handle, or 0 on failure.
    */
    // method: TextureRenderer::GetTextureHandle
    GLuint TextureRenderer::GetTextureHandle(std::string const& assetName) {
        using namespace CrashoutsEngine::Assets;

        // QUESTION: Can this be checked so we don't keep searching for handle if already present?
        // std::cout << "DEBUG: Requesting texture asset: '" << assetName << "'" << std::endl;

        // Normalize name to manifest key if a path/ext was provided
        std::string key = assetName;
        if (key.find('/') != std::string::npos || key.find('\\') != std::string::npos) {
            std::filesystem::path p(key);
            key = p.stem().string();
        }
        else {
            size_t dot = key.find_last_of('.');
            if (dot != std::string::npos) {
                key = key.substr(0, dot);
            }
        }

        // Try manifest lookup first
        TextureAsset* texture = AssetManager::GetInstance().Get<TextureAsset>(key);
        if (texture) {
            if (texture->IsReady()) {
                return texture->GetGLHandle();
            }
            else if (texture->GetState() == AssetState::Loading) {
                // Asset is enqueued but not yet finished GPU upload
                // Return 0 so it doesn't render, but don't spam errors
                return 0;
            }
            else if (texture->GetState() == AssetState::Failed) {
                std::cerr << "Error: TextureAsset '" << key << "' failed to load." << std::endl;
                return 0;
            }
            
            // If it's Unloaded but exists as an object, it might be in transition
            if (texture->IsUploaded()) {
                return texture->GetGLHandle();
            }
            return 0;
        }

        // Fallback: treat input as a file path and load directly if the file exists
        std::filesystem::path maybePath(assetName);
        if (std::filesystem::exists(maybePath)) {
            auto newTexture = TextureAsset::Create(maybePath.string());
            if (newTexture) {
                newTexture->UploadToGPU();
                AssetManager::GetInstance().Load(key, std::move(newTexture));
                TextureAsset* loaded = AssetManager::GetInstance().Get<TextureAsset>(key);
                if (loaded && loaded->IsUploaded()) {
                    std::cout << "INFO: Loaded texture via path fallback as asset '" << key << "'" << std::endl;
                    return loaded->GetGLHandle();
                }
            }
            std::cerr << "Error: Failed to load texture from path: " << maybePath.string() << std::endl;
            return 0;
        }

        std::cerr << "Error: Texture asset not found: " << key << std::endl;
        std::cerr << "Tip: Place images in 'assets/images' and ensure manifest contains their names." << std::endl;
        return 0;
    }

    //    // Load new texture
    //    GLuint textureHandle = LoadTexture(texturePath);
    //    if (textureHandle != 0) {
    //        // Cache the texture
    //        textureCache[texturePath] = textureHandle;
    //        std::cout << "Cached texture: " << texturePath << std::endl;
    //    }

    //    return textureHandle;
    //}

    /*!
    @brief Loads and caches a texture in advance.
    @param texturePath Path to the texture file.
    @return True if successfully loaded, false otherwise.
    */

    bool TextureRenderer::PreloadTexture(const std::string& assetName) {
        // Check if already loaded
        /*if (textureCache.find(texturePath) != textureCache.end()) {
            return true;
        }

        GLuint textureHandle = LoadTexture(texturePath);
        if (textureHandle != 0) {
            textureCache[texturePath] = textureHandle;
            std::cout << "Preloaded texture: " << texturePath << std::endl;
            return true;
        }

        return false;*/
        return GetTextureHandle(assetName) != 0;
    }

    /*!
    @brief Renders a quad with the given texture and transformation parameters.
    @param textureHandle OpenGL texture ID.
    @param position      3D position of the quad.
    @param size          2D size of the quad.
    @param rotation      Rotation in degrees (around Z axis).
    @param scale         Scale factor applied to size.
    @param tint          RGB tint applied to texture.
    @param modulate      Whether to modulate with vertex color.
    */

    void TextureRenderer::RenderQuad(GLuint textureHandle,
        const CEVec3& position,
        const CEVec2& size,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate) {

        // Get the rendering instance and use shader
        Rendering& renderer = Rendering::Get();
        renderer.shdr_pgm->Use();

        // Setup transformation matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(size.x * scale, size.y * scale, 1.0f));

        // 1. Get the current camera matrices from the renderer
        glm::mat4 view = renderer.viewMatrix;
        glm::mat4 projection = renderer.projectionMatrix;

        // 2. Get the uniform locations
        GLint model_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uModel");
        GLint view_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uView");
        GLint proj_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uProjection");

        // 3. Send all three matrices to the shader
        if (model_loc != -1) {
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        }
        if (view_loc != -1) {
            glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        }
        if (proj_loc != -1) {
            glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        // Enable texture usage in shader
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) {
            glUniform1i(useTexture_loc, 1); // Enable texture
        }

        // Set texture sampler
        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) {
            glUniform1i(tex_loc, 6);
        }

        // Set modulation (whether to multiply by vertex color)
        GLint modulate_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_modulate");
        if (modulate_loc != -1) {
            glUniform1i(modulate_loc, modulate ? 1 : 0);
        }

        // Set tint color
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) {
            glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);
        }

        // Bind texture to texture unit 6
        glBindTextureUnit(6, textureHandle);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        // Use shared VAO for rendering (no buffer creation/deletion per call)
        glBindVertexArray(sharedVAO);
        glDrawElements(GL_TRIANGLES, INDICES_COUNT, GL_UNSIGNED_SHORT, 0);
        renderer.drawCalls++;
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);

        // Unbind texture and shader
        glBindTextureUnit(6, 0);
        renderer.shdr_pgm->UnUse();
    }

    /*!
    @brief Renders a quad with custom UV coordinates for sprite sheets.
    @param textureHandle OpenGL texture ID.
    @param position      3D position of the quad.
    @param size          2D size of the quad.
    @param rotation      Rotation in degrees (around Z axis).
    @param scale         Scale factor applied to size.
    @param tint          RGB tint applied to texture.
    @param modulate      Whether to modulate with vertex color.
    @param u1, v1, u2, v2 Custom UV coordinates.
    */
    void TextureRenderer::RenderQuadWithUV(GLuint textureHandle,
        const CEVec3& position,
        const CEVec2& size,
        float rotation,
        float scale,
        const CEColor& tint,
        bool modulate,
        float u1, float v1, float u2, float v2) {

        // Create dynamic vertex data with custom UV coordinates
        float vertices[] = {
            // positions        // colors         // texture coords
            -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  u1, v2, // bottom left
             0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  u2, v2, // bottom right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  u2, v1, // top right
            -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  u1, v1  // top left
        };

        // Use shared VAO/VBO instead of creating temporary ones
        glBindVertexArray(sharedVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);

        // Update the shared VBO with new vertex data
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        // Bind the shared EBO (already bound with VAO)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);

        // Get the rendering instance and use shader
        Rendering& renderer = Rendering::Get();
        renderer.shdr_pgm->Use();

        // Setup transformation matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(size.x * scale, size.y * scale, 1.0f));

        // 1. Get the current camera matrices from the renderer
        glm::mat4 view = renderer.viewMatrix;
        glm::mat4 projection = renderer.projectionMatrix;

        // 2. Get the uniform locations
        GLint model_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uModel");
        GLint view_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uView");
        GLint proj_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uProjection");

        // 3. Send all three matrices to the shader
        if (model_loc != -1) {
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        }
        if (view_loc != -1) {
            glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        }
        if (proj_loc != -1) {
            glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        // Enable texture usage in shader
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) {
            glUniform1i(useTexture_loc, 1); // Enable texture
        }

        // Set texture sampler
        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) {
            glUniform1i(tex_loc, 6);
        }

        // Set modulation (whether to multiply by vertex color)
        GLint modulate_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_modulate");
        if (modulate_loc != -1) {
            glUniform1i(modulate_loc, modulate ? 1 : 0);
        }

        // Set tint color
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) {
            glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);
        }

        // Bind texture to texture unit 6
        glBindTextureUnit(6, textureHandle);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        // Render the quad
        glDrawElements(GL_TRIANGLES, INDICES_COUNT, GL_UNSIGNED_SHORT, 0);
        renderer.drawCalls++;

        // Unbind shared resources
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);

        // Unbind texture and shader
        glBindTextureUnit(6, 0);
        renderer.shdr_pgm->UnUse();
    }

    /*!
    @brief Calculate UV coordinates for a sprite sheet frame
    @param frameX Frame column (0-based)
    @param frameY Frame row (0-based)
    @param totalCols Total columns in sprite sheet
    @param totalRows Total rows in sprite sheet
    @param u1, v1, u2, v2 Output UV coordinates
    */
    void TextureRenderer::CalculateFrameUV(int frameX, int frameY, int totalCols, int totalRows,
        float& u1, float& v1, float& u2, float& v2) {

        // Calculate frame dimensions in UV space
        float frameWidth = 1.0f / static_cast<float>(totalCols);
        float frameHeight = 1.0f / static_cast<float>(totalRows);

        // Add small inset to prevent bleeding from adjacent frames
        // Use a conservative inset that works for most sprite sheet sizes
        float uvInsetX = frameWidth * 0.01f;  // 1% inset of frame width
        float uvInsetY = frameHeight * 0.01f; // 1% inset of frame height

        // Calculate UV coordinates for this frame with inset
        u1 = frameX * frameWidth + uvInsetX;
        v1 = frameY * frameHeight + uvInsetY;
        u2 = (frameX + 1) * frameWidth - uvInsetX;
        v2 = (frameY + 1) * frameHeight - uvInsetY;

        /*std::cout << "[DEBUG] UV Calculation - Frame(" << frameX << "," << frameY << ") in " << totalCols << "x" << totalRows << " grid" << std::endl;
        std::cout << "[DEBUG] UV Coords: (" << u1 << "," << v1 << ") to (" << u2 << "," << v2 << ")" << std::endl;*/
    }

    /*!
    @brief Unloads a single texture from the cache and deletes its GPU resource.
    @param texturePath Path to the texture file to unload.
    */

    void TextureRenderer::UnloadTexture(const std::string& texturePath) {
        auto it = textureCache.find(texturePath);
        if (it != textureCache.end()) {
            glDeleteTextures(1, &it->second);
            textureCache.erase(it);
            std::cout << "Unloaded texture: " << texturePath << std::endl;
        }
    }

    /*!
    @brief Clears all cached textures and frees GPU resources.
    */

    void inline TextureRenderer::ClearTextureCache() {
        for (const auto& pair : textureCache) {
            glDeleteTextures(1, &pair.second);
        }
        textureCache.clear();
        std::unordered_map<std::string, GLuint>().swap(textureCache);
        std::cout << "Cleared texture cache." << std::endl;
    }

    /*!
    @brief Releases all TextureRenderer resources (textures and shared quad).
    Call this before shutting down the engine.
    */

    void TextureRenderer::Cleanup() {
        // Clean up cached textures
        ClearTextureCache();

        // Clean up shared quad resources
        if (sharedVAO != 0) {
            glDeleteVertexArrays(1, &sharedVAO);
            sharedVAO = 0;
        }
        if (sharedVBO != 0) {
            glDeleteBuffers(1, &sharedVBO);
            sharedVBO = 0;
        }
        if (sharedEBO != 0) {
            glDeleteBuffers(1, &sharedEBO);
            sharedEBO = 0;
        }

        initialized = false;
        std::cout << "TextureRenderer cleaned up." << std::endl;
    }

} // namespace Component // namespace CrashoutsEngine
