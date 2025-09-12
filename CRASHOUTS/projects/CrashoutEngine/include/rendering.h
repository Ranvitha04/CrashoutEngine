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


#include "shader.h"	
#include "entitymanager.h"
#include <string>
#include <GL/glew.h>


namespace Component {

	struct Rendering {

		GLenum primitive_type = GL_LINE; 
		GLSLShader shdr_pgm;
		GLuint vaoid = 0; 
		GLuint vbo_hdl = 0; 
		GLuint ebo_hdl = 0;
		GLuint idx_elem_cnt = 0;   

		void Setup_VAO();
		void Setup_Shdrpgm();
		void Draw();

	};

	extern Rendering model;




}


#endif
