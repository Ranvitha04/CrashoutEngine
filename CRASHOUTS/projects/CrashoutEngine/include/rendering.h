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
#include "shader.h"	
#include "managers/entitymanager.h"
#include "resources/textures.h"

namespace Component {


    class Rendering {
    public:
        
        Rendering() = default;
        
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


    };

}


#endif
