#ifndef RENDERING_H
#define RENDERING_H

/*!
@file        rendering.h
@author      ranvitha@digipen.edu
@date        09/12/2025

This file declares the Rendering class, which provides a comprehensive
OpenGL rendering infrastructure for the CrashoutEngine. It encapsulates
all resources and operations needed to efficiently render 2D primitives
with support for both textured and non-textured geometry.

The Rendering class serves as the core rendering foundation and provides:

**Core Functionality:**
- OpenGL resource management (VAO, VBO, EBO objects)
- GLSL shader program compilation, linking, and management
- Vertex attribute configuration and binding
- Primitive rendering with configurable draw modes

**Performance Optimizations:**
- Buffer pooling system to reduce allocation overhead
- Persistent buffer mapping for ultra-fast data uploads
- In-place buffer updates to minimize GPU state changes
- Optimized buffer setup functions for different rendering scenarios

**Rendering Modes:**
- Basic geometry rendering (position + color attributes)
- Textured geometry rendering (position + color + texture coordinate attributes)
- Support for indexed and non-indexed rendering
- Configurable primitive types (triangles, triangle fans, lines, etc.)

**Buffer Management:**
- Dynamic buffer allocation with intelligent reuse
- Separate VBO management for different attribute types
- Element buffer (EBO) support for indexed rendering
- Memory-mapped buffers for high-performance data streaming

**Singleton Pattern:**
The class implements a singleton pattern to ensure consistent rendering
state across the entire engine, accessible via the Get() static method.

**Integration:**
Designed to work seamlessly with the Shapes, TextureRenderer, and
sprite animation systems, providing the low-level rendering foundation
for all 2D graphics operations in the engine.
*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */


#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstddef>
#include "shader.h"	
#include "entitymanager.h"

namespace Component {

    /*!
    @class Rendering
    @brief Core OpenGL rendering infrastructure for the CrashoutEngine.
    @details
    The Rendering class provides a comprehensive rendering system that manages
    OpenGL resources, shader programs, and vertex data for efficient 2D graphics
    rendering. It implements a singleton pattern and includes advanced performance
    optimizations such as buffer pooling and persistent buffer mapping.
    
    Key features include:
    - Automatic OpenGL resource management
    - High-performance buffer operations with pooling
    - Support for both textured and non-textured rendering
    - Flexible vertex attribute configuration
    - Memory-mapped buffers for optimal data streaming
    */
    class Rendering {
    public:
        
        /*!
        @brief Default constructor initializing all OpenGL handles and state.
        @details
        Initializes all OpenGL resource handles to 0 (invalid state) and sets up
        default rendering parameters. The actual OpenGL resources are created
        when needed through the various setup functions.
        */
        Rendering() : vaoid(0), vbo_hdl(0), ebo_hdl(0), primitive_type(GL_TRIANGLES), 
                      idx_elem_cnt(0), tex_vbo_hdl(0), texobj(0), current_vbo_size(0), 
                      current_ebo_size(0), mapped_vbo_ptr(nullptr), mapped_ebo_ptr(nullptr), 
                      buffers_initialized(false) {}
        
        /*!
        @brief Get the singleton instance of the Rendering class.
        @return Reference to the static Rendering instance
        @details
        Implements the singleton pattern to ensure a single, consistent rendering
        context throughout the engine. The instance is created on first access
        and persists for the lifetime of the application.
        */
        static Rendering& Get();
        
        /*!
        @brief Initialize the rendering system and compile default shaders.
        @details
        Performs one-time initialization of the rendering system, including:
        - Compilation and linking of default vertex and fragment shaders
        - Validation of shader programs
        - Setup of default rendering state
        Should be called once during engine initialization.
        */
        static void Initialize();
        
        /*!
        @brief Clean up all rendering resources and buffer pools.
        @details
        Performs comprehensive cleanup of all OpenGL resources including:
        - Deletion of VAO, VBO, and EBO objects
        - Cleanup of persistent buffer mappings
        - Clearing of buffer pools
        - Shader program cleanup
        Should be called during engine shutdown to prevent resource leaks.
        */
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
        GLSLShader* shdr_pgm = nullptr;
        GLSLShader shdr_pgm_instanced;
        
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        size_t current_vbo_size{ 0 };
        size_t current_ebo_size{ 0 };
        
        
        void* mapped_vbo_ptr{ nullptr };
        void* mapped_ebo_ptr{ nullptr };
        
        // Buffer reuse or not bool
        bool buffers_initialized{ false };

    private:
        // Performance optimization members
        struct BufferPool {
            std::vector<GLuint> available_vbos;  ///< Pool of reusable VBO handles for performance optimization
            std::vector<GLuint> available_vaos;  ///< Pool of reusable VAO handles for performance optimization
            std::vector<GLuint> available_ebos;  ///< Pool of reusable EBO handles for performance optimization
            size_t max_pool_size = 100;          ///< Maximum number of buffers to keep in each pool
        };
        static BufferPool buffer_pool;           ///< Static buffer pool instance shared across all rendering operations

