#ifndef RENDERING_H
#define RENDERING_H

/*!
@file        rendering.h
@author      ranvitha@digipen.edu
@date        09/12/2025

This file declares the Rendering struct, which encapsulates the
resources and operations needed to render primitives in OpenGL.
It defines handles for VAO, VBO, and EBO objects, the associated
GLSL shader program, and the primitive type to be drawn.

The struct provides functions to:
- Initialize and configure a Vertex Array Object (Setup_VAO)
- Compile and link a shader program (Setup_Shdrpgm)
- Issue draw calls to render primitives (Draw)

An external instance `model` is also declared for shared use in
rendering pipelines.
*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */


#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstddef>
#include "shader.h"	
#include "managers/entitymanager.h"

namespace Component {


    class Rendering {
    public:
        
        Rendering() : vaoid(0), vbo_hdl(0), ebo_hdl(0), primitive_type(GL_TRIANGLES), 
                      idx_elem_cnt(0), tex_vbo_hdl(0), texobj(0), current_vbo_size(0), 
                      current_ebo_size(0), mapped_vbo_ptr(nullptr), mapped_ebo_ptr(nullptr), 
                      buffers_initialized(false) {}
        
        static Rendering& Get();
        static void Initialize();
        static void Cleanup();

        
        Rendering(const Rendering&) = delete;
        Rendering& operator=(const Rendering&) = delete;

        // OpenGL resources
        GLuint vaoid{ 0 };
        GLuint vbo_hdl{ 0 };
        GLuint ebo_hdl{ 0 };
        GLuint primitive_type{ GL_TRIANGLES };
        GLuint idx_elem_cnt{ 0 };
        GLuint tex_vbo_hdl{ 0 };
        GLuint texobj{ 0 };
        GLSLShader shdr_pgm;
        
        
        size_t current_vbo_size{ 0 };
        size_t current_ebo_size{ 0 };
        
        
        void* mapped_vbo_ptr{ nullptr };
        void* mapped_ebo_ptr{ nullptr };
        
        // Buffer reuse or not bool
        bool buffers_initialized{ false };

    private:
        // Performance optimization members
        struct BufferPool {
            std::vector<GLuint> available_vbos;
            std::vector<GLuint> available_vaos;
            std::vector<GLuint> available_ebos;
            size_t max_pool_size = 100;
        };
        static BufferPool buffer_pool;

    public:

        
        void Setup_VAO();
        void Setup_Shdrpgm();
        void Draw();

        // Dynamic rendering 
        void SetupBuffers(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* indices, size_t index_count,
            GLenum primitive);

        
        void SetupTexturedBuffers(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* tex_coords, size_t tex_coord_size,
            const void* indices, size_t index_count,
            GLenum primitive);

        // 
        void SetupBuffersOptimized(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* indices, size_t index_count,
            GLenum primitive);
            
        void SetupTexturedBuffersOptimized(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* tex_coords, size_t tex_coord_size,
            const void* indices, size_t index_count,
            GLenum primitive);
            
        void UpdateBuffersInPlace(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* indices, size_t index_count);
            
        // Buffer pool management
        GLuint GetPooledVBO();
        GLuint GetPooledVAO();
        GLuint GetPooledEBO();
        void ReturnToPool(GLuint vbo, GLuint vao, GLuint ebo);
        
        
        void InitializePersistentBuffers(size_t max_vertex_size, size_t max_index_size);
        void CleanupPersistentBuffers();


    };

}


#endif
