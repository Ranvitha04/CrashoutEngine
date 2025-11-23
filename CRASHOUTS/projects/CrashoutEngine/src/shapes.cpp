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

/* includes
----------------------------------------------------------------------------- */

#include "shapes.h"
#include "rendering.h"
#include "entitymanager.h"
#include <array>
#include <vector>
#include <CEMath.h>
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "textureasset.h"

// Ensure we are using the new Color definition
#include "CEColor.h"

namespace Component {

    /*!
    @struct WorldSettings
    @brief Configuration settings for the world coordinate system.
    @details
    Defines the base size for the world coordinate system used in shape rendering.
    The BASE_SIZE determines the world units for the smaller dimension of the screen,
    with the larger dimension scaled proportionally based on aspect ratio.
    */
    struct WorldSettings {
        /*!
        @brief Base world size in units for the smaller screen dimension.
        @details
        This value determines the world coordinate range. For example, with BASE_SIZE = 100.0f,
        a square screen would have world coordinates from -50 to +50 in both dimensions.
        */
        inline static float BASE_SIZE = 100.0f;
    };

    // Static buffers for GPU instancing (created once, reused)
    static GLuint s_instanceVAO = 0;
    static GLuint s_instanceVBO = 0;  // Base geometry
    static GLuint s_instanceEBO = 0;  // Indices
    static GLuint s_instanceDataVBO = 0;  // Instance data (positions, colors, etc.)
    static bool s_instanceBuffersInitialized = false;

    void Shapes::Cleanup() {
        Component::Rendering::Cleanup();

        // Clean up static instancing buffers
        if (s_instanceBuffersInitialized) {
            if (s_instanceVAO != 0) {
                glDeleteVertexArrays(1, &s_instanceVAO);
                s_instanceVAO = 0;
            }
            if (s_instanceVBO != 0) {
                glDeleteBuffers(1, &s_instanceVBO);
                s_instanceVBO = 0;
            }
            if (s_instanceEBO != 0) {
                glDeleteBuffers(1, &s_instanceEBO);
                s_instanceEBO = 0;
            }
            if (s_instanceDataVBO != 0) {
                glDeleteBuffers(1, &s_instanceDataVBO);
                s_instanceDataVBO = 0;
            }
            s_instanceBuffersInitialized = false;
        }
    }

    void Shapes::CreateLine(const CEVec3& start, const CEVec3& end, float thickness, const CEColor& color, const bool isWorldSpace) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // 1. CALCULATE VERTICES IN FINAL WORLD-SPACE
        CEVec3 dir = end - start;
        glm::vec3 gdir(dir.x, dir.y, dir.z);

