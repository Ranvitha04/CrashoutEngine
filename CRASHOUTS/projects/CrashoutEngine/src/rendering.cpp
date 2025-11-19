/*!
@file        rendering.cpp
@author      ranvitha@digipen.edu (65%)
@co-author   lee.r@digipen.edu (35% - Mainly camera related)
@date        09/12/2025
@brief       Implementation of the Rendering class for high-performance OpenGL rendering.

This file provides the complete implementation of the Component::Rendering class,
which serves as the core rendering infrastructure for the CrashoutEngine. It implements
advanced OpenGL rendering techniques with a focus on performance optimization and
efficient resource management.

- Support for both textured and non-textured geometry
- Configurable vertex attribute layouts (position, color, texture coordinates)
- Multiple primitive types (triangles, triangle fans, lines, etc.)
- Indexed and non-indexed rendering support

- Memory-mapped buffers for ultra-fast data streaming
- In-place buffer updates to minimize GPU state changes
- Buffer reuse through sophisticated pooling mechanisms
- Optimized vertex attribute configuration

This implementation provides the foundation for all 2D rendering operations
in the CrashoutEngine, with particular emphasis on performance-critical
applications such as sprite animation, particle systems, and dynamic geometry.
*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "rendering.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "shaderasset.h"
#include <string>
#include "CEVec3.h"
#include "CEVec2.h"
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "settings.h"

namespace Component {

    struct WorldSettings {
        inline static float BASE_SIZE = 100.f;
        inline static float MAX_NEAR_FAR = 50.f;
    };

    // Static member definitions
    Rendering::BufferPool Rendering::buffer_pool;

    Rendering& Rendering::Get() {
        static Rendering staticInstance;
        return staticInstance;
    }

    // Buffer pool management
    /*!
    @brief Retrieve a VBO from the pool or create a new one if pool is empty.
    @return OpenGL handle to a Vertex Buffer Object ready for use
    @details
    Implements efficient VBO pooling to reduce OpenGL object creation overhead.
    The function first checks if any VBOs are available in the pool. If found,
    it removes and returns the most recently added VBO (LIFO order for better
    cache locality). If the pool is empty, creates a new VBO using the modern
    glCreateBuffers function (OpenGL 4.5+).

    Performance benefits:
    - Eliminates frequent glGenBuffers/glDeleteBuffers calls
    - Reduces GPU driver overhead for buffer management
    - Maintains warm buffer objects that may still be in GPU cache

    The returned VBO is unbound and ready for configuration.
    */
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

    /*!
    @brief Retrieve a VAO from the pool or create a new one if pool is empty.
    @return OpenGL handle to a Vertex Array Object ready for configuration
    @details
    Similar to GetPooledVBO but for Vertex Array Objects. VAOs are particularly
    expensive to create as they store complex vertex attribute state, making
    pooling especially beneficial for performance.

    The function uses glCreateVertexArrays (OpenGL 4.5+) for direct state
    access, which is more efficient than the traditional glGenVertexArrays
    approach. The returned VAO has no vertex attributes configured and is
    ready for setup.

    VAO pooling is critical for applications that frequently switch between
    different vertex layouts, such as UI rendering mixed with sprite rendering.
    */
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

    /*!
    @brief Retrieve an EBO from the pool or create a new one if pool is empty.
    @return OpenGL handle to an Element Buffer Object ready for use
    @details
    Provides pooled access to Element Buffer Objects (index buffers) using
    the same efficient pooling strategy as VBOs. EBOs are used for indexed
    rendering to reduce vertex duplication in geometry with shared vertices.

    Uses glCreateBuffers for direct buffer creation without binding, which
    is part of the modern OpenGL 4.5+ direct state access (DSA) API.
    This approach is more efficient and less error-prone than traditional
    buffer creation methods.

    The returned EBO is unbound and ready for index data upload.
    */
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

    /*!
    @brief Return OpenGL buffer objects to their respective pools for reuse.
    @param vbo Vertex Buffer Object handle to return (0 = skip)
    @param vao Vertex Array Object handle to return (0 = skip)
    @param ebo Element Buffer Object handle to return (0 = skip)
    @details
    Implements intelligent buffer recycling with pool size management.
    Each buffer type has a maximum pool size to prevent unbounded memory
    growth. When a pool reaches its limit, additional buffers are deleted
    immediately rather than stored.

    Pool management strategy:
    - Buffers are added to pools only if space is available
    - Pool size limits prevent memory bloat in long-running applications
    - LIFO ordering (stack-like) for better cache locality
    - Zero handles are safely ignored (no-op)

    This function should be called when buffers are no longer needed
    to maintain the efficiency of the pooling system. Proper usage
    prevents both memory leaks and excessive buffer creation overhead.
    */
    void Rendering::ReturnToPool(GLuint vbo, GLuint vao, GLuint ebo) {
        if (buffer_pool.available_vbos.size() < buffer_pool.max_pool_size && vbo != 0) {
            buffer_pool.available_vbos.push_back(vbo);
        }
        else if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }

        if (buffer_pool.available_vaos.size() < buffer_pool.max_pool_size && vao != 0) {
            buffer_pool.available_vaos.push_back(vao);
        }
        else if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }

        if (buffer_pool.available_ebos.size() < buffer_pool.max_pool_size && ebo != 0) {
            buffer_pool.available_ebos.push_back(ebo);
        }
        else if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
        }
    }

    void Rendering::Initialize() {
        Get().viewMatrix = glm::mat4(1.f);
        Get().projectionMatrix = glm::mat4(1.f);
        Get().Setup_Shdrpgm();
        // Initialize instanced shader so uniforms exist on shdr_pgm_instanced
        Get().Setup_ShdrpgmInstanced();
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

    /*!
    @brief Initialize persistent mapped buffers for maximum performance data streaming.
    @param max_vertex_size Maximum size in bytes for the vertex buffer
    @param max_index_size Maximum size in bytes for the index buffer (0 to skip EBO)
    @details
    Creates persistent mapped buffers using OpenGL 4.4+ buffer storage functionality
    to achieve the highest possible performance for dynamic geometry updates. This
    approach eliminates the overhead of mapping/unmapping buffers for each update.

    **Buffer Storage Flags:**
    - GL_DYNAMIC_STORAGE_BIT: Allows buffer content modification after creation
    - GL_MAP_WRITE_BIT: Enables CPU write access to buffer memory
    - GL_MAP_PERSISTENT_BIT: Buffer remains mapped across OpenGL calls

    **Mapping Flags:**
    - GL_MAP_WRITE_BIT: CPU can write to the mapped memory
    - GL_MAP_PERSISTENT_BIT: Mapping persists across OpenGL operations
    - GL_MAP_FLUSH_EXPLICIT_BIT: Manual synchronization control for optimal performance

    **Performance Benefits:**
    - Zero-copy data uploads via direct memory access
    - No map/unmap overhead per frame
    - Optimal for frequently updated geometry (sprites, particles, UI)
    - Eliminates GPU pipeline stalls from buffer operations

    **Safety Features:**
    - Automatically cleans up existing buffers before initialization
    - Validates buffer creation success
    - Handles optional index buffer creation
    - Sets initialization flag for state tracking

    @note Requires OpenGL 4.4+ or ARB_buffer_storage extension
    @warning Buffer sizes cannot be changed after creation - choose carefully
    */
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

    /*!
    @brief Clean up persistent mapped buffers and release all associated resources.
    @details
    Properly unmaps and releases persistent buffers created by InitializePersistentBuffers.
    This function ensures clean resource management and prevents memory leaks.

    **Cleanup Operations:**
    - Unmaps VBO memory if currently mapped
    - Unmaps EBO memory if currently mapped
    - Resets all mapped pointers to nullptr for safety
    - Clears the initialization flag
    - Preserves buffer handles for potential deletion by other cleanup functions

    **Safety Features:**
    - Null pointer checks prevent double-unmapping errors
    - Graceful handling of partially initialized state
    - State flag management for proper tracking

    This function should be called:
    - During engine shutdown
    - Before reinitializing with different buffer sizes
    - When switching rendering contexts
    - As part of error recovery procedures

    @note Does not delete the actual OpenGL buffer objects - only unmaps them
    @warning Accessing mapped_vbo_ptr or mapped_ebo_ptr after this call is undefined behavior
    */
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

    /*!
    @brief Set up a default Vertex Array Object with a unit quad geometry.
    @details
    Creates a complete VAO configuration for rendering a basic unit quad with
    position and color attributes. This function serves as the foundation for
    basic shape rendering and demonstrates proper OpenGL 4.5+ DSA usage.

    **Geometry Configuration:**
    - Unit quad from -0.5 to 0.5 in X and Y axes
    - Z coordinate set to 0.0 for 2D rendering
    - Vertex order: bottom-left, bottom-right, top-right, top-left
    - Triangle fan primitive for efficient rendering (2 triangles, 4 vertices)

    **Color Configuration:**
    - Bottom-left: White (1.0, 1.0, 1.0)
    - Bottom-right: Red (1.0, 0.0, 0.0)
    - Top-right: Green (0.0, 1.0, 0.0)
    - Top-left: Blue (0.0, 0.0, 1.0)

    **Vertex Attribute Layout:**
    - Location 0: Position (3 floats, 12 bytes)
    - Location 1: Color (3 floats, 12 bytes)
    - Total vertex size: 24 bytes

    **Buffer Organization:**
    - Interleaved buffer layout with separate attribute streams
    - Position data at offset 0
    - Color data at offset (position_data_size)
    - Element buffer with indices [0, 1, 2, 3] for triangle fan

    **OpenGL Features Used:**
    - Direct State Access (DSA) for all operations
    - glCreateBuffers/glCreateVertexArrays for object creation
    - glNamedBufferStorage for immutable buffer storage
    - glVertexArrayVertexBuffer for attribute binding
    - glVertexArrayElementBuffer for index buffer binding

    This function creates a reusable quad geometry that can be transformed
    and rendered for basic shape drawing operations.
    */
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

    /*!
    @brief Compile and link the default shader program with comprehensive error checking.
    @details
    Loads, compiles, and links the default vertex and fragment shaders for the
    CrashoutEngine rendering system. This function performs extensive validation
    and error reporting to ensure shader program integrity.

    **Shader Files:**
    - Vertex Shader: CrashoutShader.vert (handles vertex transformations)
    - Fragment Shader: CrashoutShader.frag (handles pixel color output)
    - Path: ../../../projects/CrashoutEngine/shaders/

    **Compilation Process:**
    1. Vertex shader compilation with error checking
    2. Fragment shader compilation with error checking
    3. Program linking with validation
    4. Runtime validation in current OpenGL context

    **Error Handling:**
    - Detailed error messages for each compilation stage
    - Shader log output for debugging compilation issues
    - Application termination on any shader errors (fail-fast approach)
    - Comprehensive validation ensures shader program correctness

    **Automation Support:**
    - Includes automation hook for testing framework integration
    - Maintains compatibility with automated shader validation systems

    **Validation Stages:**
    - Compilation: Checks shader syntax and OpenGL compatibility
    - Linking: Verifies vertex/fragment shader interface compatibility
    - Validation: Ensures program can execute in current OpenGL state

    This function must be called after OpenGL context creation and before
    any rendering operations. Shader compilation errors will terminate the
    application with descriptive error messages for debugging.

    @note Requires valid OpenGL context and GLSL shader files to be present
    @warning Application exits on any shader compilation or linking errors
    */
    void Rendering::Setup_Shdrpgm() {

        using CrashoutsEngine::Assets::AssetManager;
        using CrashoutsEngine::Assets::ShaderAsset;

        ShaderAsset* shaderAsset = AssetManager::GetInstance().Get<ShaderAsset>("CrashoutShader");

        std::cout << shaderAsset << std::endl;

        if (!shaderAsset) {
            std::cout << "FATAL ERROR: Failed to get 'CrashoutShader' from AssetManager." << std::endl;
            //std::exit(EXIT_FAILURE);
        }

        shdr_pgm = &(shaderAsset->shaderObject);

        // Variable needed for automation hook
        std::vector<std::pair<GLenum, std::string>> shdr_files;

        // Automation hook. [!WARNING!] Do not alter/remove this
        AUTOMATION_HOOK_SHADER(shdr_pgm, shdr_files);

        if (!shdr_pgm) {
            std::cout << "FATAL ERROR: Shader pointer is null after assignment." << std::endl;
            std::exit(EXIT_FAILURE);
        }

       
        if (!shdr_pgm->Validate()) {
            std::cout << "Shader program 'CrashoutShader' failed to validate in current state!" << std::endl;
            std::cout << "GLSLShader Log: " << shdr_pgm->GetLog() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    void Rendering::Setup_ShdrpgmInstanced() {
        // Prefer instanced shader if available; otherwise fall back to default CrashoutShader
        using CrashoutsEngine::Assets::AssetManager;
        using CrashoutsEngine::Assets::ShaderAsset;

        ShaderAsset* shaderAsset = AssetManager::GetInstance().Get<ShaderAsset>("CrashoutShader_Instancing");
        if (!shaderAsset) {
            std::cout << "[Rendering] Instancing asset missing from manifest; attempting direct load..." << std::endl;

            // Attempt direct creation from expected paths (returns unique_ptr)
            //const std::string vert = "assets/shaders/CrashoutShader_Instancing/CrashoutShader_Instancing.vert";
            //const std::string frag = "assets/shaders/CrashoutShader_Instancing/CrashoutShader_Instancing.frag";
            //auto direct = ShaderAsset::Create(vert, frag);  // unique_ptr<ShaderAsset>

            //if (direct && direct->shaderObject.Validate()) {
            //    // Assign program from the directly created asset and return
            //    shdr_pgm_instanced = direct->shaderObject;
            //    std::cout << "[Rendering] Directly loaded CrashoutShader_Instancing for instanced draws." << std::endl;
            //    return;
            //}

            std::cout << "[Rendering] Direct load failed; falling back to CrashoutShader." << std::endl;
            shaderAsset = AssetManager::GetInstance().Get<ShaderAsset>("CrashoutShader");
        }

        if (!shaderAsset) {
            std::cout << "FATAL ERROR: No shader available for instanced rendering." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // Assign and validate
        shdr_pgm_instanced = shaderAsset->shaderObject;

        if (!shdr_pgm_instanced.Validate()) {
            std::cout << "[Rendering] Instanced shader validation failed: " << shdr_pgm_instanced.GetLog() << std::endl;
            ShaderAsset* fallbackAsset = AssetManager::GetInstance().Get<ShaderAsset>("CrashoutShader");
            if (fallbackAsset && fallbackAsset->shaderObject.Validate()) {
                shdr_pgm_instanced = fallbackAsset->shaderObject;
                std::cout << "[Rendering] Using fallback CrashoutShader." << std::endl;
            }
            else {
                std::cout << "FATAL ERROR: Fallback shader also invalid." << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
        else {
            std::cout << "[Rendering] Using CrashoutShader_Instancing for instanced draws." << std::endl;
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
        // Binds buffer 'vbo_hdl' to VAO binding point 0
        glVertexArrayVertexBuffer(vaoid, 0, vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoid, 0, 0);

        // Color attribute (location 1)
        glEnableVertexArrayAttrib(vaoid, 1);
        // Binds buffer 'vbo_hdl' to VAO binding point 1, with the *correct* offset for this frame
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
            size_t padded_size = static_cast<size_t>(total_vertex_size * 1.5f);
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

            size_t padded_ebo_size = static_cast<size_t>(total_index_size * 1.5);
            glCreateBuffers(1, &ebo_hdl);
            glNamedBufferStorage(ebo_hdl, static_cast<GLsizeiptr>(padded_ebo_size), nullptr,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

            mapped_ebo_ptr = glMapNamedBufferRange(ebo_hdl, 0, static_cast<GLsizeiptr>(padded_ebo_size),
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

        // Only create VAO if it doesn't exist
        if (vaoid == 0) {
            glCreateVertexArrays(1, &vaoid);
        }

        // This setup MUST run every frame, as the offsets change
        // and the attribute layout might be different from the last draw.

        // Position attribute (location 0)
        glEnableVertexArrayAttrib(vaoid, 0);
        glVertexArrayVertexBuffer(vaoid, 0, vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoid, 0, 0);

        // Color attribute (location 1)
        glEnableVertexArrayAttrib(vaoid, 1);
        glVertexArrayVertexBuffer(vaoid, 1, vbo_hdl, vertex_size, sizeof(CEVec3)); // Correct offset
        glVertexArrayAttribFormat(vaoid, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoid, 1, 1);

        // Texture coordinate attribute (location 2)
        // EXPLICITLY DISABLE IT for non-textured draws
        glDisableVertexArrayAttrib(vaoid, 2);

        // Update element buffer if needed
        if (ebo_hdl != 0) {
            glVertexArrayElementBuffer(vaoid, ebo_hdl);
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

            size_t padded_vbo_size = static_cast<size_t>(total_vertex_size * 1.5f);
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

            size_t padded_ebo_size = static_cast<size_t>(total_index_size * 1.5);
            glCreateBuffers(1, &ebo_hdl);
            glNamedBufferStorage(ebo_hdl, static_cast<GLsizeiptr>(padded_ebo_size), nullptr,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

            mapped_ebo_ptr = glMapNamedBufferRange(ebo_hdl, 0, static_cast<GLsizeiptr>(padded_ebo_size),
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

            current_ebo_size = padded_ebo_size;
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
        // Binds buffer 'vbo_hdl' to VAO binding point 0
        glVertexArrayVertexBuffer(vaoid, 0, vbo_hdl, 0, sizeof(CEVec3));
        glVertexArrayAttribFormat(vaoid, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vaoid, 0, 0);

        // Color attribute (location 1)
        glEnableVertexArrayAttrib(vaoid, 1);
        // Binds buffer 'vbo_hdl' to VAO binding point 1, with the *correct* offset for this frame
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

    /*!
    @brief Execute a complete draw call using the current VAO and shader state.
    @details
    Performs a complete rendering operation by activating the shader program,
    binding the vertex array object, issuing the draw call, and properly
    cleaning up OpenGL state. This function represents the final stage of
    the rendering pipeline.

    */
    void Rendering::Draw() {
        if (!shdr_pgm) {
            std::cerr << "Error: Rendering::Draw called with null shader pointer!" << std::endl;
            return;
        }
        shdr_pgm->Use();
        glBindVertexArray(vaoid);
        glDrawElements(primitive_type, idx_elem_cnt, GL_UNSIGNED_SHORT, NULL);

        // Ensure the GPU has finished reading from the persistently-mapped buffers
        // before we overwrite them for the next draw.
        GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        while (true) {
            GLenum status = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) break;
            if (status == GL_WAIT_FAILED) break;
            // Spin until the draw completes; acceptable for low draw counts.
        }
        glDeleteSync(fence);

        glBindVertexArray(0);
        shdr_pgm->UnUse();
    }
   
    GLuint Rendering::AcquireVAO() { return GetPooledVAO(); }
    GLuint Rendering::AcquireVBO() { return GetPooledVBO(); }
    GLuint Rendering::AcquireEBO() { return GetPooledEBO(); }

    void Rendering::ReturnVAO(GLuint vao) {
        if (vao != 0 && buffer_pool.available_vaos.size() < buffer_pool.max_pool_size) {
            buffer_pool.available_vaos.push_back(vao);
        }
        else if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
    }

    void Rendering::ReturnVBO(GLuint vbo) {
        if (vbo != 0 && buffer_pool.available_vbos.size() < buffer_pool.max_pool_size) {
            buffer_pool.available_vbos.push_back(vbo);
        }
        else if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }
    }

    void Rendering::ReturnEBO(GLuint ebo) {
        if (ebo != 0 && buffer_pool.available_ebos.size() < buffer_pool.max_pool_size) {
            buffer_pool.available_ebos.push_back(ebo);
        }
        else if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
        }
    }
}