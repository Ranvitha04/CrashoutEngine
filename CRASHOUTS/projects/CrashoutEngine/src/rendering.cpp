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
#include <cstring>
#include <cstdlib>
#include <GL/glew.h>

namespace Component {

    // Static member definitions
    Rendering::BufferPool Rendering::buffer_pool;

    Rendering& Rendering::Get() {
        static Rendering staticInstance;
        return staticInstance;
    }

    // Buffer pool management
    GLuint Rendering::GetPooledVBO() {
        if (!buffer_pool.available_vbos.empty()) {
            GLuint vbo = buffer_pool.available_vbos.back();
            buffer_pool.available_vbos.pop_back();
            return vbo;
        }
        
        GLuint vbo;
        glCreateBuffers(1, &vbo);
        return vbo;
    }

    GLuint Rendering::GetPooledVAO() {
        if (!buffer_pool.available_vaos.empty()) {
            GLuint vao = buffer_pool.available_vaos.back();
            buffer_pool.available_vaos.pop_back();
            return vao;
        }
        
        GLuint vao;
        glCreateVertexArrays(1, &vao);
        return vao;
    }

    GLuint Rendering::GetPooledEBO() {
        if (!buffer_pool.available_ebos.empty()) {
            GLuint ebo = buffer_pool.available_ebos.back();
            buffer_pool.available_ebos.pop_back();
            return ebo;
        }
        
        GLuint ebo;
        glCreateBuffers(1, &ebo);
        return ebo;
    }

    void Rendering::ReturnToPool(GLuint vbo, GLuint vao, GLuint ebo) {
        if (buffer_pool.available_vbos.size() < buffer_pool.max_pool_size && vbo != 0) {
            buffer_pool.available_vbos.push_back(vbo);
        } else if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }
        
        if (buffer_pool.available_vaos.size() < buffer_pool.max_pool_size && vao != 0) {
            buffer_pool.available_vaos.push_back(vao);
        } else if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
        
