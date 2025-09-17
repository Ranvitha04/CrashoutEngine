/*!
@file        rendering.cpp
@author      ranvitha@digipen.edu
@date        09/12/2025
@brief       Implements the Rendering struct functions for OpenGL rendering.

This file implements the functions of the Component::Rendering struct declared in rendering.h.
It provides functionality for setting up vertex data, configuring VAO/VBO/EBO objects,
compiling and linking shaders, and rendering primitives.

Functions implemented:
- Setup_VAO(): Initializes vertex and color buffers, sets up a VAO, and configures attributes.
- Setup_Shdrpgm(): Compiles and links vertex and fragment shaders, validates the shader program.
- Draw(): Binds the shader program and VAO, and issues a draw call using glDrawElements.

This file uses CEVec3 for vertex positions and colors, GLSLShader for shader management,
and OpenGL 4.x core profile functions for buffer and vertex array management.
*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "rendering.h"
#include "managers/entitymanager.h"
#include <string>
#include "math/CEVec3.h"
#include "math/CEVec2.h"
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace Component {

     // Static member definition
Rendering& Rendering::Get() {
    static Rendering staticInstance;
    return staticInstance;
}

void Rendering::Initialize() {
    Get().Setup_Shdrpgm();
}

void Rendering::Cleanup() {
    Rendering& instance = Get();
    if (instance.vaoid != 0) {
        glDeleteVertexArrays(1, &instance.vaoid);
        instance.vaoid = 0;
    }
    if (instance.vbo_hdl != 0) {
        glDeleteBuffers(1, &instance.vbo_hdl);
        instance.vbo_hdl = 0;
    }
    if (instance.ebo_hdl != 0) {
        glDeleteBuffers(1, &instance.ebo_hdl);
        instance.ebo_hdl = 0;
    }
}

void Rendering::Setup_VAO() {
    std::array<CEVec3, 4> pos_vtx{
        CEVec3(-0.5f, -0.5f, 0.0f), // Bottom-left
        CEVec3(0.5f, -0.5f, 0.0f),  // Bottom-right
        CEVec3(0.5f, 0.5f, 0.0f),   // Top-right
        CEVec3(-0.5f, 0.5f, 0.0f)   // Top-left
    };

    std::array<CEVec3, 4> clr_vtx{
        CEVec3(1.0f, 1.0f, 1.0f), // white
        CEVec3(1.0f, 0.0f, 0.0f), // red
        CEVec3(0.0f, 1.0f, 0.0f), // green
        CEVec3(0.0f, 0.0f, 1.0f)  // blue
    };

    GLsizei position_data_offset = 0;
    GLsizei position_attribute_size = sizeof(CEVec3);
    GLsizei position_data_size = position_attribute_size * static_cast<GLsizei>(pos_vtx.size());
    GLsizei color_data_offset = position_data_size;
    GLsizei color_attribute_size = sizeof(CEVec3);
    GLsizei color_data_size = color_attribute_size * static_cast<GLsizei>(clr_vtx.size());

    // Create and populate VBO
    glCreateBuffers(1, &vbo_hdl);
    glNamedBufferStorage(vbo_hdl, position_data_size + color_data_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferSubData(vbo_hdl, position_data_offset, position_data_size, pos_vtx.data());
    glNamedBufferSubData(vbo_hdl, color_data_offset, color_data_size, clr_vtx.data());

    // Create VAO
    glCreateVertexArrays(1, &vaoid);

    // Position attribute
    glEnableVertexArrayAttrib(vaoid, 0);
    glVertexArrayVertexBuffer(vaoid, 0, vbo_hdl, position_data_offset, position_attribute_size);
    glVertexArrayAttribFormat(vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vaoid, 0, 0);

    // Color attribute
    glEnableVertexArrayAttrib(vaoid, 1);
    glVertexArrayVertexBuffer(vaoid, 1, vbo_hdl, color_data_offset, color_attribute_size);
    glVertexArrayAttribFormat(vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vaoid, 1, 1);

    primitive_type = GL_TRIANGLE_FAN;

    // Index buffer for quad using triangle fan
    std::array<GLushort, 4> idx_vtx{ 0, 1, 2, 3 };
    idx_elem_cnt = static_cast<GLuint>(idx_vtx.size());

    GLuint ebo_hdl_temp;
    glCreateBuffers(1, &ebo_hdl_temp);
    glNamedBufferStorage(ebo_hdl_temp, sizeof(GLushort) * idx_elem_cnt,
        reinterpret_cast<GLvoid*>(idx_vtx.data()), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayElementBuffer(vaoid, ebo_hdl_temp);
    ebo_hdl = ebo_hdl_temp;

    glBindVertexArray(0);
}

void Rendering::Setup_Shdrpgm() {
    const std::string Shader = "../../../projects/CrashoutEngine/shaders/CrashoutShader.frag";
    const std::string Vertex = "../../../projects/CrashoutEngine/shaders/CrashoutShader.vert";

    // Automation hook. [!WARNING!] Do not alter/remove this
    AUTOMATION_HOOK_SHADER(shdr_pgm, shdr_files);

    if (!shdr_pgm.CompileShaderFromFile(GL_VERTEX_SHADER, Vertex)) {
        std::cout << "Vertex Shader failed to compile: ";
        std::cout << shdr_pgm.GetLog() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (!shdr_pgm.CompileShaderFromFile(GL_FRAGMENT_SHADER, Shader)) {
        std::cout << "Fragment Shader failed to compile: ";
        std::cout << shdr_pgm.GetLog() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (!shdr_pgm.Link()) {
        std::cout << "Shader program failed to link!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (!shdr_pgm.Validate()) {
        std::cout << "Shader program failed to validate!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void Rendering::SetupBuffers(const void* vertices, size_t vertex_size,
    const void* colors, size_t color_size,
    const void* indices, size_t index_count,
    GLenum primitive) {
    // Clean up existing buffers if they exist
    if (vaoid != 0) {
        glDeleteVertexArrays(1, &vaoid);
    }
    if (vbo_hdl != 0) {
        glDeleteBuffers(1, &vbo_hdl);
    }
    if (ebo_hdl != 0) {
        glDeleteBuffers(1, &ebo_hdl);
    }

    // Create new VBO
    glCreateBuffers(1, &vbo_hdl);
    glNamedBufferStorage(vbo_hdl, vertex_size + color_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferSubData(vbo_hdl, 0, vertex_size, vertices);
    glNamedBufferSubData(vbo_hdl, vertex_size, color_size, colors);

    // Create new VAO
    glCreateVertexArrays(1, &vaoid);

    // Position attribute (location 0)
    glEnableVertexArrayAttrib(vaoid, 0);
    glVertexArrayVertexBuffer(vaoid, 0, vbo_hdl, 0, sizeof(CEVec3));
    glVertexArrayAttribFormat(vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vaoid, 0, 0);

    // Color attribute (location 1)
    glEnableVertexArrayAttrib(vaoid, 1);
    glVertexArrayVertexBuffer(vaoid, 1, vbo_hdl, vertex_size, sizeof(CEVec3));
    glVertexArrayAttribFormat(vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vaoid, 1, 1);

    // Create EBO if indices provided
    if (indices && index_count > 0) {
        glCreateBuffers(1, &ebo_hdl);
        glNamedBufferStorage(ebo_hdl, sizeof(GLushort) * index_count, indices, GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayElementBuffer(vaoid, ebo_hdl);
        idx_elem_cnt = static_cast<GLuint>(index_count);
    }

    primitive_type = primitive;
}

void Rendering::Draw() {
    shdr_pgm.Use();
    glBindVertexArray(vaoid);
    glDrawElements(primitive_type, idx_elem_cnt, GL_UNSIGNED_SHORT, NULL);
    glBindVertexArray(0);
    shdr_pgm.UnUse();
}

}


