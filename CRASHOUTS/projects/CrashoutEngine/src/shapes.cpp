/*!
@file        shapes.cpp
@author      ranvitha@digipen.edu
@date        12/09/2025

This file implements the Shapes struct, which provides static functions
to create and draw common geometric primitives (square, rectangle, line,
and triangle) using modern OpenGL. Each shape function encapsulates
the setup of vertex data, color data, buffer objects (VBO, VAO, EBO),
and shader uniform transformations (model, view, projection matrices).

The file also manages the lifetime of rendering components across frames,
ensuring cleanup of GPU resources such as VAOs, VBOs, and EBOs. Helper
functions are included to apply transformations (translation, rotation,
scaling) to shape vertices for flexible rendering.

*//*__________________________________________________________________________*/

/* _________________________________________________________________________
----------------------------------------------------------------------------- */

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "shapes.h"
#include "rendering.h"
#include "managers/entitymanager.h"
#include <array>
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "resources/textureasset.h"


namespace Component {

    // Centered world coordinate system settings
    struct WorldSettings {

        //float const& WindowWidth = static_cast<float>(GLApp::width);
        inline static float BASE_SIZE = 100.0f;
    };

    glm::mat4 CreateCenteredWorldProjection() {
        float aspect = static_cast<float>(GLApp::width) / GLApp::height;

        float worldWidth, worldHeight;

        if (aspect >= 1.0f) {
            // Wide/square screen: fix height, expand width
            worldHeight = WorldSettings::BASE_SIZE;
            worldWidth = worldHeight * aspect;
        }
        else {
            // Tall screen: fix width, expand height  
            worldWidth = WorldSettings::BASE_SIZE;
            worldHeight = worldWidth / aspect;
        }

        return glm::ortho(
            -worldWidth / 2.0f, worldWidth / 2.0f,
            -worldHeight / 2.0f, worldHeight / 2.0f,
            -1.0f, 1.0f
        );
    }

    void Shapes::Cleanup() {
        Component::Rendering::Cleanup();
    }

