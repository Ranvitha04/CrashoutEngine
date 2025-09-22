
/*!
@file        texture.cpp
@author      ranvitha@digipen.edu
@date        05/05/2025

This file implements an efficient texture rendering system that caches
loaded textures and reuses a shared quad mesh. This minimizes OpenGL
state changes and memory allocations while maintaining the simple
interface similar to the Shapes system.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#ifndef TETXURE_H
#define TETXURE_H


#define STB_IMAGE_IMPLEMENTATION

#include "texture.h"
#include "rendering.h"
#include "entitymanager.h"
#include <array>
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "CEVec2.h"
#include "stb_image.h"
#include <algorithm>
#include <cctype>

namespace Component {

    // Static member definitions
    std::unordered_map<std::string, GLuint> Texture::textureCache;
    GLuint Texture::sharedVAO = 0;
    GLuint Texture::sharedVBO = 0;
    GLuint Texture::sharedEBO = 0;
    bool Texture::initialized = false;

    void Texture::Initialize() {
        if (initialized) return;

        InitializeSharedQuad();
        initialized = true;

        std::cout << "Texture system initialized with shared resources." << std::endl;
    }

    void Texture::InitializeSharedQuad() {
        // Create a unit quad with texture coordinates and white vertex colors
        std::array<float, 32> vertices = {
            // positions        // colors           // texture coords
            -0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f, // bottom left
             0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f, // bottom right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f, // top right
            -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f  // top left
        };

        std::array<GLushort, 6> indices = {
            0, 1, 2,
            2, 3, 0
        };

        // Generate and bind VAO (reused for all texture rendering)
        glGenVertexArrays(1, &sharedVAO);
        glBindVertexArray(sharedVAO);

        // Generate and bind VBO
        glGenBuffers(1, &sharedVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

        // Generate and bind EBO
        glGenBuffers(1, &sharedEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

        // Position attribute (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute (location 2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    GLuint LoadTexture(const char* pathname) {
        int width, height, channels;
        // Load with stb_image, force RGBA (4 channels)
        unsigned char* data = stbi_load(pathname, &width, &height, &channels, STBI_rgb_alpha);

        if (!data) {
            std::cerr << "Error: Could not load texture file: " << pathname << std::endl;
            return 0;
        }

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


    void Texture::CreateTexturedQuad(const std::string& texturePath,
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

    void Texture::CreateTexturedQuad(GLuint textureHandle,
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

    GLuint Texture::GetOrLoadTexture(const std::string& texturePath) {
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

    bool Texture::PreloadTexture(const std::string& texturePath) {
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

    void Texture::RenderQuad(GLuint textureHandle,
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
        GLint model_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_model");
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
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);

        // Unbind texture and shader
        glBindTextureUnit(6, 0);
        renderer.shdr_pgm.UnUse();
    }

    GLuint Texture::LoadTexture(const std::string& pathname) {
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

    GLuint Texture::LoadBinaryTexture(const std::string& pathname) {
        GLuint width = 256, height = 256, bytes_per_texel = 4;

        std::streamsize expected_size = static_cast<std::streamsize>(width * height * bytes_per_texel);

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

        char* ptr_texels = new char[size];

        if (ptr_texels == nullptr) {
            std::cerr << "Error: Memory allocation failed for: " << pathname << std::endl;
            file.close();
            return 0;
        }

        file.seekg(0);

        if (!file.read(ptr_texels, size)) {
            std::cerr << "Error: Failed to read binary texture data from: " << pathname << std::endl;
            file.close();
            delete[] ptr_texels;
            return 0;
        }

        file.close();

        GLuint texobj_hdl;
        glCreateTextures(GL_TEXTURE_2D, 1, &texobj_hdl);

        glTextureStorage2D(texobj_hdl, 1, GL_RGBA8, width, height);

        glTextureSubImage2D(texobj_hdl, 0, 0, 0, width, height,
            GL_RGBA, GL_UNSIGNED_BYTE, ptr_texels);

        glTextureParameteri(texobj_hdl, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texobj_hdl, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(texobj_hdl, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texobj_hdl, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        delete[] ptr_texels;

        std::cout << "Successfully loaded binary texture: " << pathname << " (" << width << "x" << height << ")" << std::endl;

        return texobj_hdl;
    }

    GLuint Texture::LoadImageTexture(const std::string& pathname) {
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

    void Texture::UnloadTexture(const std::string& texturePath) {
        auto it = textureCache.find(texturePath);
        if (it != textureCache.end()) {
            glDeleteTextures(1, &it->second);
            textureCache.erase(it);
            std::cout << "Unloaded texture: " << texturePath << std::endl;
        }
    }

    void Texture::ClearTextureCache() {
        for (const auto& pair : textureCache) {
            glDeleteTextures(1, &pair.second);
        }
        textureCache.clear();
        std::cout << "Cleared texture cache." << std::endl;
    }

    void Texture::Cleanup() {
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
        std::cout << "Texture system cleaned up." << std::endl;
    }

}  // namespace Component

#endif // TEXTURE_H