    public:

        /*!
        @brief Updates the view and projection matrices based on target dimensions.
        @param targetWidth Width of the render target (window or FBO texture).
        @param targetHeight Height of the render target.
        */
        void UpdateMatrices(int targetWidth, int targetHeight);

        /*!
        @brief Set up a default Vertex Array Object with quad geometry.
        @details
        Creates a default VAO with a unit quad geometry suitable for basic rendering.
        The quad is defined with:
        - 4 vertices in a square formation (-0.5 to 0.5 range)
        - Default white, red, green, blue colors for each vertex
        - Triangle fan primitive type for efficient rendering
        - Element buffer with indices for proper vertex ordering
        
        This function is primarily used for basic shape rendering and serves as
        a foundation for more complex geometry setup.
        */
        void Setup_VAO();
        
        /*!
        @brief Compile and link the default shader program.
        @details
        Loads, compiles, and links the default vertex and fragment shaders from:
        - CrashoutShader.vert (vertex shader)
        - CrashoutShader.frag (fragment shader)
        
        Performs comprehensive error checking including:
        - Shader compilation validation
        - Program linking verification
        - Runtime validation in current OpenGL context
        
        Exits the application with error messages if any step fails.
        */
        void Setup_Shdrpgm();
        void Setup_ShdrpgmInstanced();
        
        /*!
        @brief Execute a draw call using the current VAO and shader state.
        @details
        Performs a complete draw operation by:
        1. Activating the shader program
        2. Binding the current VAO
        3. Issuing a glDrawElements call with stored primitive type and index count
        4. Cleaning up by unbinding VAO and deactivating shader
        
        Uses the primitive_type and idx_elem_cnt members to determine
        the rendering parameters. Assumes all necessary OpenGL state
        has been properly configured through the setup functions.
        */
        void Draw();

        // Dynamic rendering 
        void SetupBuffers(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* indices, size_t index_count,
            GLenum primitive);

        
        /*!
        @brief Set up buffers for textured geometry using traditional OpenGL calls.
        @param vertices Vertex data array pointer
        @param vertex_size Size of vertex data in bytes
        @param colors Color data array pointer
        @param color_size Size of color data in bytes
        @param tex_coords Texture coordinate data array pointer
        @param tex_coord_size Size of texture coordinate data in bytes
        @param indices Index array pointer
        @param index_count Number of indices
        @param primitive OpenGL primitive type (default: GL_TRIANGLES)
        @details
        Traditional buffer setup function for textured geometry that doesn't
        use persistent mapping optimizations. Suitable for static geometry
        or when persistent mapping is not available.
        
        Performs standard OpenGL buffer operations:
        - glGenBuffers for VBO and EBO creation
        - glBufferData for data upload
        - glVertexAttribPointer for attribute configuration
        
        Uses traditional buffer management approach for textured rendering.
        */
        void SetupTexturedBuffers(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* tex_coords, size_t tex_coord_size,
            const void* indices, size_t index_count,
            GLenum primitive);

        // Optimized buffer operations
        /*!
        @brief Set up optimized buffers for non-textured geometry rendering.
        @param vertices Vertex data array pointer
        @param vertex_size Size of vertex data in bytes
        @param colors Color data array pointer
        @param color_size Size of color data in bytes
        @param indices Index array pointer
        @param index_count Number of indices
        @param primitive OpenGL primitive type (default: GL_TRIANGLES)
        @details
        Creates or updates VBO and EBO with optimized buffer management:
        - Uses persistent mapped buffers when available for zero-copy uploads
        - Automatically resizes buffers when data exceeds current capacity
        - Configures vertex attributes for position and color
        - Sets up VAO with proper attribute bindings and stride calculations
        
        This function provides the highest performance path for frequently
        updated geometry by leveraging persistent buffer mapping.
        */
        void SetupBuffersOptimized(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* indices, size_t index_count,
            GLenum primitive);
            
        /*!
        @brief Set up optimized buffers for textured geometry rendering.
        @param vertices Vertex data array pointer
        @param vertex_size Size of vertex data in bytes
        @param colors Color data array pointer
        @param color_size Size of color data in bytes
        @param tex_coords Texture coordinate data array pointer
        @param tex_coord_size Size of texture coordinate data in bytes
        @param indices Index array pointer
        @param index_count Number of indices
        @param primitive OpenGL primitive type (default: GL_TRIANGLES)
        @details
        Similar to SetupBuffersOptimized but configured for textured rendering.
        Configures vertex attributes for position, color, and texture coordinates.
        
        Uses the same persistent buffer optimizations as SetupBuffersOptimized
        but with additional texture coordinate attribute configuration.
        */
        void SetupTexturedBuffersOptimized(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* tex_coords, size_t tex_coord_size,
            const void* indices, size_t index_count,
            GLenum primitive);
            