    void Shapes::CreateSquare(const CEVec3& position, float size, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Define square vertices centered at origin
        float halfSize = size / 2.0f;
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfSize, -halfSize, 0.0f),
            CEVec3(halfSize, -halfSize, 0.0f),
            CEVec3(halfSize, halfSize, 0.0f),
            CEVec3(-halfSize, halfSize, 0.0f)
        };

        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };
        std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };

        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        renderer.shdr_pgm.Use();

        // Create model matrix for world position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        // Use centered world coordinate projection
        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);
        renderer.shdr_pgm.SetUniform("u_useTexture", 0);
        renderer.shdr_pgm.SetUniform("u_tint", 1.0f, 1.0f, 1.0f);
        renderer.Draw();
    }

    void Shapes::CreateLine(const CEVec3& start, const CEVec3& end, float thickness, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Calculate line in world coordinates
        CEVec3 dir = end - start;
        glm::vec3 gdir(dir.x, dir.y, dir.z);
        glm::vec3 normal(-gdir.y, gdir.x, 0.0f);
        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal) * (thickness / 2.0f);
        }

        std::array<CEVec3, 4> pos_vtx{
            CEVec3(start.x + normal.x, start.y + normal.y, start.z),
            CEVec3(start.x - normal.x, start.y - normal.y, start.z),
            CEVec3(end.x - normal.x, end.y - normal.y, end.z),
            CEVec3(end.x + normal.x, end.y + normal.y, end.z)
        };

        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };
        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };

        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm.Use();

        // Apply transformations (rotation and scale around center of line)
        CEVec3 center = (start + end) / 2.0f;
        glm::mat4 model = glm::mat4(1.0f);

        // Translate to center, apply transformations, translate back
        model = glm::translate(model, glm::vec3(center.x, center.y, center.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        model = glm::translate(model, glm::vec3(-center.x, -center.y, -center.z));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);
        renderer.shdr_pgm.SetUniform("u_useTexture", 0);
        renderer.shdr_pgm.SetUniform("u_tint", 1.0f, 1.0f, 1.0f);
        renderer.Draw();
    }

    void Shapes::CreateRectangle(const CEVec3& position, float width, float height, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Rectangle vertices centered at origin
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, halfHeight, 0.0f),
            CEVec3(-halfWidth, halfHeight, 0.0f)
        };

        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };
        std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };

        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        renderer.shdr_pgm.Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("u_useTexture", 0);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);
        renderer.shdr_pgm.SetUniform("u_tint", 1.0f, 1.0f, 1.0f);

        renderer.Draw();
    }

    void Shapes::CreateTriangle(const CEVec3& p1, const CEVec3& p2, const CEVec3& p3, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Triangle vertices in world coordinates
        std::array<CEVec3, 3> pos_vtx{
            CEVec3(p1.x, p1.y, p1.z),
            CEVec3(p2.x, p2.y, p2.z),
            CEVec3(p3.x, p3.y, p3.z)
        };

        std::array<CEVec3, 3> clr_vtx{ color, color, color };
        std::array<GLushort, 3> idx_vtx{ 0, 1, 2 };

        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm.Use();

        // Transform around centroid
        CEVec3 centroid = (p1 + p2 + p3) / 3.0f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centroid.x, centroid.y, centroid.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        model = glm::translate(model, glm::vec3(-centroid.x, -centroid.y, -centroid.z));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("u_useTexture", 0);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);
        renderer.shdr_pgm.SetUniform("u_tint", 1.0f, 1.0f, 1.0f);
        renderer.Draw();
    }

    void Shapes::CreateTexturedSquare(const std::string& texturePath, const CEVec3& position, float size,
        float rotation, float scale, const CEVec3& tint, const CEVec2& uvSize) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Get or load the texture
        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(texturePath);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }

        // Define square vertices with texture coordinates
        float halfSize = size / 2.0f;

        // Position data
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfSize, -halfSize, 0.0f), // bottom left
            CEVec3(halfSize, -halfSize, 0.0f),  // bottom right
            CEVec3(halfSize, halfSize, 0.0f),   // top right
            CEVec3(-halfSize, halfSize, 0.0f)   // top left
        };

        // Color data (will be used as tint)
        std::array<CEVec3, 4> clr_vtx{ tint, tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;

        // TextureAsset coordinate data
        std::array<CEVec2, 4> tex_vtx{
            CEVec2(0.0f,        uvHeightMax), // bottom left
            CEVec2(uvWidthMax,  uvHeightMax), // bottom right  
            CEVec2(uvWidthMax,  0.0f), // top right
            CEVec2(0.0f,        0.0f)  // top left
        };

        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 }; // Two triangles

        // Setup buffers with texture coordinates
        renderer.SetupTexturedBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            tex_vtx.data(), sizeof(tex_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm.Use();

        // Create model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        // Set uniforms
        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        // Enable texture rendering
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) {
            glUniform1i(useTexture_loc, 1);
        }

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "uTex2d");
        if (tex_loc != -1) {
            glUniform1i(tex_loc, 0);
        }

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
        if (tint_loc != -1) {
            glUniform3f(tint_loc, tint.x, tint.y, tint.z);
        }

        // Enable Blending for Transparency
        glEnable(GL_BLEND);
        // Set the standard alpha blending function
        // (Source Alpha * Source Color + (1 - Source Alpha) * Destination Color)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Bind texture and draw
        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
    }

    void Shapes::CreateTexturedRectangle(const std::string& texturePath, const CEVec3& position,
        float width, float height, float rotation, float scale, const CEVec3& tint, const CEVec2& uvSize) {
        Component::Rendering& renderer = Component::Rendering::Get();

        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(texturePath);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }

        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, halfHeight, 0.0f),
            CEVec3(-halfWidth, halfHeight, 0.0f)
        };

        std::array<CEVec3, 4> clr_vtx{ tint, tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;

        std::array<CEVec2, 4> tex_vtx{
            CEVec2(0.0f,        uvHeightMax), // bottom left
            CEVec2(uvWidthMax,  uvHeightMax), // bottom right  
            CEVec2(uvWidthMax,  0.0f), // top right
            CEVec2(0.0f,        0.0f)  // top left
        };

        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };

        renderer.SetupTexturedBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            tex_vtx.data(), sizeof(tex_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm.Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        // TextureAsset uniforms
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);

        // Enable Blending for Transparency
        glEnable(GL_BLEND);
        // Set the standard alpha blending function
        // (Source Alpha * Source Color + (1 - Source Alpha) * Destination Color)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
    }

    void Shapes::CreateTexturedTriangle(const std::string& texturePath, const CEVec3& p1, const CEVec3& p2,
        const CEVec3& p3, float rotation, float scale, const CEVec3& tint, const CEVec2& uvSize) {
        Component::Rendering& renderer = Component::Rendering::Get();

        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(texturePath);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }

        std::array<CEVec3, 3> pos_vtx{
            CEVec3(p1.x, p1.y, p1.z),
            CEVec3(p2.x, p2.y, p2.z),
            CEVec3(p3.x, p3.y, p3.z)
        };

        std::array<CEVec3, 3> clr_vtx{ tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;

        // Triangle texture coordinates
        std::array<CEVec2, 3> tex_vtx{
            CEVec2(0.0f, uvHeightMax), // bottom left
            CEVec2(uvWidthMax, uvHeightMax), // bottom right
            CEVec2(uvWidthMax * 0.5f, 0.0f)  // top center
        };

        std::array<GLushort, 3> idx_vtx{ 0, 1, 2 };

        renderer.SetupTexturedBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            tex_vtx.data(), sizeof(tex_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm.Use();

        CEVec3 centroid = (p1 + p2 + p3) / 3.0f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centroid.x, centroid.y, centroid.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        model = glm::translate(model, glm::vec3(-centroid.x, -centroid.y, -centroid.z));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);

        // Enable Blending for Transparency
        glEnable(GL_BLEND);
        // Set the standard alpha blending function
        // (Source Alpha * Source Color + (1 - Source Alpha) * Destination Color)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
    }
}
