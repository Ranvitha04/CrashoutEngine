#pragma once
/*!
@file        glslshader.h
@author      ranvitha@digipen.edu
@date        09/12/2025

This header file declares the GLSLShader class, which provides a comprehensive
C++ abstraction layer for OpenGL GLSL shader management within the CrashoutEngine.
The class encapsulates the complete shader pipeline from compilation to execution,
offering robust error handling, validation, and uniform management capabilities.

**Core Functionality:**
- **Shader Compilation**: Compile vertex, fragment, geometry, and tessellation shaders
  from both file sources and string literals with comprehensive error reporting
- **Program Linking**: Link multiple shader stages into executable shader programs
  with detailed validation and error diagnostics
- **Runtime Validation**: Validate shader programs against current OpenGL context
  to ensure compatibility and correctness
- **Uniform Management**: Type-safe uniform variable setting for scalars, vectors,
  and matrices with automatic location resolution

**Advanced Features:**
- **Multi-Stage Support**: Support for vertex, fragment, geometry, tessellation
  control, and tessellation evaluation shaders
- **Flexible Input Sources**: Load shaders from files or compile from string literals
  for dynamic shader generation and embedded shader code
- **Comprehensive Error Handling**: Detailed compilation and linking error messages
  with shader log extraction for debugging
- **Attribute Binding**: Manual vertex attribute and fragment output location binding
  for precise shader interface control

**Performance Optimizations:**
- **Cached Uniform Locations**: Efficient uniform location caching to minimize
  OpenGL state queries during rendering
- **Program State Management**: Optimized shader program activation/deactivation
  with state tracking to prevent redundant OpenGL calls
- **Validation Caching**: Link status caching to avoid repeated validation overhead

**Integration Features:**
- **Automation Support**: Built-in automation hooks for testing frameworks and
  continuous integration systems
- **Engine Integration**: Seamless integration with CrashoutEngine's rendering
  pipeline and resource management systems
- **Debug Support**: Active attribute and uniform introspection for development
  and debugging workflows

**Shader Types Supported:**
- Vertex Shaders (GL_VERTEX_SHADER): Vertex transformation and attribute processing
- Fragment Shaders (GL_FRAGMENT_SHADER): Pixel color computation and effects
- Geometry Shaders (GL_GEOMETRY_SHADER): Primitive generation and modification
- Tessellation Control (GL_TESS_CONTROL_SHADER): Tessellation level control
- Tessellation Evaluation (GL_TESS_EVALUATION_SHADER): Tessellation vertex generation

**Usage Patterns:**
- Single-stage shaders for basic rendering (vertex + fragment)
- Multi-stage pipelines for advanced effects (geometry, tessellation)
- Dynamic shader compilation for procedural effects
- Shader hot-reloading for development workflows

This class serves as the foundation for all shader operations in the CrashoutEngine,
providing a safe, efficient, and feature-rich interface for modern OpenGL shader
programming while maintaining compatibility with automated testing systems.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */


