#pragma once
/*!
@file        glslshader.h
@author      ranvitha@digipen.edu
@date        09/12/2025

This header file declares the GLSLShader class which provides a
C++ wrapper around OpenGL’s GLSL shader functionality. It includes
methods to compile shaders from source or files, link them into a
program object, validate execution in the current OpenGL state,
and manage shader program usage.

The class also exposes functions to bind vertex attributes, bind
fragment outputs, set uniform variables of various types, and
query active shader variables. This abstraction simplifies
shader management in rendering applications and ensures
safer handling of shader program lifecycles.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */


#include "math/CEVec2.h"
#include "math/CEVec3.h"
#include "components/namecomponent.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>


//#include "rendering.h"


namespace Component {

    using std::string;
    using std::vector;
    using std::pair;

    class GLSLShader {
        /*! GLSLShader class.
        */
    public:

        // default ctor required to initialize GLSLShader object to safe state
        GLSLShader() : pgm_handle(0), is_linked(GL_FALSE) { /* empty by design */ }

       
        GLboolean CompileLinkValidate(vector<pair<GLenum, string>>, bool compile_from_file = true);

     
        GLboolean CompileShaderFromFile(GLenum shader_type, const string& file_name);

     
        GLboolean CompileShaderFromString(GLenum shader_type, const string& shader_src);

        // log to data member log_string. 
        GLboolean Link();

        
        void Use();

        void UnUse();

      
        GLboolean Validate();

        
        GLuint GetHandle() const;

        
        GLboolean IsLinked() const;

        std::string GetLog() const;

       
        void BindAttribLocation(GLuint index, GLchar const* name);

       
        void BindFragDataLocation(GLuint color_number, GLchar const* name);

       
        void DeleteShaderProgram();

        void SetUniform(GLchar const* name, GLboolean val);
        void SetUniform(GLchar const* name, GLint val);
        void SetUniform(GLchar const* name, GLfloat val);
        void SetUniform(GLchar const* name, GLfloat x, GLfloat y);
        void SetUniform(GLchar const* name, GLfloat x, GLfloat y, GLfloat z);
        void SetUniform(GLchar const* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        void SetUniform(GLchar const* name, glm::vec2 const& val);
        void SetUniform(GLchar const* name, glm::vec3 const& val);
        void SetUniform(GLchar const* name, glm::vec4 const& val);
        void SetUniform(GLchar const* name, glm::mat3 const& val);
        void SetUniform(GLchar const* name, glm::mat4 const& val);

       
        void PrintActiveAttribs() const;

        
        void PrintActiveUniforms() const;

       
        GLint GetUniformLocation(GLchar const* name, bool exit_on_error = false);

        
        GLboolean FileExists(const string& file_name);

    private:
        enum ShaderType {
            VERTEX_SHADER = GL_VERTEX_SHADER,
            FRAGMENT_SHADER = GL_FRAGMENT_SHADER,
            GEOMETRY_SHADER = GL_GEOMETRY_SHADER,
            TESS_CONTROL_SHADER = GL_TESS_CONTROL_SHADER,
            TESS_EVALUATION_SHADER = GL_TESS_EVALUATION_SHADER,
            // ignore compute shader for now because it is not connected to
            // the graphics pipe
            // COMPUTE_SHADER = GL_COMPUTE_SHADER
        };

        GLuint pgm_handle = 0;  // handle to linked shader program object
        GLboolean is_linked = GL_FALSE; // has the program successfully linked?
        std::string log_string; // log for OpenGL compiler and linker messages
    };

#if USE_CSD2101_AUTOMATION == 1
    // This automation hook reads the shader from the submission tutorial's shader directory as a string literal.
    // It requires an automation script to convert the shader files from file format to string literal format.
    // After conversion, the file names must be changed to my-shader.vert and my-shader.frag.
    /*std::string const tutorial_vs = {
      #include "../shaders/my-shader.vert"
    };
    std::string const tutorial_fs = {
      #include "../shaders/my-shader.frag"
    };*/

    const ::std::string Shader = "../../../projects/CrashoutEngine/shaders/CrashoutShader.frag";
    const ::std::string Vertex = "../../../projects/CrashoutEngine/shaders/CrashoutShader.vert";

#define AUTOMATION_HOOK_SHADER(shdr_pgm, shdr_files) \
shdr_files.clear(); \
shdr_files.push_back(std::make_pair(GL_VERTEX_SHADER, tutorial_vs)); \
shdr_files.push_back(std::make_pair(GL_FRAGMENT_SHADER, tutorial_fs)); \
shdr_pgm.CompileLinkValidate(shdr_files, false); \
return;
#else
#define AUTOMATION_HOOK_SHADER(shdr_pgm, shdr_files) 
#endif

}
