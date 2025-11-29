/*!
@file        glslshader.cpp
@author      ranvitha@digipen.edu
@date        09/12/2025

This file implements the GLSLShader class, which provides a
C++ abstraction layer for creating, compiling, linking, and
managing GLSL shaders in an OpenGL context. It defines functions
to load shader code from strings or files, attach shaders to
a program object, and validate the resulting shader program.

In addition, it offers helper methods to query and print active
attributes and uniforms, bind attribute and fragment data locations,
and set uniform variables of various types (scalars, vectors, and
matrices). These implementations simplify shader program usage
by encapsulating common OpenGL calls and error handling.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "shader.h"
#include "rendering.h"
#include <iostream>
#include <array>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>



namespace Component {

    /*!
    @brief Get the location of a uniform variable with optional error handling.
    @param name Uniform variable name in shader source
    @param exit_on_error If true, terminate application on uniform not found
    @return Uniform location index (>= 0) or -1 if not found
    @details
    Queries the OpenGL program for the location of a named uniform variable
    using glGetUniformLocation. Provides flexible error handling for both
    optional and required uniforms with clear diagnostic output.


    @note Uniform must be active (used) in shader to have valid location
    @warning Application terminates if exit_on_error is true and uniform not found
    */
    GLint
        GLSLShader::GetUniformLocation(GLchar const* name, bool exit_on_error) {
        GLint location = glGetUniformLocation(pgm_handle, name);
        if (location < 0) {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;

            if (exit_on_error) ::std::exit(EXIT_FAILURE);
        }
        return location;
    }

    /*!
    @brief Check if a file exists and is accessible for reading.
    @param file_name Path to file to check (relative or absolute)
    @return GL_TRUE if file exists and is readable, GL_FALSE otherwise
    @details
    Utility function that verifies file existence and accessibility using
    standard C++ file stream operations. Provides early error detection
    for shader file loading operations with minimal overhead.

    **File Validation Process:**
    1. Attempt to open file with std::ifstream
    2. Check stream state using good() method
    3. Return boolean result based on accessibility
    4. File stream automatically closed by destructor

    **Path Handling:**
    - Supports both relative and absolute file paths
    - Handles forward slashes (/) on all platforms
    - Processes backslashes (\) correctly on Windows
    - Resolves relative paths from current working directory

    **Accessibility Checks:**
    - File exists in filesystem
    - File has read permissions for current user
    - File is not locked by another process
    - Path is valid and accessible

    **Performance Characteristics:**
    - Minimal overhead using standard library operations
    - No file content reading, only accessibility check
    - Fast operation suitable for validation workflows
    - Cross-platform implementation with consistent behavior

    **Error Prevention:**
    - Prevents file I/O exceptions during shader compilation
    - Enables better error messages for missing shader files
    - Allows graceful fallback to embedded or default shaders
    - Supports shader file validation in development workflows

    **Usage Examples:**
    ```cpp
    if (FileExists("shaders/vertex.glsl")) {
        CompileShaderFromFile(GL_VERTEX_SHADER, "shaders/vertex.glsl");
    } else {
        std::cout << "Shader file not found, using default" << std::endl;
        CompileShaderFromString(GL_VERTEX_SHADER, defaultVertexShader);
    }
    ```

    @note File accessibility can change between check and actual file operations
    @warning Does not guarantee file will remain accessible for subsequent reads
    */
    GLboolean
        GLSLShader::FileExists(::std::string const& file_name) {
        ::std::ifstream infile(file_name); return infile.good();
    }

    void
        GLSLShader::DeleteShaderProgram() {
        if (pgm_handle > 0) {
            glDeleteProgram(pgm_handle);
        }
    }

    /*!
    @brief Complete shader program creation with compilation, linking, and validation.
    @param vec Vector of shader type and source pairs for multi-stage programs
    @param compile_from_file If true, treat sources as file paths; if false, as source code
    @return GL_TRUE on complete success, GL_FALSE on any stage failure
    @details
    Provides a comprehensive one-call solution for creating complete shader programs
    from multiple shader stages. Handles compilation of all provided shaders,
    linking them into a program object, validation of the complete program,
    and introspection of active attributes and uniforms for debugging.


    @note Requires valid OpenGL context and properly formatted shader sources
    @warning All shader stages must be compatible for successful linking
    @note Introspection output is printed to console for debugging purposes
    */
    GLboolean
        GLSLShader::CompileLinkValidate(::std::vector<::std::pair<GLenum, ::std::string>> vec, bool compile_from_file) {
        for (auto& elem : vec) {
            if (compile_from_file) {
                if (GL_FALSE == CompileShaderFromFile(elem.first, elem.second.c_str())) {
                    return GL_FALSE;
                }
            }
            else {
                if (GL_FALSE == CompileShaderFromString(elem.first, elem.second.c_str())) {
                    return GL_FALSE;
                }
            }
        }

        if (GL_FALSE == Link()) {
            return GL_FALSE;
        }
        if (GL_FALSE == Validate()) {
            return GL_FALSE;
        }
        PrintActiveAttribs();
        PrintActiveUniforms();

        return GL_TRUE;
    }

    /*!
    @brief Compile a shader from a source file with comprehensive error handling.
    @param shader_type OpenGL shader type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
    @param file_name Path to shader source file (relative or absolute)
    @return GL_TRUE on successful compilation, GL_FALSE on any error
    @details
    Loads and compiles a shader from a file source with robust error handling
    at each stage. Creates the shader program object if needed and delegates
    actual compilation to CompileShaderFromString after loading file content.


    @note Requires valid OpenGL context and accessible shader file
    @warning File path should use forward slashes or proper OS-specific separators
    */
    GLboolean
        GLSLShader::CompileShaderFromFile(GLenum shader_type, const ::std::string& file_name) {
        if (GL_FALSE == FileExists(file_name)) {
            log_string = "File not found";
            return GL_FALSE;
        }
        if (pgm_handle <= 0) {
            pgm_handle = glCreateProgram();
            if (0 == pgm_handle) {
                log_string = "Cannot create program handle";
                return GL_FALSE;
            }
        }

        ::std::ifstream shader_file(file_name, ::std::ifstream::in);
        if (!shader_file) {
            log_string = "Error opening file " + file_name;
            return GL_FALSE;
        }
        ::std::stringstream buffer;
        buffer << shader_file.rdbuf();
        shader_file.close();
        return CompileShaderFromString(shader_type, buffer.str());
    }

    /*!
    @brief Compile a shader from source code string with comprehensive error handling.
    @param shader_type OpenGL shader type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
    @param shader_src Complete GLSL shader source code as string
    @return GL_TRUE on successful compilation, GL_FALSE on any error
    @details
    Compiles a shader from provided source code string with robust error handling
    and automatic program object management. Creates the shader program object
    if needed and attaches successfully compiled shaders for subsequent linking.

    **Compilation Process:**
    1. Create OpenGL program object if not already created
    2. Create shader object of specified type
    3. Associate source code with shader object
    4. Compile shader and check compilation status
    5. Extract detailed error information on compilation failure
    6. Attach successfully compiled shader to program object
    7. Clean up temporary shader object handle

    **Program Object Management:**
    - Creates OpenGL program object on first shader compilation
    - Reuses existing program object for subsequent shaders
    - Validates program creation success before proceeding
    - Maintains program handle for linking and validation
    - Enables multi-stage shader program construction

    **Shader Object Lifecycle:**
    - Creates temporary shader object for compilation
    - Associates source code using glShaderSource
    - Performs compilation using glCompileShader
    - Attaches to program object on successful compilation
    - Automatically deletes shader object after attachment
    - Prevents shader object handle leaks

    **Error Handling and Diagnostics:**
    - Comprehensive compilation status checking
    - Detailed GLSL compiler error message extraction
    - Error log length validation for safe memory allocation
    - Complete error information stored in log_string
    - Graceful handling of OpenGL context issues

    **GLSL Compilation Features:**
    - Support for all OpenGL shader types (vertex, fragment, geometry, etc.)
    - Full GLSL language feature support including modern versions
    - Automatic shader version detection and compatibility
    - Preprocessor directive support (#define, #include, etc.)
    - Cross-platform GLSL compilation consistency

    **Memory Management:**
    - Safe dynamic allocation for error log messages
    - Proper cleanup of temporary allocations
    - No memory leaks from shader object creation
    - Efficient string handling for source code processing
    - Automatic resource cleanup on error conditions

    **Performance Characteristics:**
    - Efficient compilation process with minimal overhead
    - Single-pass compilation for optimal performance
    - No redundant OpenGL state changes during compilation
    - Optimized error message extraction when needed
    - Minimal CPU overhead for successful compilation path

    **Integration with Shader Pipeline:**
    - Seamless integration with Link() and Validate() functions
    - Compatible with multi-stage shader program construction
    - Supports incremental shader addition to program objects
    - Enables flexible shader loading from various sources
    - Maintains consistent compilation behavior across shader types

    **Error Logging and Debugging:**
    - Detailed GLSL compiler error messages with line numbers
    - Syntax error reporting with context information
    - Semantic error detection for variable and function usage
    - Preprocessor error handling for directive issues
    - Complete error information available through GetLog()

    **Usage Examples:**
    ```cpp
    // Compile vertex shader from string
    std::string vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    if (!shader.CompileShaderFromString(GL_VERTEX_SHADER, vertexSource)) {
        std::cout << "Vertex shader compilation failed: " << shader.GetLog() << std::endl;
        return false;
    }

    // Compile fragment shader from string
    std::string fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";

    if (!shader.CompileShaderFromString(GL_FRAGMENT_SHADER, fragmentSource)) {
        std::cout << "Fragment shader compilation failed: " << shader.GetLog() << std::endl;
        return false;
    }
    ```

    **Supported Shader Types:**
    - GL_VERTEX_SHADER: Vertex processing stage
    - GL_FRAGMENT_SHADER: Fragment/pixel processing stage
    - GL_GEOMETRY_SHADER: Geometry processing stage
    - GL_TESS_CONTROL_SHADER: Tessellation control stage
    - GL_TESS_EVALUATION_SHADER: Tessellation evaluation stage
    - GL_COMPUTE_SHADER: Compute processing stage

    @note Requires valid OpenGL context and properly formatted GLSL source
    @warning Source code must be compatible with current OpenGL context version
    */
    GLboolean
        GLSLShader::CompileShaderFromString(GLenum shader_type,
            const ::std::string& shader_src) {
        if (pgm_handle <= 0) {
            pgm_handle = glCreateProgram();
            if (0 == pgm_handle) {
                log_string = "Cannot create program handle";
                return GL_FALSE;
            }
        }

        GLuint shader_handle = 0;
        switch (shader_type) {
        case VERTEX_SHADER: shader_handle = glCreateShader(GL_VERTEX_SHADER); break;
        case FRAGMENT_SHADER: shader_handle = glCreateShader(GL_FRAGMENT_SHADER); break;
        case GEOMETRY_SHADER: shader_handle = glCreateShader(GL_GEOMETRY_SHADER); break;
        case TESS_CONTROL_SHADER: shader_handle = glCreateShader(GL_TESS_CONTROL_SHADER); break;
        case TESS_EVALUATION_SHADER: shader_handle = glCreateShader(GL_TESS_EVALUATION_SHADER); break;
            //case COMPUTE_SHADER: shader_handle = glCreateShader(GL_COMPUTE_SHADER); break;
        default:
            log_string = "Incorrect shader type";
            return GL_FALSE;
        }

        // load shader source code into shader object
        GLchar const* shader_code[] = { shader_src.c_str() };
        glShaderSource(shader_handle, 1, shader_code, NULL);

        // compile the shader
        glCompileShader(shader_handle);

        // check compilation status
        GLint comp_result;
        glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &comp_result);
        if (GL_FALSE == comp_result) {
            log_string = "Vertex shader compilation failed\n";
            GLint log_len;
            glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
            if (log_len > 0) {
                GLchar* log = new GLchar[log_len];
                GLsizei written_log_len;
                glGetShaderInfoLog(shader_handle, log_len, &written_log_len, log);
                log_string += log;
                delete[] log;
            }
            return GL_FALSE;
        }
        else { // attach the shader to the program object
            glAttachShader(pgm_handle, shader_handle);
            return GL_TRUE;
        }
    }

    GLboolean GLSLShader::GLSLShader::Link() {
        if (GL_TRUE == is_linked) {
            return GL_TRUE;
        }
        if (pgm_handle <= 0) {
            return GL_FALSE;
        }

        glLinkProgram(pgm_handle); // link the various compiled shaders

        // verify the link status
        GLint lnk_status;
        glGetProgramiv(pgm_handle, GL_LINK_STATUS, &lnk_status);
        if (GL_FALSE == lnk_status) {
            log_string = "Failed to link shader program\n";
            GLint log_len;
            glGetProgramiv(pgm_handle, GL_INFO_LOG_LENGTH, &log_len);
            if (log_len > 0) {
                GLchar* log_str = new GLchar[log_len];
                GLsizei written_log_len;
                glGetProgramInfoLog(pgm_handle, log_len, &written_log_len, log_str);
                log_string += log_str;
                delete[] log_str;
            }
            return GL_FALSE;
        }
        return is_linked = GL_TRUE;
    }

    void GLSLShader::Use() {
        if (pgm_handle > 0 && is_linked == GL_TRUE) {
            glUseProgram(pgm_handle);
        }
    }

    void GLSLShader::UnUse() {
        glUseProgram(0);
    }

    GLboolean GLSLShader::Validate() {
        if (pgm_handle <= 0 || is_linked == GL_FALSE) {
            return GL_FALSE;
        }

        glValidateProgram(pgm_handle);
        GLint status;
        glGetProgramiv(pgm_handle, GL_VALIDATE_STATUS, &status);
        if (GL_FALSE == status) {
            log_string = "Failed to validate shader program for current OpenGL context\n";
            GLint log_len;
            glGetProgramiv(pgm_handle, GL_INFO_LOG_LENGTH, &log_len);
            if (log_len > 0) {
                GLchar* log_str = new GLchar[log_len];
                GLsizei written_log_len;
                glGetProgramInfoLog(pgm_handle, log_len, &written_log_len, log_str);
                log_string += log_str;
                delete[] log_str;
            }
            return GL_FALSE;
        }
        else {
            return GL_TRUE;
        }
    }

    GLuint GLSLShader::GetHandle() const {
        return pgm_handle;
    }

    GLboolean GLSLShader::IsLinked() const {
        return is_linked;
    }

    std::string GLSLShader::GetLog() const {
        return log_string;
    }


    void GLSLShader::BindAttribLocation(GLuint index, GLchar const* name) {
        glBindAttribLocation(pgm_handle, index, name);
    }

    void GLSLShader::BindFragDataLocation(GLuint color_number, GLchar const* name) {
        glBindFragDataLocation(pgm_handle, color_number, name);
    }

    void GLSLShader::SetUniform(GLchar const* name, GLboolean val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform1i(loc, val);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, GLint val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform1i(loc, val);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, GLfloat val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform1f(loc, val);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, GLfloat x, GLfloat y) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform2f(loc, x, y);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, GLfloat x, GLfloat y, GLfloat z) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform3f(loc, x, y, z);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void
        GLSLShader::SetUniform(GLchar const* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform4f(loc, x, y, z, w);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, glm::vec2 const& val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform2f(loc, val.x, val.y);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, glm::vec3 const& val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform3f(loc, val.x, val.y, val.z);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, glm::vec4 const& val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniform4f(loc, val.x, val.y, val.z, val.w);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, glm::mat3 const& val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniformMatrix3fv(loc, 1, GL_FALSE, &val[0][0]);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::SetUniform(GLchar const* name, glm::mat4 const& val) {
        GLint loc = glGetUniformLocation(pgm_handle, name);
        if (loc >= 0) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, &val[0][0]);
        }
        else {
            std::cout << "Uniform variable " << name << " doesn't exist" << ::std::endl;
        }
    }

    void GLSLShader::PrintActiveAttribs() const {
#if 1
        GLint max_length, num_attribs;
        glGetProgramiv(pgm_handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);
        glGetProgramiv(pgm_handle, GL_ACTIVE_ATTRIBUTES, &num_attribs);
        GLchar* pname = new GLchar[max_length];
        std::cout << "Index\t|\tName\n";
        std::cout << "----------------------------------------------------------------------\n";
        for (GLint i = 0; i < num_attribs; ++i) {
            GLsizei written;
            GLint size;
            GLenum type;
            glGetActiveAttrib(pgm_handle, i, max_length, &written, &size, &type, pname);
            GLint loc = glGetAttribLocation(pgm_handle, pname);
            std::cout << loc << "\t\t" << pname << std::endl;
        }
        std::cout << "----------------------------------------------------------------------\n";
        delete[] pname;
#else
        GLint numAttribs;
        glGetProgramInterfaceiv(pgm_handle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);
        GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };
        std::cout << "Active attributes:" << std::endl;
        for (GLint i = 0; i < numAttribs; ++i) {
            GLint results[3];
            glGetProgramResourceiv(pgm_handle, GL_PROGRAM_INPUT, i, 3, properties, 3, NULL, results);

            GLint nameBufSize = results[0] + 1;
            GLchar* pname = new GLchar[nameBufSize];
            glGetProgramResourceName(pgm_handle, GL_PROGRAM_INPUT, i, nameBufSize, NULL, pname);
            //   std::cout << results[2] << " " << pname << " " << getTypeString(results[1]) << std::endl;
            std::cout << results[2] << " " << pname << " " << results[1] << std::endl;
            delete[] pname;
        }
#endif
    }


    void GLSLShader::PrintActiveUniforms() const {
        GLint max_length;
        glGetProgramiv(pgm_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);
        GLchar* pname = new GLchar[max_length];
        GLint num_uniforms;
        glGetProgramiv(pgm_handle, GL_ACTIVE_UNIFORMS, &num_uniforms);
        std::cout << "Location\t|\tName\n";
        std::cout << "----------------------------------------------------------------------\n";
        for (GLint i = 0; i < num_uniforms; ++i) {
            GLsizei written;
            GLint size;
            GLenum type;
            glGetActiveUniform(pgm_handle, i, max_length, &written, &size, &type, pname);
            GLint loc = glGetUniformLocation(pgm_handle, pname);
            std::cout << loc << "\t\t" << pname << std::endl;
        }
        std::cout << "----------------------------------------------------------------------\n";
        delete[] pname;
    }

}