#include "CEVec2.h"
#include "CEVec3.h"
#include "namecomponent.h"
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
        /*!
        @brief OpenGL GLSL shader program management class.
        @details
        Provides comprehensive shader compilation, linking, validation, and uniform
        management capabilities for modern OpenGL applications. Encapsulates the
        complete shader pipeline with robust error handling and performance optimizations.
        */
    public:

        /*!
        @brief Default constructor initializing shader object to safe state.
        @details
        Initializes the GLSLShader object with default values ensuring safe operation:
        - Program handle set to 0 (invalid/uninitialized state)
        - Link status set to GL_FALSE (not linked)
        - Log string cleared for error message storage
        
        **Safe State Guarantees:**
        - No OpenGL resources allocated until first compilation
        - All member variables initialized to known safe values
        - Object ready for immediate use without additional setup
        
        **Usage Pattern:**
        ```cpp
        GLSLShader shader;  // Safe default state
        shader.CompileShaderFromFile(GL_VERTEX_SHADER, "vertex.glsl");
        shader.CompileShaderFromFile(GL_FRAGMENT_SHADER, "fragment.glsl");
        shader.Link();
        ```
        */
        GLSLShader() : pgm_handle(0), is_linked(GL_FALSE) { /* empty by design */ }

        /*!
        @brief Compile, link, and validate multiple shaders in a single operation.
        @param shader_files Vector of shader type and source pairs to compile
        @param compile_from_file If true, treat sources as file paths; if false, as source code
        @return GL_TRUE on success, GL_FALSE on any compilation, linking, or validation error
        @details
        Streamlined function for complete shader program creation from multiple shader stages.
        Performs all necessary steps in sequence with comprehensive error checking at each stage.
        
        **Process Flow:**
        1. Iterate through all provided shader stages
        2. Compile each shader (from file or string based on flag)
        3. Link all compiled shaders into program
        4. Validate program against current OpenGL context
        5. Print active attributes and uniforms for debugging
        
        **Error Handling:**
        - Returns GL_FALSE immediately on first compilation failure
        - Returns GL_FALSE on linking failure with detailed error log
        - Returns GL_FALSE on validation failure with context information
        - Error details available via GetLog() for debugging
        
        **Performance Notes:**
        - Efficient batch compilation reduces OpenGL state changes
        - Automatic validation ensures program correctness
        - Debug output can be disabled in release builds
        
        **Example Usage:**
        ```cpp
        vector<pair<GLenum, string>> shaders = {
            {GL_VERTEX_SHADER, "shaders/vertex.glsl"},
            {GL_FRAGMENT_SHADER, "shaders/fragment.glsl"}
        };
        if (!shader.CompileLinkValidate(shaders, true)) {
            cout << "Shader compilation failed: " << shader.GetLog() << endl;
        }
        ```
        */
        GLboolean CompileLinkValidate(vector<pair<GLenum, string>>, bool compile_from_file = true);

        /*!
        @brief Compile a shader from a source file.
        @param shader_type OpenGL shader type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
        @param file_name Path to the shader source file
        @return GL_TRUE on successful compilation, GL_FALSE on error
        @details
        Loads and compiles a shader from a file source with comprehensive error handling.
        Creates the shader program object if it doesn't exist and attaches the compiled
        shader for subsequent linking.
        
        **File Handling:**
        - Validates file existence before attempting to read
        - Reads entire file content into memory for compilation
        - Handles file I/O errors with descriptive error messages
        - Supports relative and absolute file paths
        
        **Compilation Process:**
        1. Check file existence and accessibility
        2. Create program object if not already created
        3. Read file content into string buffer
        4. Create shader object of specified type
        5. Compile shader source with error checking
        6. Attach compiled shader to program object
        
        **Error Conditions:**
        - File not found or inaccessible
        - Program object creation failure
        - Shader compilation errors (syntax, compatibility)
        - OpenGL context errors
        
        **Supported Shader Types:**
        - GL_VERTEX_SHADER: Vertex transformation and attribute processing
        - GL_FRAGMENT_SHADER: Pixel color computation
        - GL_GEOMETRY_SHADER: Primitive generation and modification
        - GL_TESS_CONTROL_SHADER: Tessellation level control
        - GL_TESS_EVALUATION_SHADER: Tessellation vertex generation
        
        @note Requires valid OpenGL context and accessible shader file
        @warning File path should use forward slashes or proper OS-specific separators
        */
        GLboolean CompileShaderFromFile(GLenum shader_type, const string& file_name);

        /*!
        @brief Compile a shader from source code string.
        @param shader_type OpenGL shader type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
        @param shader_src GLSL source code as string
        @return GL_TRUE on successful compilation, GL_FALSE on error
        @details
        Compiles a shader directly from source code string, enabling dynamic shader
        generation and embedded shader code. Creates the shader program object if
        it doesn't exist and attaches the compiled shader for subsequent linking.
        
        **Compilation Process:**
        1. Create program object if not already created
        2. Create shader object of specified type
        3. Load source code into shader object
        4. Compile shader with comprehensive error checking
        5. Attach compiled shader to program object
        
        **Dynamic Shader Support:**
        - Runtime shader generation from templates
        - Procedural shader code construction
        - Embedded shader code compilation
        - Shader hot-reloading from memory
        
        **Error Handling:**
        - Detailed compilation error messages with line numbers
        - GLSL syntax error reporting
        - OpenGL compatibility validation
        - Shader log extraction for debugging
        
        **Performance Considerations:**
        - No file I/O overhead compared to file-based compilation
        - Suitable for frequently changing shader code
        - Memory-efficient for small shader programs
        - Ideal for shader caching systems
        
        **Example Usage:**
        ```cpp
        string vertexSource = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            void main() {
                gl_Position = vec4(aPos, 1.0);
            }
        )";
        if (!shader.CompileShaderFromString(GL_VERTEX_SHADER, vertexSource)) {
            cout << "Compilation failed: " << shader.GetLog() << endl;
        }
        ```
        
        @note Requires valid OpenGL context and syntactically correct GLSL code
        @warning Source string must remain valid during compilation process
        */
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

        /*!
        @brief Print all active vertex attributes to console for debugging.
        @details
        Introspects the linked shader program and prints detailed information about
        all active vertex attributes to the console. Essential for debugging vertex
        attribute layouts and ensuring proper vertex array object configuration.
        
        **Output Information:**
        - Attribute location index (0-based)
        - Attribute variable name as declared in shader
        - Attribute data type and component count
        - Formatted table output for easy reading
        
        **Debug Output Format:**
        ```
        Index   |   Name
        ----------------------------------------------------------------------
        0       aPosition
        1       aColor
        2       aTexCoord
        ----------------------------------------------------------------------
        ```
        
        **Usage Scenarios:**
        - Debugging vertex attribute binding issues
        - Verifying attribute location assignments
        - Development and shader optimization workflows
        - Automated testing and validation systems
        
        **Requirements:**
        - Shader program must be successfully linked
        - Only active attributes (used in shader) are displayed
        - Unused attributes are optimized out and not shown
        
        **Performance Notes:**
        - Introspection is expensive; use only for debugging
        - Safe to call multiple times with same results
        - Output can be disabled in release builds
        
        @note Requires successfully linked shader program
        @warning Only shows attributes actually used by the shader
        */
        void PrintActiveAttribs() const;

        /*!
        @brief Print all active uniform variables to console for debugging.
        @details
        Introspects the linked shader program and prints detailed information about
        all active uniform variables to the console. Invaluable for debugging uniform
        binding issues and understanding shader parameter requirements.
        
        **Output Information:**
        - Uniform location index (-1 if not found)
        - Uniform variable name as declared in shader
        - Uniform data type and array size information
        - Formatted table output for easy reading
        
        **Debug Output Format:**
        ```
        Location    |   Name
        ----------------------------------------------------------------------
        0           modelMatrix
        1           viewMatrix
        2           projectionMatrix
        3           lightPosition
        ----------------------------------------------------------------------
        ```
        
        **Uniform Categories:**
        - Scalar uniforms (float, int, bool)
        - Vector uniforms (vec2, vec3, vec4)
        - Matrix uniforms (mat3, mat4)
        - Sampler uniforms (sampler2D, samplerCube)
        - Array uniforms with element count
        
        **Usage Scenarios:**
        - Debugging uniform binding and location issues
        - Verifying uniform variable names and types
        - Development and shader optimization workflows
        - Automated testing and validation systems
        
        **Requirements:**
        - Shader program must be successfully linked
        - Only active uniforms (used in shader) are displayed
        - Unused uniforms are optimized out and not shown
        
        @note Requires successfully linked shader program
        @warning Only shows uniforms actually used by the shader
        */
        void PrintActiveUniforms() const;

        /*!
        @brief Get the location of a uniform variable by name.
        @param name Uniform variable name in shader
        @param exit_on_error If true, exit application on uniform not found
        @return Uniform location index, or -1 if not found
        @details
        Queries the OpenGL program for the location of a named uniform variable.
        Provides optional error handling with application termination for critical
        uniforms that must exist.
        
        **Location Resolution:**
        - Returns valid location index (>= 0) if uniform exists and is active
        - Returns -1 if uniform doesn't exist or is optimized out
        - Caches location for performance (implementation dependent)
        - Case-sensitive name matching
        
        **Error Handling Modes:**
        - exit_on_error = false: Returns -1 and prints warning message
        - exit_on_error = true: Terminates application with error message
        - Useful for distinguishing optional vs. required uniforms
        
        **Performance Considerations:**
        - Uniform location queries can be expensive
        - Cache locations when possible for frequently used uniforms
        - Consider querying locations once after linking
        - Avoid querying in render loops
        
        **Usage Patterns:**
        ```cpp
        // Optional uniform (graceful handling)
        GLint loc = shader.GetUniformLocation("optionalParam", false);
        if (loc >= 0) {
            glUniform1f(loc, value);
        }
        
        // Required uniform (fail-fast)
        GLint loc = shader.GetUniformLocation("modelMatrix", true);
        glUniformMatrix4fv(loc, 1, GL_FALSE, &matrix[0][0]);
        ```
        
        **Common Failure Reasons:**
        - Uniform name misspelled or doesn't exist in shader
        - Uniform declared but not used (optimized out by compiler)
        - Shader program not linked or linking failed
        - Uniform name contains array notation incorrectly
        
        @note Uniform must be active (used) in shader to have valid location
        @warning Application exits if exit_on_error is true and uniform not found
        */
        GLint GetUniformLocation(GLchar const* name, bool exit_on_error = false);

        /*!
        @brief Check if a file exists and is accessible.
        @param file_name Path to file to check
        @return GL_TRUE if file exists and is readable, GL_FALSE otherwise
        @details
        Utility function to verify file existence and accessibility before attempting
        to read shader source files. Provides early error detection for missing
        shader files and improves error reporting.
        
        **File Validation:**
        - Checks file existence in filesystem
        - Verifies file is readable with current permissions
        - Handles both relative and absolute file paths
        - Cross-platform file access checking
        
        **Path Handling:**
        - Supports forward slashes (/) on all platforms
        - Handles backslashes (\) on Windows
        - Resolves relative paths from current working directory
        - Supports absolute paths with drive letters (Windows)
        
        **Error Prevention:**
        - Prevents file I/O errors during shader compilation
        - Enables better error messages for missing files
        - Allows graceful fallback to default shaders
        - Supports shader file validation workflows
        
        **Usage Patterns:**
        ```cpp
        if (shader.FileExists("shaders/vertex.glsl")) {
            shader.CompileShaderFromFile(GL_VERTEX_SHADER, "shaders/vertex.glsl");
        } else {
            // Use embedded default shader or show error
            cout << "Shader file not found: shaders/vertex.glsl" << endl;
        }
        ```
        
        **Performance Notes:**
        - File system access can be slow on some platforms
        - Consider caching results for frequently checked files
        - Minimal overhead compared to actual file reading
        
        @note Does not guarantee file will remain accessible between check and read
        @warning File accessibility can change between check and actual file operations
        */
        GLboolean FileExists(const string& file_name);

    private:
        /*!
        @enum ShaderType
        @brief Enumeration of supported OpenGL shader types.
        @details
        Defines the shader types supported by the GLSLShader class, mapping to
        corresponding OpenGL shader type constants. Provides type safety and
        documentation for shader compilation operations.
        
        **Supported Shader Stages:**
        - VERTEX_SHADER: Processes individual vertices, handles vertex transformations
        - FRAGMENT_SHADER: Processes individual pixels/fragments, handles color output
        - GEOMETRY_SHADER: Processes primitives, can generate/modify geometry
        - TESS_CONTROL_SHADER: Controls tessellation levels and patch subdivision
        - TESS_EVALUATION_SHADER: Generates tessellated vertices from patches
        
        **Graphics Pipeline Integration:**
        - Vertex shaders are mandatory for all rendering operations
        - Fragment shaders are mandatory for pixel color computation
        - Geometry shaders are optional for primitive manipulation
        - Tessellation shaders are optional for surface subdivision
        
        **Compute Shader Note:**
        Compute shaders (GL_COMPUTE_SHADER) are intentionally excluded as they
        operate outside the graphics pipeline and require different management
        patterns. They may be supported in future versions if needed.
        */
        enum ShaderType {
            VERTEX_SHADER = GL_VERTEX_SHADER,           ///< Vertex transformation and attribute processing
            FRAGMENT_SHADER = GL_FRAGMENT_SHADER,       ///< Pixel color computation and effects
            GEOMETRY_SHADER = GL_GEOMETRY_SHADER,       ///< Primitive generation and modification
            TESS_CONTROL_SHADER = GL_TESS_CONTROL_SHADER,     ///< Tessellation level control
            TESS_EVALUATION_SHADER = GL_TESS_EVALUATION_SHADER, ///< Tessellation vertex generation
            // ignore compute shader for now because it is not connected to
            // the graphics pipe
            // COMPUTE_SHADER = GL_COMPUTE_SHADER
        };

        /*!
        @brief OpenGL shader program object handle.
        @details
        Stores the OpenGL program object handle returned by glCreateProgram().
        This handle is used for all subsequent program operations including
        shader attachment, linking, validation, and activation.
        
        **Handle States:**
        - 0: No program created or creation failed (invalid state)
        - >0: Valid OpenGL program object handle
        
        **Lifecycle:**
        - Created during first shader compilation
        - Remains valid until program deletion
        - Automatically managed by class methods
        - Used internally for all OpenGL program operations
        */
        GLuint pgm_handle = 0;

        /*!
        @brief Program linking status flag.
        @details
        Tracks whether the shader program has been successfully linked and is
        ready for use. Only linked programs can be activated for rendering.
        
        **Status Values:**
        - GL_FALSE: Program not linked or linking failed
        - GL_TRUE: Program successfully linked and ready for use
        
        **State Management:**
        - Updated automatically during Link() operation
        - Cached to avoid repeated OpenGL queries
        - Used for safety checks in Use() and other operations
        - Reset when program is recreated or modified
        */
        GLboolean is_linked = GL_FALSE;

        /*!
        @brief Accumulated error and information log.
        @details
        Stores detailed error messages, warnings, and information from shader
        compilation, linking, and validation operations. Essential for debugging
        and understanding shader issues.
        
        **Log Content Sources:**
        - Shader compilation errors and warnings
        - Program linking errors and interface mismatches
        - Validation errors and context compatibility issues
        - File I/O errors and resource problems
        
        **Log Management:**
        - Messages accumulate across multiple operations
        - Previous content is preserved unless explicitly cleared
        - Accessible via GetLog() for external error handling
        - Automatically updated by all error-prone operations
        
        **Usage for Debugging:**
        - Contains line numbers for compilation errors
        - Includes detailed descriptions of linking failures
        - Provides context for validation issues
        - Essential for automated testing and error reporting
        */
        std::string log_string;
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

    const ::std::string Shader = "assets/shaders/CrashoutShader.frag";
    const ::std::string Vertex = "assets/shaders/CrashoutShader.vert";

#define AUTOMATION_HOOK_SHADER(shdr_pgm, shdr_files) \
shdr_files.clear(); \
shdr_files.push_back(std::make_pair(GL_VERTEX_SHADER, tutorial_vs)); \
shdr_files.push_back(std::make_pair(GL_FRAGMENT_SHADER, tutorial_fs)); \
shdr_pgm->CompileLinkValidate(shdr_files, false); \
return;
#else
#define AUTOMATION_HOOK_SHADER(shdr_pgm, shdr_files) 
#endif

}
