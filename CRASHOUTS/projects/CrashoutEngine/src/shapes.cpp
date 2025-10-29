
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
#include "entitymanager.h"
#include <array>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "textureasset.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


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

    /*!
    @brief Create an orthographic projection matrix for centered world coordinates.
    @return Orthographic projection matrix for the current screen aspect ratio
    @details
    Creates an orthographic projection matrix that centers the world coordinate system
    at (0,0) and scales the world to maintain consistent proportions across different
    screen aspect ratios. The world dimensions are calculated as follows:
    - For wide/square screens: height = BASE_SIZE, width = height * aspect
    - For tall screens: width = BASE_SIZE, height = width / aspect

    The resulting coordinate system has (0,0) at the center of the screen with
    positive X to the right and positive Y upward.
    */
    glm::mat4 CreateCenteredWorldProjection() {
        float aspect = static_cast<float>(GLApp::width) / GLApp::height;

        float worldWidth, worldHeight;

        if (aspect >= 1.0f) {

            worldHeight = WorldSettings::BASE_SIZE;
            worldWidth = worldHeight * aspect;
        }
        else {

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

    /*!
    @brief Create and render a colored square with full transformation support.
    @param position World position for the square center (x, y, z coordinates)
    @param size Side length of the square in world units
    @param rotation Rotation angle in degrees around the Z-axis
    @param scale Uniform scaling factor applied to the square
    @param color RGB color values for the square (0.0-1.0 range)
    @details
    Creates and immediately renders a square primitive with comprehensive
    transformation support and optimized rendering pipeline integration.
    The square is generated with vertex colors and uses the centered world
    coordinate system for consistent positioning across different screen sizes.

    @note Requires active OpenGL context and initialized Rendering system
    @warning Color values outside 0.0-1.0 range may produce unexpected results
    */
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


        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        renderer.shdr_pgm->Use();

        // Create model matrix for world position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        // Use centered world coordinate projection
        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f); // No tint modification

        renderer.Draw();
    }

    /*!
    @brief Create and render multiple squares using GPU instancing for optimal performance.
    @param positions Array of world positions for each square center
    @param colors Array of RGB color values for each square
    @param rotations Array of rotation angles in degrees for each square
    @param scales Array of scaling factors for each square
    @param count Number of squares to render (must match array sizes)
    @param baseSize Base side length for squares before individual scaling
    @details
    Renders multiple squares in a single draw call using OpenGL instancing
    for maximum performance. Each square can have individual position, color,
    rotation, and scale while sharing the same base geometry. This function
    is ideal for particle systems, UI elements, or any scenario requiring
    many similar shapes with different properties.

    @note Requires arrays of size 'count' for all parameter arrays
    @warning Array sizes must match count parameter to avoid memory access errors
    @note First call initializes static buffers; subsequent calls reuse them
    */
    void Shapes::CreateInstancedSquares(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        int count, float baseSize) {
        if (count <= 0) return;

        Component::Rendering& renderer = Component::Rendering::Get();

        // Initialize static buffers once
        if (!s_instanceBuffersInitialized) {
            // Define base square vertices (only 4 vertices, not count*4!)
            float halfSize = baseSize / 2.0f;
            std::array<CEVec3, 4> base_vertices{
                CEVec3(-halfSize, -halfSize, 0.0f),
                CEVec3(halfSize, -halfSize, 0.0f),
                CEVec3(halfSize, halfSize, 0.0f),
                CEVec3(-halfSize, halfSize, 0.0f)
            };

            // Base indices for one quad
            std::array<GLushort, 6> base_indices{
                0, 1, 2,  // First triangle
                0, 2, 3   // Second triangle
            };

            // Create VAO and buffers
            glGenVertexArrays(1, &s_instanceVAO);
            glGenBuffers(1, &s_instanceVBO);
            glGenBuffers(1, &s_instanceEBO);
            glGenBuffers(1, &s_instanceDataVBO);

            glBindVertexArray(s_instanceVAO);

            // Setup base geometry (vertices)
            glBindBuffer(GL_ARRAY_BUFFER, s_instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices.data(), GL_STATIC_DRAW);

            // Vertex positions (location 0)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CEVec3), (void*)0);
            glEnableVertexAttribArray(0);

            // Setup indices
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_instanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

            //  instance data buffer 
            glBindBuffer(GL_ARRAY_BUFFER, s_instanceDataVBO);

            // Instance position (location 3) 
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            // Instance color (location 4) 
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            // Instance rotation (location 5)
            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            // Instance scale (location 6) 
            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            s_instanceBuffersInitialized = true;
        }

        // Prepare instance data (position, color, rotation, scale per instance)
        std::vector<float> instanceData;
        instanceData.reserve(count * 8);  // 8 floats per instance

        for (int i = 0; i < count; ++i) {

            // Position 
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);

            // Color 
            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);

            // Rotation
            instanceData.push_back(rotations[i]);

            // Scale 
            instanceData.push_back(scales[i]);
        }

        // Update instance data buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_instanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        // Render using instanced drawing
        renderer.shdr_pgm->Use();

        glm::mat4 projection = CreateCenteredWorldProjection();
        renderer.shdr_pgm->SetUniform("uModel", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f);

        // Bind VAO and draw instances
        glBindVertexArray(s_instanceVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, count);
        glBindVertexArray(0);
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


        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        // Apply transformations (rotation and scale around center of line)
        CEVec3 center = (start + end) / 2.0f;
        glm::mat4 model = glm::mat4(1.0f);

        // Translate to center, apply transformations, translate back
        model = glm::translate(model, glm::vec3(center.x, center.y, center.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        model = glm::translate(model, glm::vec3(-center.x, -center.y, -center.z));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f); // No tint modification

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


        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        renderer.shdr_pgm->Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("u_useTexture", 0);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);
        renderer.shdr_pgm->SetUniform("u_tint", 1.0f, 1.0f, 1.0f);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f); // No tint modification

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
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        model = glm::translate(model, glm::vec3(-centroid.x, -centroid.y, -centroid.z));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("u_useTexture", 0);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f); // No tint modification

        renderer.Draw();
    }

    void Shapes::CreateCircle(const CEVec3& position, float radius, int segments, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        //minimum segments for circle
        if (segments < 3) segments = 3;
        if (segments > 360) segments = 360; // Reasonable upper limit


        std::vector<CEVec3> pos_vtx;
        std::vector<CEVec3> clr_vtx;
        std::vector<GLushort> idx_vtx;

        // Center vertex at origin (will be transformed to position)
        pos_vtx.push_back(CEVec3(0.0f, 0.0f, 0.0f));
        clr_vtx.push_back(color);

        // Generate perimeter vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
            float x = radius * cosf(angle);
            float y = radius * sinf(angle);
            pos_vtx.push_back(CEVec3(x, y, 0.0f));
            clr_vtx.push_back(color);
        }

        // Generate indices for triangle fan
        for (int i = 0; i < segments; ++i) {
            // Ensure indices fit within GLushort range (0-65535)
            idx_vtx.push_back(0);                                  // Center vertex
            idx_vtx.push_back(static_cast<GLushort>(i + 1));       // Current perimeter vertex
            idx_vtx.push_back(static_cast<GLushort>(i + 2));       // Next perimeter vertex
        }


        renderer.SetupBuffersOptimized(pos_vtx.data(), pos_vtx.size() * sizeof(CEVec3),
            clr_vtx.data(), clr_vtx.size() * sizeof(CEVec3),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        // Create model matrix for world position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        renderer.Draw();
    }

    /*!
    @brief Create and render a textured square with full transformation and tinting support.
    @param texturePath File path to the texture image (relative or absolute)
    @param position World position for the square center (x, y, z coordinates)
    @param size Side length of the square in world units
    @param rotation Rotation angle in degrees around the Z-axis
    @param scale Uniform scaling factor applied to the square
    @param tint RGB color tint applied to the texture (0.0-1.0 range)
    @param uvSize UV coordinate scaling for texture tiling effects
    @details
    Creates and immediately renders a textured square with comprehensive
    transformation support, color tinting, and UV coordinate control.
    The function integrates with CrashoutEngine's texture management system
    for efficient texture loading and binding, while supporting alpha blending
    for transparent textures.

    @note Requires valid texture file and active OpenGL context
    @warning Invalid texture paths will result in early function return
    @note Alpha blending is automatically enabled for transparency support
    */
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


        std::array<CEVec3, 4> clr_vtx{ tint, tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;

        // TextureAsset coordinate data
        std::array<CEVec2, 4> tex_vtx{
            CEVec2(0.0f, uvHeightMax),       // bottom left
            CEVec2(uvWidthMax, uvHeightMax), // bottom right
            CEVec2(uvWidthMax, 0.0f),        // top right
            CEVec2(0.0f, 0.0f)               // top left
        };

        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 }; // Two triangles

        // Use optimized textured buffer setup for maximum performance
        renderer.SetupTexturedBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            tex_vtx.data(), sizeof(tex_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm->Use();

        // Create model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        // Set uniforms
        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        // Enable texture rendering
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) {
            glUniform1i(useTexture_loc, 1);
        }

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) {
            glUniform1i(tex_loc, 0);
        }

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) {
            glUniform3f(tint_loc, tint.x, tint.y, tint.z);
        }

        // Enable Blending for Transparency
        glDepthMask(GL_FALSE); // Stop writing new depth values to the depth buffer
        glEnable(GL_BLEND);    // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
        //Reset Depth Mask for the next frame or subsequent opaque rendering
        glDepthMask(GL_TRUE); // Re-enable writing
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
            CEVec2(0.0f, uvHeightMax),       // bottom left
            CEVec2(uvWidthMax, uvHeightMax), // bottom right
            CEVec2(uvWidthMax, 0.0f),        // top right
            CEVec2(0.0f, 0.0f)               // top left
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
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        // TextureAsset uniforms
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);

        // Enable Blending for Transparency
        glDepthMask(GL_FALSE); // Stop writing new depth values to the depth buffer
        glEnable(GL_BLEND);    // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
        //Reset Depth Mask for the next frame or subsequent opaque rendering
        glDepthMask(GL_TRUE); // Re-enable writing
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
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        model = glm::translate(model, glm::vec3(-centroid.x, -centroid.y, -centroid.z));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm->SetUniform("uModel", model);
        renderer.shdr_pgm->SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm->SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);

        // Enable Blending for Transparency
        glDepthMask(GL_FALSE); // Stop writing new depth values to the depth buffer
        glEnable(GL_BLEND);    // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        renderer.Draw();
        glBindTextureUnit(0, 0);
        //Reset Depth Mask for the next frame or subsequent opaque rendering
        glDepthMask(GL_TRUE); // Re-enable writing
    }
}

