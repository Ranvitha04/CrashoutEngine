/*!
@file        textures.cpp
@author      ranvitha@digipen.edu

@brief
This file implements the texture management and rendering functionality
for CrashoutsEngine. It contains:

- **CrashoutsEngine::Assets::IAsset**:
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
- Use `IAsset::Create()` for lower-level texture asset creation.

Clean-up functions are provided to release GPU resources and clear caches
when shutting down the engine.
*/

#define STB_IMAGE_IMPLEMENTATION
#include "resources/textures.h"
#include "managers/entitymanager.h"
#include <stb_image.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CrashoutsEngine {
    namespace Assets {

        /*!
        @brief Constructs an IAsset with given width, height, and raw pixel data.
        @param w      Width of the texture in pixels.
        @param h      Height of the texture in pixels.
        @param inputData Raw pointer to the texture data (RGBA).
        */

        IAsset::IAsset(int w, int h, texdata inputData) :
            width(w), height(h), data(inputData), glHandle(0), uploaded(false) {
        }

        /*!
        @brief Loads image data from file using stb_image.
        @param filePath Path to the image file.
        @param width    Output reference for image width.
        @param height   Output reference for image height.
        @return Raw pixel data pointer (RGBA), or nullptr on failure.
        */


        IAsset::texdata IAsset::LoadImageData(const std::string& filePath, int& width, int& height) {
            int channels;

            
            texdata data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (!data) {
                std::cerr << "Error loading image: " << filePath << " - " << stbi_failure_reason() << std::endl;
            }
            return data;
        }

        /*!
        @brief Creates a new IAsset from an image file.
        @param filePath Path to the image file.
        @return Unique pointer to a valid IAsset, or nullptr on failure.
        */

        std::unique_ptr<IAsset> IAsset::Create(const std::string& filePath) {
            int w, h;
            texdata loadedData = LoadImageData(filePath, w, h);
            if (!loadedData) {
                return nullptr;
            }

           
            return std::unique_ptr<IAsset>(new IAsset(w, h, loadedData));
        }

        /*!
        @brief Destructor. Frees GPU resources and image memory.
        */

        IAsset::~IAsset() {
            FreeGPU();
            if (data) {
                stbi_image_free(data);
                data = nullptr;
            }
        }

        /*!
        @brief Move constructor. Transfers ownership of resources.
        */

        IAsset::IAsset(IAsset&& other) noexcept :
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

        IAsset& IAsset::operator=(IAsset&& other) noexcept {
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

        bool IAsset::UploadToGPU() {
            if (uploaded || !data) {
                return uploaded; // Already uploaded or no data
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &glHandle);
            if (glHandle == 0) {
                std::cerr << "Failed to create OpenGL texture" << std::endl;
                return false;
            }

            glTextureStorage2D(glHandle, 1, GL_RGBA8, width, height);
            glTextureSubImage2D(glHandle, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

            // Set texture parameters
            glTextureParameteri(glHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(glHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(glHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(glHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            uploaded = true;
            return true;
        }

        /*!
        @brief Frees the GPU texture resource if allocated.
        */

        void IAsset::FreeGPU() {
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

            // Texture coordinate attribute (location 2)
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
            const CEVec3& tint,
            bool modulate) {

            // Auto-initialize if needed
            if (!initialized) {
                Initialize();
            }

            // Get or load texture (uses cache)
            GLuint textureHandle = GetOrLoadTexture(texturePath);
            if (textureHandle == 0) {
                std::cerr << "Failed to load texture: " << texturePath << std::endl;
                return;
            }

            // Render using the cached texture
            RenderQuad(textureHandle, position, size, rotation, scale, tint, modulate);
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
            const CEVec3& tint,
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


        GLuint TextureRenderer::GetOrLoadTexture(const std::string& texturePath) {
            // Check cache first
            auto it = textureCache.find(texturePath);
            if (it != textureCache.end()) {
                return it->second; // Return cached texture
            }

            // Load new texture
            GLuint textureHandle = LoadTexture(texturePath);
            if (textureHandle != 0) {
                // Cache the texture
                textureCache[texturePath] = textureHandle;
                std::cout << "Cached texture: " << texturePath << std::endl;
            }

            return textureHandle;
        }

        /*!
        @brief Loads and caches a texture in advance.
        @param texturePath Path to the texture file.
        @return True if successfully loaded, false otherwise.
        */

        bool TextureRenderer::PreloadTexture(const std::string& texturePath) {
            // Check if already loaded
            if (textureCache.find(texturePath) != textureCache.end()) {
                return true;
            }

            GLuint textureHandle = LoadTexture(texturePath);
            if (textureHandle != 0) {
                textureCache[texturePath] = textureHandle;
                std::cout << "Preloaded texture: " << texturePath << std::endl;
                return true;
            }

            return false;
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
            const CEVec3& tint,
            bool modulate) {

            // Get the rendering instance and use shader
            Rendering& renderer = Rendering::Get();
            renderer.shdr_pgm.Use();

            // Setup transformation matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
            model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(size.x * scale, size.y * scale, 1.0f));

            // Pass transformation matrix to shader
            GLint model_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "uModel");
            if (model_loc != -1) {
                glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
            }

            // Enable texture usage in shader
            GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
            if (useTexture_loc != -1) {
                glUniform1i(useTexture_loc, 1); // Enable texture
            }

            // Set texture sampler
            GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "uTex2d");
            if (tex_loc != -1) {
                glUniform1i(tex_loc, 6);
            }

            // Set modulation (whether to multiply by vertex color)
            GLint modulate_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_modulate");
            if (modulate_loc != -1) {
                glUniform1i(modulate_loc, modulate ? 1 : 0);
            }

            // Set tint color
            GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
            if (tint_loc != -1) {
                glUniform3f(tint_loc, tint.x, tint.y, tint.z);
            }

            // Bind texture to texture unit 6
            glBindTextureUnit(6, textureHandle);

            // Use shared VAO for rendering (no buffer creation/deletion per call)
            glBindVertexArray(sharedVAO);
            glDrawElements(GL_TRIANGLES, INDICES_COUNT, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);

            // Unbind texture and shader
            glBindTextureUnit(6, 0);
            renderer.shdr_pgm.UnUse();
        }

        /*!
        @brief Loads a texture from file depending on its extension.
        @param pathname Path to the texture file (.png, .jpg, .tex, etc).
        @return OpenGL texture handle, or 0 on failure.
        */


        GLuint TextureRenderer::LoadTexture(const std::string& pathname) {
            size_t dotPos = pathname.find_last_of('.');
            if (dotPos == std::string::npos) {
                std::cerr << "Error: No file extension found in: " << pathname << std::endl;
                return 0;
            }

            std::string extension = pathname.substr(dotPos);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".tex") {
                return LoadBinaryTexture(pathname);
            }
            else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                extension == ".bmp" || extension == ".tga" || extension == ".gif") {
                return LoadImageTexture(pathname);
            }
            else {
                std::cerr << "Error: Unsupported texture format: " << extension << std::endl;
                return 0;
            }
        }

        /*!
        @brief Loads a custom binary texture (.tex format).
        @param pathname Path to binary texture file.
        @return OpenGL texture handle, or 0 on failure.
        */

        GLuint TextureRenderer::LoadBinaryTexture(const std::string& pathname) {
            const GLuint width = 256, height = 256, bytes_per_texel = 4;
            const std::streamsize expected_size = static_cast<std::streamsize>(width * height * bytes_per_texel);

            std::ifstream file(pathname, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open binary texture file: " << pathname << std::endl;
                return 0;
            }

            std::streamsize size = file.tellg();
            if (size != expected_size) {
                std::cerr << "Error: Binary texture size mismatch for " << pathname
                    << ". Expected: " << expected_size << ", Got: " << size << std::endl;
                file.close();
                return 0;
            }

            std::vector<char> buffer(size);
            file.seekg(0);

            if (!file.read(buffer.data(), size)) {
                std::cerr << "Error: Failed to read binary texture data from: " << pathname << std::endl;
                file.close();
                return 0;
            }

            file.close();

            GLuint texobj_hdl;
            glCreateTextures(GL_TEXTURE_2D, 1, &texobj_hdl);
            glTextureStorage2D(texobj_hdl, 1, GL_RGBA8, width, height);
            glTextureSubImage2D(texobj_hdl, 0, 0, 0, width, height,
                GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

            glTextureParameteri(texobj_hdl, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(texobj_hdl, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(texobj_hdl, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(texobj_hdl, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            std::cout << "Successfully loaded binary texture: " << pathname
                << " (" << width << "x" << height << ")" << std::endl;

            return texobj_hdl;
        }

        /*!
        @brief Loads a standard image texture using stb_image.
        @param pathname Path to image file (.png, .jpg, etc).
        @return OpenGL texture handle, or 0 on failure.
        */

        GLuint TextureRenderer::LoadImageTexture(const std::string& pathname) {
            int width, height, channels;

            // Load with stb_image, force RGBA (4 channels)
            unsigned char* data = stbi_load(pathname.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!data) {
                std::cerr << "Error: stb_image failed to load: " << pathname << std::endl;
                std::cerr << "stb_image error: " << stbi_failure_reason() << std::endl;
                return 0;
            }

            std::cout << "Successfully loaded image: " << pathname << " (" << width << "x" << height
                << ", " << channels << " channels)" << std::endl;

            GLuint texobj_hdl;
            glCreateTextures(GL_TEXTURE_2D, 1, &texobj_hdl);
            glTextureStorage2D(texobj_hdl, 1, GL_RGBA8, width, height);
            glTextureSubImage2D(texobj_hdl, 0, 0, 0, width, height,
                GL_RGBA, GL_UNSIGNED_BYTE, data);

            glTextureParameteri(texobj_hdl, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(texobj_hdl, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(texobj_hdl, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(texobj_hdl, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            stbi_image_free(data);
            return texobj_hdl;
        }

        /*!
        @brief Loads a texture using the asset system (alias of LoadImageTexture).
        @param pathname Path to image file.
        @return OpenGL texture handle, or 0 on failure.
        */

        GLuint TextureRenderer::LoadTextureFromAsset(const std::string& pathname) {
            // This method is now an alias to LoadImageTexture for consistency
            return LoadImageTexture(pathname);
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

        void TextureRenderer::ClearTextureCache() {
            for (const auto& pair : textureCache) {
                glDeleteTextures(1, &pair.second);
            }
            textureCache.clear();
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

    } // namespace Component
 // namespace CrashoutsEngine