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
#include <GL/glew.h>
#include "glapp.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Component {
    // Store the last created rendering components to keep them alive for at least one frame
    static std::vector<Component::Rendering*> activeRenderers;
    
    /*!
    @brief Creates a new rendering component and cleans up previously created
    renderers from earlier frames.
    @return Pointer to a new Rendering component stored in activeRenderers.
    */

    Component::Rendering* CreateRenderingComponent() {
        // Clean up old renderers (from previous frames)
        for (auto renderer : activeRenderers) {
            glDeleteVertexArrays(1, &renderer->vaoid);
            glDeleteBuffers(1, &renderer->vbo_hdl);
            glDeleteBuffers(1, &renderer->ebo_hdl);
            delete renderer;
        }
        activeRenderers.clear();
        
        // Create new renderer
        Component::Rendering* renderer = new Component::Rendering();
        activeRenderers.push_back(renderer);
        return renderer;
    }
    
    /*!
       @brief Releases all GPU resources (VAO, VBO, EBO) associated with
       previously created renderers.
    */
    void Shapes::Cleanup() {
        for (auto renderer : activeRenderers) {
            glDeleteVertexArrays(1, &renderer->vaoid);
            glDeleteBuffers(1, &renderer->vbo_hdl);
            glDeleteBuffers(1, &renderer->ebo_hdl);
            delete renderer;
        }
        activeRenderers.clear();
    }

    /*!
    @brief Creates and renders a square centered at a given position.
    @param position Center position of the square.
    @param size Length of the square’s sides.
    @param rotation Rotation angle (in degrees) applied around Z-axis.
    @param scale Uniform scaling factor.
    @param color RGB color applied to all vertices.
    */

    void Shapes::CreateSquare(const CEVec3& position, float size, float rotation, float scale, const CEVec3& color) {
        Component::Rendering* renderer = CreateRenderingComponent();

        // Define square vertices centered at origin (no aspect ratio adjustment)
        float halfSize = size / 2.0f;
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfSize, -halfSize, 0.0f),
            CEVec3(halfSize, -halfSize, 0.0f),
            CEVec3(halfSize, halfSize, 0.0f),
            CEVec3(-halfSize, halfSize, 0.0f)
        };


        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };

        // VBO
        glCreateBuffers(1, &renderer->vbo_hdl);
        GLsizei pos_size = static_cast<GLsizei>(sizeof(CEVec3) * pos_vtx.size());
        GLsizei color_size = static_cast < GLsizei>(sizeof(CEVec3) * clr_vtx.size());
        glNamedBufferStorage(renderer->vbo_hdl, pos_size + color_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferSubData(renderer->vbo_hdl, 0, pos_size, pos_vtx.data());
        glNamedBufferSubData(renderer->vbo_hdl, pos_size, color_size, clr_vtx.data());

        // VAO
        glCreateVertexArrays(1, &renderer->vaoid);
        glEnableVertexArrayAttrib(renderer->vaoid, 0);
        glVertexArrayVertexBuffer(renderer->vaoid, 0, renderer->vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 0, 0);

        glEnableVertexArrayAttrib(renderer->vaoid, 1);
        glVertexArrayVertexBuffer(renderer->vaoid, 1, renderer->vbo_hdl, pos_size, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 1, 1);

        renderer->primitive_type = GL_TRIANGLE_FAN;

        std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };
        renderer->idx_elem_cnt = static_cast<GLuint>(idx_vtx.size());
        glCreateBuffers(1, &renderer->ebo_hdl);
        glNamedBufferStorage(renderer->ebo_hdl, sizeof(GLushort) * renderer->idx_elem_cnt, idx_vtx.data(), GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayElementBuffer(renderer->vaoid, renderer->ebo_hdl);


        renderer->Setup_Shdrpgm();
        renderer->shdr_pgm.Use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        renderer->shdr_pgm.SetUniform("uModel", model);
        renderer->shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer->shdr_pgm.SetUniform("uProjection", glm::mat4(1.0f));

        renderer->Draw();
    }

    /*!
    @brief Creates and renders a line between two endpoints.
           Implemented as a thin rectangle with adjustable thickness.
    @param start Start point of the line.
    @param end End point of the line.
    @param thickness Line thickness (world units).
    @param rotation Additional rotation angle around the midpoint.
    @param color RGB color applied to the line.
    */


    void Shapes::CreateLine(const CEVec3& start, const CEVec3& end, float thickness, float rotation, float scale ,const CEVec3& color) {
        Component::Rendering* renderer = CreateRenderingComponent();

        // Direction and perpendicular vector
        CEVec3 dir = end - start;
        glm::vec3 gdir(dir.x, dir.y, dir.z);
        glm::vec3 normal(-gdir.y, gdir.x, 0.0f); // perpendicular vector
        normal = glm::normalize(normal) * (thickness / 2.0f);

        // 4 vertices for thick line (quad)
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(start.x + normal.x, start.y + normal.y, start.z),
            CEVec3(start.x - normal.x, start.y - normal.y, start.z),
            CEVec3(end.x - normal.x, end.y - normal.y, end.z),
            CEVec3(end.x + normal.x, end.y + normal.y, end.z)
        };

        // Same color for all vertices
        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };

        // VBO
        glCreateBuffers(1, &renderer->vbo_hdl);
        GLsizei pos_size = static_cast < GLsizei>(sizeof(CEVec3) * pos_vtx.size());
        GLsizei color_size = static_cast < GLsizei>(sizeof(CEVec3) * clr_vtx.size());
        glNamedBufferStorage(renderer->vbo_hdl, pos_size + color_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferSubData(renderer->vbo_hdl, 0, pos_size, pos_vtx.data());
        glNamedBufferSubData(renderer->vbo_hdl, pos_size, color_size, clr_vtx.data());

        // VAO
        glCreateVertexArrays(1, &renderer->vaoid);

        // Position attribute
        glEnableVertexArrayAttrib(renderer->vaoid, 0);
        glVertexArrayVertexBuffer(renderer->vaoid, 0, renderer->vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 0, 0);

        // Color attribute
        glEnableVertexArrayAttrib(renderer->vaoid, 1);
        glVertexArrayVertexBuffer(renderer->vaoid, 1, renderer->vbo_hdl, pos_size, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 1, 1);

        // Indices for 2 triangles
        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };
        renderer->idx_elem_cnt = static_cast<GLuint>(idx_vtx.size());
        glCreateBuffers(1, &renderer->ebo_hdl);
        glNamedBufferStorage(renderer->ebo_hdl, sizeof(GLushort) * renderer->idx_elem_cnt, idx_vtx.data(), GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayElementBuffer(renderer->vaoid, renderer->ebo_hdl);

        // Primitive type
        renderer->primitive_type = GL_TRIANGLES;

        // Center for transformations
        CEVec3 center = (start + end) / 2.0f;

        // Shader 

        //Links shdr prgm (vert + frag)
        renderer->Setup_Shdrpgm();
        renderer->shdr_pgm.Use();

        //Creating identity mat
        glm::mat4 model = glm::mat4(1.0f);

        //Translation
        model = glm::translate(model, glm::vec3(center.x, center.y, center.z));

        //Rotation
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));

        //Scaling
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        //Uploads the final mdl mat to shader as uniform variable uModel
        renderer->shdr_pgm.SetUniform("uModel", model);

        //View matrix
        renderer->shdr_pgm.SetUniform("uView", glm::mat4(1.0f));

        //Proj to identity mat
        renderer->shdr_pgm.SetUniform("uProjection", glm::mat4(1.0f));

        // Draw
        renderer->Draw();
    }

    /*!
    @brief Creates and renders a rectangle centered at a given position.
    @param position Center position of the rectangle.
    @param width Width of the rectangle.
    @param height Height of the rectangle.
    @param rotation Rotation angle (in degrees) applied around Z-axis.
    @param scale Uniform scaling factor.
    @param color RGB color applied to all vertices.
    */

    void Shapes::CreateRectangle(const CEVec3& position, float width, float height, float rotation, float scale, const CEVec3& color) {
        Component::Rendering* renderer = CreateRenderingComponent();

        // Get window dimensions and aspect ratio
        int windowWidth, windowHeight;
        glfwGetWindowSize(GLApp::ptr_window, &windowWidth, &windowHeight);
        float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

        // Define rectangle vertices centered at origin
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(-halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, -halfHeight, 0.0f),
            CEVec3(halfWidth, halfHeight, 0.0f),
            CEVec3(-halfWidth, halfHeight, 0.0f)
        };

        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };

        // VBO
        glCreateBuffers(1, &renderer->vbo_hdl);
        GLsizei pos_size = static_cast < GLsizei>(sizeof(CEVec3) * pos_vtx.size());
        GLsizei color_size = static_cast < GLsizei>(sizeof(CEVec3) * clr_vtx.size());
        glNamedBufferStorage(renderer->vbo_hdl, pos_size + color_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferSubData(renderer->vbo_hdl, 0, pos_size, pos_vtx.data());
        glNamedBufferSubData(renderer->vbo_hdl, pos_size, color_size, clr_vtx.data());

        // VAO
        glCreateVertexArrays(1, &renderer->vaoid);
        glEnableVertexArrayAttrib(renderer->vaoid, 0);
        glVertexArrayVertexBuffer(renderer->vaoid, 0, renderer->vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 0, 0);

        glEnableVertexArrayAttrib(renderer->vaoid, 1);
        glVertexArrayVertexBuffer(renderer->vaoid, 1, renderer->vbo_hdl, pos_size, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 1, 1);

        renderer->primitive_type = GL_TRIANGLE_FAN;

        // EBO
        std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };
        renderer->idx_elem_cnt = static_cast<GLuint>(idx_vtx.size());
        glCreateBuffers(1, &renderer->ebo_hdl);
        glNamedBufferStorage(renderer->ebo_hdl, sizeof(GLushort) * renderer->idx_elem_cnt, idx_vtx.data(), GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayElementBuffer(renderer->vaoid, renderer->ebo_hdl);

        // Shader 
        renderer->Setup_Shdrpgm();
        renderer->shdr_pgm.Use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        renderer->shdr_pgm.SetUniform("uModel", model);
        renderer->shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer->shdr_pgm.SetUniform("uProjection", glm::mat4(1.0f));

        renderer->Draw();
    }

    /*!
    @brief Creates and renders a triangle using three vertices.
    @param p1 First vertex of the triangle.
    @param p2 Second vertex of the triangle.
    @param p3 Third vertex of the triangle.
    @param rotation Rotation angle (in degrees) applied around centroid.
    @param scale Uniform scaling factor.
    @param color RGB color applied to all vertices.
    */

    void Shapes::CreateTriangle(const CEVec3& p1, const CEVec3& p2, const CEVec3& p3, float rotation, float scale, const CEVec3& color) {
        Component::Rendering* renderer = CreateRenderingComponent();

        // Define the triangle vertices
        std::array<CEVec3, 3> pos_vtx{
            CEVec3(p1.x, p1.y, p1.z),
            CEVec3(p2.x, p2.y, p2.z),
            CEVec3(p3.x, p3.y, p3.z)
        };


        // Set all vertices to the same color
        std::array<CEVec3, 3> clr_vtx{
            color, color, color
        };

        // VBO
        glCreateBuffers(1, &renderer->vbo_hdl);
        GLsizei pos_size = static_cast <GLsizei>(sizeof(CEVec3) * pos_vtx.size());
        GLsizei color_size = static_cast <GLsizei>(sizeof(CEVec3) * clr_vtx.size());
        glNamedBufferStorage(renderer->vbo_hdl, pos_size + color_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferSubData(renderer->vbo_hdl, 0, pos_size, pos_vtx.data());
        glNamedBufferSubData(renderer->vbo_hdl, pos_size, color_size, clr_vtx.data());

        // VAO
        glCreateVertexArrays(1, &renderer->vaoid);
        glEnableVertexArrayAttrib(renderer->vaoid, 0);
        glVertexArrayVertexBuffer(renderer->vaoid, 0, renderer->vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 0, 0);

        glEnableVertexArrayAttrib(renderer->vaoid, 1);
        glVertexArrayVertexBuffer(renderer->vaoid, 1, renderer->vbo_hdl, pos_size, sizeof(CEVec3));
        glVertexArrayAttribFormat(renderer->vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(renderer->vaoid, 1, 1);

        renderer->primitive_type = GL_TRIANGLES;

        // EBO
        std::array<GLushort, 3> idx_vtx{ 0, 1, 2 };
        renderer->idx_elem_cnt = static_cast<GLuint>(idx_vtx.size());
        glCreateBuffers(1, &renderer->ebo_hdl);
        glNamedBufferStorage(renderer->ebo_hdl, sizeof(GLushort) * renderer->idx_elem_cnt, idx_vtx.data(), GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayElementBuffer(renderer->vaoid, renderer->ebo_hdl);

        // Compute centroid for rotation
        CEVec3 centroid = (p1 + p2 + p3) / 3.0f;

        // Shader + uniforms
        renderer->Setup_Shdrpgm();
        renderer->shdr_pgm.Use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centroid.x, centroid.y, centroid.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        renderer->shdr_pgm.SetUniform("uModel", model);
        renderer->shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer->shdr_pgm.SetUniform("uProjection", glm::mat4(1.0f));

        renderer->Draw();
    }

}