        if (buffer_pool.available_ebos.size() < buffer_pool.max_pool_size && ebo != 0) {
            buffer_pool.available_ebos.push_back(ebo);
        } else if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
        }
    }

    void Rendering::Initialize() {
        Get().Setup_Shdrpgm();
    }

    void Rendering::Cleanup() {
        Rendering& instance = Get();
        
        // Clean up persistent mappings first
        instance.CleanupPersistentBuffers();
        
        if (instance.vaoid != 0) {
            glDeleteVertexArrays(1, &instance.vaoid);
            instance.vaoid = 0;
        }
        if (instance.vbo_hdl != 0) {
            glDeleteBuffers(1, &instance.vbo_hdl);
            instance.vbo_hdl = 0;
        }
        if (instance.tex_vbo_hdl != 0) {
            glDeleteBuffers(1, &instance.tex_vbo_hdl);
            instance.tex_vbo_hdl = 0;
        }
        if (instance.ebo_hdl != 0) {
            glDeleteBuffers(1, &instance.ebo_hdl);
            instance.ebo_hdl = 0;
        }
        
        // Clean up buffer pool
        for (GLuint vbo : buffer_pool.available_vbos) {
            glDeleteBuffers(1, &vbo);
        }
        for (GLuint vao : buffer_pool.available_vaos) {
            glDeleteVertexArrays(1, &vao);
        }
        for (GLuint ebo : buffer_pool.available_ebos) {
            glDeleteBuffers(1, &ebo);
        } 

        buffer_pool.available_vbos.clear();
        buffer_pool.available_vaos.clear();
        buffer_pool.available_ebos.clear();
    }

    void Rendering::InitializePersistentBuffers(size_t max_vertex_size, size_t max_index_size) {
        
        if (vbo_hdl != 0) {
            CleanupPersistentBuffers();
        }
        
        glCreateBuffers(1, &vbo_hdl);
        glNamedBufferStorage(vbo_hdl, max_vertex_size, nullptr,
            GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
        
        mapped_vbo_ptr = glMapNamedBufferRange(vbo_hdl, 0, max_vertex_size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
        
        current_vbo_size = max_vertex_size;
        
        if (max_index_size > 0) {
            glCreateBuffers(1, &ebo_hdl);
            glNamedBufferStorage(ebo_hdl, max_index_size, nullptr,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            
            mapped_ebo_ptr = glMapNamedBufferRange(ebo_hdl, 0, max_index_size,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
            
            current_ebo_size = max_index_size;
        }
        
        buffers_initialized = true;
    }

    void Rendering::CleanupPersistentBuffers() {
        if (mapped_vbo_ptr) {
            glUnmapNamedBuffer(vbo_hdl);
            mapped_vbo_ptr = nullptr;
        }
        if (mapped_ebo_ptr) {
            glUnmapNamedBuffer(ebo_hdl);
            mapped_ebo_ptr = nullptr;
        }
        buffers_initialized = false;
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

        // Variable needed for automation hook
        std::vector<std::pair<GLenum, std::string>> shdr_files;

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


    void Rendering::SetupBuffersOptimized(const void* vertices, size_t vertex_size,
        const void* colors, size_t color_size,
        const void* indices, size_t index_count,
        GLenum primitive) {
        
        size_t total_vertex_size = vertex_size + color_size;
        size_t total_index_size = sizeof(GLushort) * index_count;
        
        // reuse existing buffers
        bool can_reuse_vbo = (buffers_initialized && total_vertex_size <= current_vbo_size);
        bool can_reuse_ebo = (buffers_initialized && total_index_size <= current_ebo_size);
        
        if (!can_reuse_vbo) {
            // Need new VBO - clean up old one if exists
            if (vbo_hdl != 0) {
                if (mapped_vbo_ptr) {
                    glUnmapNamedBuffer(vbo_hdl);
                    mapped_vbo_ptr = nullptr;
                }
                glDeleteBuffers(1, &vbo_hdl);
            }
            
            // Create new VBO with extra space 
            size_t padded_size = total_vertex_size * 1.5f; 
            glCreateBuffers(1, &vbo_hdl);
            glNamedBufferStorage(vbo_hdl, padded_size, nullptr, 
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            
            
            mapped_vbo_ptr = glMapNamedBufferRange(vbo_hdl, 0, padded_size,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
            
            current_vbo_size = padded_size;
        }
        
        if (!can_reuse_ebo && indices && index_count > 0) {
            // Need new EBO
            if (ebo_hdl != 0) {
                if (mapped_ebo_ptr) {
                    glUnmapNamedBuffer(ebo_hdl);
                    mapped_ebo_ptr = nullptr;
                }
                glDeleteBuffers(1, &ebo_hdl);
            }
            
            size_t padded_ebo_size = total_index_size * 1.5f;
            glCreateBuffers(1, &ebo_hdl);
            glNamedBufferStorage(ebo_hdl, padded_ebo_size, nullptr,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            
            mapped_ebo_ptr = glMapNamedBufferRange(ebo_hdl, 0, padded_ebo_size,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
            
            current_ebo_size = padded_ebo_size;
        }
        
         
        if (mapped_vbo_ptr) {
            std::memcpy(mapped_vbo_ptr, vertices, vertex_size);
            std::memcpy(static_cast<char*>(mapped_vbo_ptr) + vertex_size, colors, color_size);
            glFlushMappedNamedBufferRange(vbo_hdl, 0, total_vertex_size);
        }
        
        if (mapped_ebo_ptr && indices && index_count > 0) {
            std::memcpy(mapped_ebo_ptr, indices, total_index_size);
            glFlushMappedNamedBufferRange(ebo_hdl, 0, total_index_size);
            idx_elem_cnt = static_cast<GLuint>(index_count);
        }
        
        // Only create VAO 
        if (vaoid == 0) {
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
            
            if (ebo_hdl != 0) {
                glVertexArrayElementBuffer(vaoid, ebo_hdl);
            }
        }
        
        primitive_type = primitive;
        buffers_initialized = true;
    }

    
    void Rendering::UpdateBuffersInPlace(const void* vertices, size_t vertex_size,
        const void* colors, size_t color_size,
        const void* indices, size_t index_count) {
        
        if (!buffers_initialized || !mapped_vbo_ptr) {
            return; 
        }
        
        size_t total_vertex_size = vertex_size + color_size;
        size_t total_index_size = sizeof(GLushort) * index_count;
        
        // Check if data fits in existing buffers
        if (total_vertex_size <= current_vbo_size) {
            std::memcpy(mapped_vbo_ptr, vertices, vertex_size);
            std::memcpy(static_cast<char*>(mapped_vbo_ptr) + vertex_size, colors, color_size);
            glFlushMappedNamedBufferRange(vbo_hdl, 0, total_vertex_size);
        }
        
        if (mapped_ebo_ptr && indices && total_index_size <= current_ebo_size) {
            std::memcpy(mapped_ebo_ptr, indices, total_index_size);
            glFlushMappedNamedBufferRange(ebo_hdl, 0, total_index_size);
            idx_elem_cnt = static_cast<GLuint>(index_count);
        }
    }

    
    void Rendering::SetupTexturedBuffersOptimized(const void* vertices, size_t vertex_size,
        const void* colors, size_t color_size,
        const void* tex_coords, size_t tex_coord_size,
        const void* indices, size_t index_count,
        GLenum primitive) {
        
        size_t total_vertex_size = vertex_size + color_size + tex_coord_size;
        size_t total_index_size = sizeof(GLushort) * index_count;
        
        
        bool can_reuse_vbo = (vbo_hdl != 0) && (total_vertex_size <= current_vbo_size);
        
        if (!can_reuse_vbo) {
            // Need new VBO
            if (vbo_hdl != 0) {
                if (mapped_vbo_ptr) {
                    glUnmapNamedBuffer(vbo_hdl);
                    mapped_vbo_ptr = nullptr;
                }
                glDeleteBuffers(1, &vbo_hdl);
            }
            
            size_t padded_vbo_size = total_vertex_size * 1.5f;
            glCreateBuffers(1, &vbo_hdl);
            glNamedBufferStorage(vbo_hdl, padded_vbo_size, nullptr,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            
            mapped_vbo_ptr = glMapNamedBufferRange(vbo_hdl, 0, padded_vbo_size,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
            
            current_vbo_size = padded_vbo_size;
        }
        
        // Check if can reuse existing EBO
        bool can_reuse_ebo = (ebo_hdl != 0) && (total_index_size <= current_ebo_size);
        
        if (!can_reuse_ebo && indices && index_count > 0) {
            // Need new EBO
            if (ebo_hdl != 0) {
                if (mapped_ebo_ptr) {
                    glUnmapNamedBuffer(ebo_hdl);
                    mapped_ebo_ptr = nullptr;
                }
                glDeleteBuffers(1, &ebo_hdl);
            }
            
            float padded_ebo_size = total_index_size * 1.5f;
            glCreateBuffers(1, &ebo_hdl);
            glNamedBufferStorage(ebo_hdl, padded_ebo_size, nullptr,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            
            mapped_ebo_ptr = glMapNamedBufferRange(ebo_hdl, 0, padded_ebo_size,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
            
            current_ebo_size = static_cast<size_t>(padded_ebo_size);
        }
        
        // Ultra-fast data upload
        if (mapped_vbo_ptr) {
            char* ptr = static_cast<char*>(mapped_vbo_ptr);
            std::memcpy(ptr, vertices, vertex_size);
            ptr += vertex_size;
            std::memcpy(ptr, colors, color_size);
            ptr += color_size;
            std::memcpy(ptr, tex_coords, tex_coord_size);
            glFlushMappedNamedBufferRange(vbo_hdl, 0, total_vertex_size);
        }
        
        if (mapped_ebo_ptr && indices && index_count > 0) {
            std::memcpy(mapped_ebo_ptr, indices, total_index_size);
            glFlushMappedNamedBufferRange(ebo_hdl, 0, total_index_size);
            idx_elem_cnt = static_cast<GLuint>(index_count);
        }
        
        // Create VAO if not exists
        if (vaoid == 0) {
            glCreateVertexArrays(1, &vaoid);
        }
        
        
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
        
        // Texture coordinate attribute (location 2)
        glEnableVertexArrayAttrib(vaoid, 2);
        glVertexArrayVertexBuffer(vaoid, 2, vbo_hdl, vertex_size + color_size, sizeof(CEVec2));
        glVertexArrayAttribFormat(vaoid, 2, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoid, 2, 2);
        
        // Update element buffer if needed
        if (ebo_hdl != 0) {
            glVertexArrayElementBuffer(vaoid, ebo_hdl);
        }
        
        primitive_type = primitive;
        buffers_initialized = true;
    }

    void Rendering::SetupTexturedBuffers(const void* vertices, size_t vertex_size,
        const void* colors, size_t color_size,
        const void* tex_coords, size_t tex_coord_size,
        const void* indices, size_t index_count,
        GLenum primitive) {
        // Clean up existing buffers if they exist
        if (vaoid != 0) {
            glDeleteVertexArrays(1, &vaoid);
        }
        if (vbo_hdl != 0) {
            glDeleteBuffers(1, &vbo_hdl);
        }
        if (tex_vbo_hdl != 0) {
            glDeleteBuffers(1, &tex_vbo_hdl);
        }
        if (ebo_hdl != 0) {
            glDeleteBuffers(1, &ebo_hdl);
        }

        // Create main VBO for positions and colors
        glCreateBuffers(1, &vbo_hdl);
        glNamedBufferStorage(vbo_hdl, vertex_size + color_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferSubData(vbo_hdl, 0, vertex_size, vertices);
        glNamedBufferSubData(vbo_hdl, vertex_size, color_size, colors);

        // Create separate VBO for texture coordinates
        glCreateBuffers(1, &tex_vbo_hdl);
        glNamedBufferStorage(tex_vbo_hdl, tex_coord_size, tex_coords, GL_DYNAMIC_STORAGE_BIT);

        // Create VAO
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

        // Texture coordinate attribute (location 2)
        glEnableVertexArrayAttrib(vaoid, 2);
        glVertexArrayVertexBuffer(vaoid, 2, tex_vbo_hdl, 0, sizeof(CEVec2));
        glVertexArrayAttribFormat(vaoid, 2, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoid, 2, 2);

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