        /*!
        @brief Update buffer data in-place without recreating OpenGL objects.
        @param vertices New vertex data to upload
        @param vertex_size Size of vertex data in bytes
        @param colors New color data to upload
        @param color_size Size of color data in bytes
        @param indices New index data to upload
        @param index_count Number of indices
        @details
        Performs ultra-fast buffer updates using persistent mapped memory.
        This function assumes buffers have already been initialized with
        SetupBuffersOptimized and simply copies new data to the mapped
        memory regions.
        
        Performance characteristics:
        - Zero OpenGL state changes
        - Direct memory copy operations
        - No buffer binding or attribute setup overhead
        - Automatic buffer resizing if data exceeds current capacity
        
        Ideal for animated geometry, particle systems, or any frequently
        changing vertex data where maximum performance is critical.
        */
        void UpdateBuffersInPlace(const void* vertices, size_t vertex_size,
            const void* colors, size_t color_size,
            const void* indices, size_t index_count);
            
        // Buffer pool management
        /*!
        @brief Retrieve a VBO from the buffer pool or create a new one.
        @return OpenGL handle to a Vertex Buffer Object
        @details
        Implements an efficient buffer pooling system to reduce allocation overhead.
        If a VBO is available in the pool, it's returned immediately. Otherwise,
        a new VBO is generated using glGenBuffers. This significantly improves
        performance by avoiding frequent buffer creation/destruction cycles.
        
        The returned VBO is ready for use but not bound to any target.
        */
        GLuint GetPooledVBO();
        
        /*!
        @brief Retrieve a VAO from the buffer pool or create a new one.
        @return OpenGL handle to a Vertex Array Object
        @details
        Similar to GetPooledVBO but for Vertex Array Objects. VAOs store vertex
        attribute configuration and are expensive to create, making pooling
        particularly beneficial. The returned VAO is in an unbound state and
        ready for configuration.
        */
        GLuint AcquireVAO();
        GLuint AcquireVBO();
        GLuint AcquireEBO();
        GLuint GetPooledVAO();
        //GLuint GetPooledEBO();
        void ReturnVAO(GLuint vao);
        void ReturnVBO(GLuint vbo);
        void ReturnEBO(GLuint ebo);
        
        /*!
        @brief Retrieve an EBO from the buffer pool or create a new one.
        @return OpenGL handle to an Element Buffer Object
        @details
        Provides pooled access to Element Buffer Objects (index buffers).
        EBOs are used for indexed rendering to reduce vertex duplication.
        The returned EBO is ready for use but not bound to any target.
        */
        GLuint GetPooledEBO();
        
        /*!
        @brief Return buffers to the appropriate pools for reuse.
        @param vbo OpenGL handle of the VBO to return
        @param vao OpenGL handle of the VAO to return
        @param ebo OpenGL handle of the EBO to return
        @details
        Returns buffers to their respective pools for future reuse.
        This prevents resource leaks and maintains the efficiency of the pooling system.
        
        The buffers should not be used after being returned to the pool.
        */
        void ReturnToPool(GLuint vbo, GLuint vao, GLuint ebo);
        
        
        /*!        @brief Initialize persistent mapped buffers for high-performance data streaming.
        @param max_vertex_size Size in bytes for the persistent VBO
        @param max_index_size Size in bytes for the persistent EBO
        @details
        Creates persistent mapped buffers using OpenGL's buffer storage functionality
        for maximum performance data uploads. These buffers remain mapped for the
        entire lifetime of the rendering context, eliminating map/unmap overhead.
        
        Features:
        - GL_MAP_WRITE_BIT: Allows CPU writes to the buffer
        - GL_MAP_PERSISTENT_BIT: Buffer remains mapped across OpenGL calls
        - GL_MAP_COHERENT_BIT: Automatic synchronization between CPU and GPU
        - GL_DYNAMIC_STORAGE_BIT: Allows buffer content modification
        
        The mapped pointers are stored in mapped_vbo_ptr and mapped_ebo_ptr
        for direct memory access. This approach provides the fastest possible
        data upload path for frequently updated geometry.
        
        @note Requires OpenGL 4.4+ or ARB_buffer_storage extension
        */
        void InitializePersistentBuffers(size_t max_vertex_size, size_t max_index_size);
        
        /*!
        @brief Clean up persistent mapped buffers and release resources.
        @details
        Properly unmaps and deletes persistent buffers created by
        InitializePersistentBuffers. This includes:
        - Unmapping buffer memory (if not using persistent mapping)
        - Deleting OpenGL buffer objects
        - Resetting mapped pointers to nullptr
        - Clearing size tracking variables
        
        Should be called during shutdown or when switching rendering contexts
        to prevent resource leaks.
        */
        void CleanupPersistentBuffers();


    };

}


#endif
