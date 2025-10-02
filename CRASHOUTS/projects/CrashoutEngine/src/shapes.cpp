
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
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "resources/textures.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


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

        // Use optimized buffer setup for maximum performance
        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
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

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f); // No tint modification

        renderer.Draw();
    }

    void Shapes::CreateInstancedSquares(const CEVec3* positions, const CEVec3* colors, 
                                      const float* rotations, const float* scales, 
                                      int count, float baseSize) {
        if (count <= 0) return;
        
        Component::Rendering& renderer = Component::Rendering::Get();



        // Define base square vertices centered at origin
        float halfSize = baseSize / 2.0f;
        std::array<CEVec3, 4> base_vertices{
            CEVec3(-halfSize, -halfSize, 0.0f),
            CEVec3(halfSize, -halfSize, 0.0f),
            CEVec3(halfSize, halfSize, 0.0f),
            CEVec3(-halfSize, halfSize, 0.0f)
        };

        // Create vertex and color data for all instances
        std::vector<CEVec3> all_vertices;
        std::vector<CEVec3> all_colors;
        std::vector<GLushort> all_indices;
        
        all_vertices.reserve(count * 4);
        all_colors.reserve(count * 4);
        all_indices.reserve(count * 6); // 6 indices per quad (2 triangles)

        for (int i = 0; i < count; ++i) {
            // Transform vertices for this instance
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(positions[i].x, positions[i].y, positions[i].z));
            model = glm::rotate(model, glm::radians(rotations[i]), glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(scales[i], scales[i], 1.0f));

            // Transform each vertex
            for (int v = 0; v < 4; ++v) {
                glm::vec4 transformed = model * glm::vec4(base_vertices[v].x, base_vertices[v].y, base_vertices[v].z, 1.0f);
                all_vertices.push_back(CEVec3(transformed.x, transformed.y, transformed.z));
                all_colors.push_back(colors[i]);
            }

            // Add indices for this quad as two triangles (offset by current vertex count)
            GLushort base_index = static_cast<GLushort>(i * 4);
            // First triangle: 0, 1, 2
            all_indices.push_back(base_index + 0);
            all_indices.push_back(base_index + 1);
            all_indices.push_back(base_index + 2);
            // Second triangle: 0, 2, 3
            all_indices.push_back(base_index + 0);
            all_indices.push_back(base_index + 2);
            all_indices.push_back(base_index + 3);
        }

        if (renderer.vaoid != 0) {
            glDeleteVertexArrays(1, &renderer.vaoid);
            renderer.vaoid = 0;
        }

      
        renderer.SetupBuffersOptimized(
            all_vertices.data(), all_vertices.size() * sizeof(CEVec3),
            all_colors.data(), all_colors.size() * sizeof(CEVec3),
            all_indices.data(), all_indices.size(),
            GL_TRIANGLES  // Use GL_TRIANGLES for modern OpenGL compatibility
        );
    

        renderer.shdr_pgm.Use();
        
        // Verify the shader program is alive
        GLint current_program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

        // Use identity matrices since we've already transformed the vertices
        glm::mat4 identity = glm::mat4(1.0f);
        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", identity);
        renderer.shdr_pgm.SetUniform("uView", identity);
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        // Ensure texture state is properly reset for non-textured rendering
        glBindTextureUnit(0, 0); // Unbind any texture from unit 0
        
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors

        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0); // Set texture sampler to unit 0

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f); // No tint

        // Upload dynamic vertex and color data to GPU
        GLsizei position_data_size = sizeof(CEVec3) * static_cast<GLsizei>(all_vertices.size());
        GLsizei color_data_size = sizeof(CEVec3) * static_cast<GLsizei>(all_colors.size());
        GLsizei total_data_size = position_data_size + color_data_size;
        
        // Resize buffer if needed
        glNamedBufferData(renderer.vbo_hdl, total_data_size, nullptr, GL_DYNAMIC_DRAW);
        
        // Upload vertex positions
        glNamedBufferSubData(renderer.vbo_hdl, 0, position_data_size, all_vertices.data());
        
        // Upload vertex colors
        glNamedBufferSubData(renderer.vbo_hdl, position_data_size, color_data_size, all_colors.data());
        
        // Update VAO to use new data layout
        glVertexArrayVertexBuffer(renderer.vaoid, 0, renderer.vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayVertexBuffer(renderer.vaoid, 1, renderer.vbo_hdl, position_data_size, sizeof(CEVec3));
        
        // Upload index data
        GLsizei index_data_size = sizeof(GLushort) * static_cast<GLsizei>(all_indices.size());
        glNamedBufferData(renderer.ebo_hdl, index_data_size, all_indices.data(), GL_DYNAMIC_DRAW);
        
        // Update element count for drawing
        renderer.idx_elem_cnt = static_cast<GLuint>(all_indices.size());
        renderer.primitive_type = GL_TRIANGLES;
        
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

        // Use optimized buffer setup for maximum performance
        renderer.SetupBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
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

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
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

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
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

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0); // Use vertex colors, not texture

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm.GetHandle(), "u_tint");
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
            float angle = 2.0f * M_PI * i / segments;
            float x = radius * cos(angle);
            float y = radius * sin(angle);
            pos_vtx.push_back(CEVec3(x, y, 0.0f));
            clr_vtx.push_back(color);
        }

        // Generate indices for triangle fan
        for (int i = 0; i < segments; ++i) {
            idx_vtx.push_back(0);           // Center vertex
            idx_vtx.push_back(i + 1);       // Current perimeter vertex
            idx_vtx.push_back(i + 2);       // Next perimeter vertex
        }

        // Use optimized buffer setup for maximum performance
        renderer.SetupBuffersOptimized(pos_vtx.data(), pos_vtx.size() * sizeof(CEVec3),
            clr_vtx.data(), clr_vtx.size() * sizeof(CEVec3),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        renderer.shdr_pgm.Use();

        // Create model matrix for world position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        glm::mat4 projection = CreateCenteredWorldProjection();

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

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

        
        std::array<CEVec3, 4> clr_vtx{ tint, tint, tint, tint };

        float uvWidthMax = uvSize.x;
        float uvHeightMax = uvSize.y;

        // Texture coordinate data
        std::array<CEVec2, 4> tex_vtx{
            CEVec2(0.0f, 1.0f), // bottom left 
            CEVec2(1.0f, 1.0f), // bottom right 
            CEVec2(1.0f, 0.0f), // top right 
            CEVec2(0.0f, 0.0f)
        };

        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 }; // Two triangles

        // Use optimized textured buffer setup for maximum performance
        renderer.SetupTexturedBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
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
            CEVec2(0.0f, 1.0f), CEVec2(1.0f, 1.0f),
            CEVec2(1.0f, 0.0f), CEVec2(0.0f, 0.0f)
        };

        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };

        // Use optimized textured buffer setup
        renderer.SetupTexturedBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
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

        // Texture uniforms
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

        // Use optimized textured buffer 
        renderer.SetupTexturedBuffersOptimized(pos_vtx.data(), sizeof(pos_vtx),
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

