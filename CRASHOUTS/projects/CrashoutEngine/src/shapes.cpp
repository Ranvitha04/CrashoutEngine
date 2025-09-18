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

namespace Component {

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

        // Setup buffers for this shape
        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        // Setup shader and draw
        renderer.shdr_pgm.Use();

        // Create transformation matrix
        glm::mat4 model = glm::mat4(1.0f);
        
        model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        float aspect = static_cast<float>(GLApp::width) / GLApp::height;
        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        renderer.Draw();
    }

    void Shapes::CreateLine(const CEVec3& start, const CEVec3& end, float thickness, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Direction and perpendicular vector
        CEVec3 dir = end - start;
        glm::vec3 gdir(dir.x, dir.y, dir.z);
        glm::vec3 normal(-gdir.y, gdir.x, 0.0f);
        normal = glm::normalize(normal) * (thickness / 2.0f);

        // 4 vertices for thick line (quad)
        std::array<CEVec3, 4> pos_vtx{
            CEVec3(start.x + normal.x, start.y + normal.y, start.z),
            CEVec3(start.x - normal.x, start.y - normal.y, start.z),
            CEVec3(end.x - normal.x, end.y - normal.y, end.z),
            CEVec3(end.x + normal.x, end.y + normal.y, end.z)
        };

        std::array<CEVec3, 4> clr_vtx{ color, color, color, color };
        std::array<GLushort, 6> idx_vtx{ 0, 1, 2, 0, 2, 3 };

        // Setup buffers
        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        // Center for transformations
        CEVec3 center = (start + end) / 2.0f;

        // Setup shader and draw
        renderer.shdr_pgm.Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(center.x, center.y, center.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        float aspect = static_cast<float>(GLApp::width) / GLApp::height;

        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        renderer.Draw();
    }

    void Shapes::CreateRectangle(const CEVec3& position, float width, float height, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

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
        std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };

        // Setup buffers
        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLE_FAN);

        // Setup shader and draw
        renderer.shdr_pgm.Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, position.z));
        model = model * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 0, 1));
        model = model * glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1.0f));

        float aspect = static_cast<float>(GLApp::width) / GLApp::height;

        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        renderer.Draw();
    }

    void Shapes::CreateTriangle(const CEVec3& p1, const CEVec3& p2, const CEVec3& p3, float rotation, float scale, const CEVec3& color) {
        Component::Rendering& renderer = Component::Rendering::Get();

        // Define the triangle vertices
        std::array<CEVec3, 3> pos_vtx{
            CEVec3(p1.x, p1.y, p1.z),
            CEVec3(p2.x, p2.y, p2.z),
            CEVec3(p3.x, p3.y, p3.z)
        };

        std::array<CEVec3, 3> clr_vtx{ color, color, color };
        std::array<GLushort, 3> idx_vtx{ 0, 1, 2 };

        // Setup buffers
        renderer.SetupBuffers(pos_vtx.data(), sizeof(pos_vtx),
            clr_vtx.data(), sizeof(clr_vtx),
            idx_vtx.data(), idx_vtx.size(),
            GL_TRIANGLES);

        // Compute centroid for rotation
        CEVec3 centroid = (p1 + p2 + p3) / 3.0f;

        // Setup shader and draw
        renderer.shdr_pgm.Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centroid.x, centroid.y, centroid.z));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        float aspect = static_cast<float>(GLApp::width) / GLApp::height;

        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

        renderer.shdr_pgm.SetUniform("uModel", model);
        renderer.shdr_pgm.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm.SetUniform("uProjection", projection);

        renderer.Draw();
    }

}