        // Calculate perpendicular normal, scaled by half-thickness
        glm::vec3 normal(-gdir.y, gdir.x, 0.0f);
        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal) * (thickness / 2.0f);
        }

        // Vertices are now in their final world positions
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(start.x + normal.x, start.y + normal.y, start.z),
            CEVec3(start.x - normal.x, start.y - normal.y, start.z),
            CEVec3(end.x - normal.x, end.y - normal.y, end.z),
            CEVec3(end.x + normal.x, end.y + normal.y, end.z)
        };

        // CEColor Copy Constructor handles this
        std::array<CEColor, 4> clr_vtx{ color, color, color, color };
        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };

        // 2. SETUP BUFFERS (with final vertex data)
        // Note: CEColor is contiguous floats (rgba), so passing .data() works for OpenGL
        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        // 3. SET MODEL MATRIX TO IDENTITY
        // The vertices are already in world space, so no model transform is needed.
        // (We assume the shader will multiply this by View * Projection)

        glm::mat4 model = glm::mat4(1.0f);

        glm::mat4 finalModelMatrix;
        if (isWorldSpace)
        {
            finalModelMatrix = model;
        }
        else
        {
            glm::mat4 viewMatrix = renderer.viewMatrix;
            glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

            finalModelMatrix = invViewMatrix * model;
        }
        renderer.shdr_pgm->SetUniform("uModel", finalModelMatrix);

        // 4. SET SHADER UNIFORMS & DRAW
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.f); // No tint

        renderer.Draw();
    }

    void Shapes::CreateRectangle(const CEVec2& transformSize, const CEVec3& position, float rotation, const CEColor& color, const bool isWorldSpace) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Rectangle vertices centered at origin
        float halfWidth = .5f;
        float halfHeight = .5f;
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, halfHeight, 0.0f),
            CEVec3(-halfWidth, halfHeight, 0.0f)
        };

        std::array<CEColor, 4> clr_vtx{ color, color, color, color };
        std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };


        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        renderer.shdr_pgm->Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transformSize.x, transformSize.y, 1.0f));

        glm::mat4 finalModelMatrix;
        if (isWorldSpace)
        {
            finalModelMatrix = model;
        }
        else
        {
            glm::mat4 viewMatrix = renderer.viewMatrix;
            glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

            finalModelMatrix = invViewMatrix * model;
        }

        renderer.shdr_pgm->SetUniform("uModel", finalModelMatrix);
        renderer.shdr_pgm->SetUniform("u_useTexture", 0);

        renderer.shdr_pgm->SetUniform("u_tint", 1.0f, 1.0f, 1.0f, 1.f);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.f); // No tint modification

        renderer.Draw();
    }

    void Shapes::CreateTriangle(const CEVec2& transformSize, const CEVec3& pos, float rotation, const CEColor& color, const bool isWorldSpace) {
        Component::Rendering& renderer = Component::Rendering::Get();

        CEVec3 p1 = pos + CEVec3{ -.5f, -.5f, 0.f };
        CEVec3 p2 = pos + CEVec3{ .5f, -.5f, 0.f };
        CEVec3 p3 = pos + CEVec3{ 0.f, .5f, 0.f };

        // Triangle vertices in world coordinates
        std::array<CEVec3, 3> pos_vtx{
            CEVec3(p1.x, p1.y, p1.z),
            CEVec3(p2.x, p2.y, p2.z),
            CEVec3(p3.x, p3.y, p3.z)
        };

        std::array<CEColor, 3> clr_vtx{ color, color, color };
        std::array<GLushort, 3> idx_vtx{ 0, 1, 2 };


        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        // Transform around centroid
        CEVec3 centroid = (p1 + p2 + p3) / 3.0f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centroid.x, centroid.y, centroid.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transformSize.x, transformSize.y, 1.0f));
        model = glm::translate(model, glm::vec3(-centroid.x, -centroid.y, -centroid.z));

        glm::mat4 finalModelMatrix;
        if (isWorldSpace)
        {
            finalModelMatrix = model;
        }
        else
        {
            glm::mat4 viewMatrix = renderer.viewMatrix;
            glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

            finalModelMatrix = invViewMatrix * model;
        }

        renderer.shdr_pgm->SetUniform("uModel", finalModelMatrix);
        renderer.shdr_pgm->SetUniform("u_useTexture", 0);


        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.f); // No tint modification

        renderer.Draw();
    }

    void Shapes::CreateCircle(const CEVec2& transformSize, const CEVec3& position, int segments, float rotation, const CEColor& color, const bool isWorldSpace) {
        Component::Rendering& renderer = Component::Rendering::Get();

        //minimum segments for circle
        if (segments < 3) segments = 3;
        if (segments > 360) segments = 360; // Reasonable upper limit


        std::vector<CEVec3> pos_vtx;
        std::vector<CEColor> clr_vtx;
        std::vector<GLushort> idx_vtx;

        // Center vertex at origin (will be transformed to position)
        pos_vtx.push_back(CEVec3(0.0f, 0.0f, 0.0f));
        clr_vtx.push_back(color);

        // Generate perimeter vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * static_cast<float>(CEMath::PI) * static_cast<float>(i) / static_cast<float>(segments);
            float x = cosf(angle);
            float y = sinf(angle);
            pos_vtx.push_back(CEVec3(x, y, 0.0f));
            clr_vtx.push_back(color);
        }

        // Generate indices for triangle fan
        for (int i = 0; i < segments; ++i) {
            // Ensure indices fit within GLushort range (0-65535)
            idx_vtx.push_back(0);                              // Center vertex
            idx_vtx.push_back(static_cast<GLushort>(i + 1));       // Current perimeter vertex
            idx_vtx.push_back(static_cast<GLushort>(i + 2));       // Next perimeter vertex
        }


        renderer.SetupBuffersOptimized(
            pos_vtx.data(), pos_vtx.size() * sizeof(CEVec3),
            clr_vtx.data(), clr_vtx.size() * sizeof(CEColor), // Updated to use sizeof(CEColor)
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        // Create model matrix for world position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transformSize.x, transformSize.y, 1.0f));

        glm::mat4 finalModelMatrix;
        if (isWorldSpace)
        {
            finalModelMatrix = model;
        }
        else
        {
            glm::mat4 viewMatrix = renderer.viewMatrix;
            glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

            finalModelMatrix = invViewMatrix * model;
        }

        renderer.shdr_pgm->SetUniform("uModel", finalModelMatrix);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.f); // No tint modification

        renderer.Draw();
    }

    void Shapes::CreateTexturedRectangle(const CEVec2& transformSize, const std::string& texturePath, const CEVec3& position, float rotation,
        const CEColor& tint, CEVec2 const& uvSize, const bool isWorldSpace) {
        Component::Rendering& renderer = Component::Rendering::Get();

        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(texturePath);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }

        float halfWidth = .5f;
        float halfHeight = .5f;

        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, halfHeight, 0.0f),
            CEVec3(-halfWidth, halfHeight, 0.0f)
        };

        std::array<CEColor, 4> clr_vtx{ tint, tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;


        std::array<CEVec2, 4> tex_vtx{
            CEVec2(0.0f, uvHeightMax),        // bottom left
            CEVec2(uvWidthMax, uvHeightMax), // bottom right
            CEVec2(uvWidthMax, 0.0f),         // top right
            CEVec2(0.0f, 0.0f)                // top left
        };

        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };

        // Use optimized textured buffer setup
        renderer.SetupTexturedBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            tex_vtx.data(), sizeof(tex_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transformSize.x, transformSize.y, 1.0f));

        glm::mat4 finalModelMatrix;
        if (isWorldSpace)
        {
            finalModelMatrix = model;
        }
        else
        {
            glm::mat4 viewMatrix = renderer.viewMatrix;
            glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

            finalModelMatrix = invViewMatrix * model;
        }

        renderer.shdr_pgm->SetUniform("uModel", finalModelMatrix);


        // TextureAsset uniforms
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        // Updated tint usage: Accessed via .r, .g, .b (members) instead of .x, .y, .z
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);

        // Enable Blending for Transparency
        glEnable(GL_BLEND);    // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
        //Reset Depth Mask for the next frame or subsequent opaque rendering
        glDisable(GL_BLEND);
    }

    void Shapes::CreateTexturedTriangle(const CEVec2& transformSize, const std::string& texturePath, const CEVec3& pos,
        float rotation, const CEColor& tint, const CEVec2& uvSize, const bool isWorldSpace) {
        Component::Rendering& renderer = Component::Rendering::Get();
        CEVec3 p1 = pos + CEVec3{ -.5f, -.5f, 0.f };
        CEVec3 p2 = pos + CEVec3{ .5f, -.5f, 0.f };
        CEVec3 p3 = pos + CEVec3{ 0.f, .5f, 0.f };

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

        // FIXED: Changed CEVec3 to CEColor to match the type of 'tint' and prevent data slicing
        std::array<CEColor, 3> clr_vtx{ tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;


        float minX = std::min(p1.x, std::min(p2.x, p3.x));
        float maxX = std::max(p1.x, std::max(p2.x, p3.x));
        float minY = std::min(p1.y, std::min(p2.y, p3.y));
        float maxY = std::max(p1.y, std::max(p2.y, p3.y));
        float width = std::max(0.0001f, maxX - minX);
        float height = std::max(0.0001f, maxY - minY);

        std::array<CEVec2, 3> tex_vtx{
            CEVec2((p1.x - minX) / width * uvWidthMax, (maxY - p1.y) / height * uvHeightMax),
            CEVec2((p2.x - minX) / width * uvWidthMax, (maxY - p2.y) / height * uvHeightMax),
            CEVec2((p3.x - minX) / width * uvWidthMax, (maxY - p3.y) / height * uvHeightMax)
        };

        std::array<GLushort, 3> idx_vtx{ 0, 1, 2 };

        // Use optimized textured buffer 
        renderer.SetupTexturedBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            tex_vtx.data(), sizeof(tex_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        CEVec3 centroid = (p1 + p2 + p3) / 3.0f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centroid.x, centroid.y, centroid.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transformSize.x, transformSize.y, 1.0f));
        model = glm::translate(model, glm::vec3(-centroid.x, -centroid.y, -centroid.z));

        glm::mat4 finalModelMatrix;
        if (isWorldSpace)
        {
            finalModelMatrix = model;
        }
        else
        {
            glm::mat4 viewMatrix = renderer.viewMatrix;
            glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

            finalModelMatrix = invViewMatrix * model;
        }

        renderer.shdr_pgm->SetUniform("uModel", finalModelMatrix);


        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        // Updated tint usage: Accessed via .r, .g, .b (members) instead of .x, .y, .z
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);

        // Enable Blending for Transparency
        glEnable(GL_BLEND);    // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
        //Reset Depth Mask for the next frame or subsequent opaque rendering
        glDisable(GL_BLEND);
    }
}