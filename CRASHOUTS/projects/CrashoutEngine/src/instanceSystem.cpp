#include <cmath>
#include "instanceSystem.h"
#include "shapes.h"
#include "CEMath.h"
#include <glm/gtc/type_ptr.hpp>




namespace InstancingSystem {

   /* void RenderInstancedRectanglesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* widths,
        const float* heights,
        int count,
        const InstancedParams& params
    ) {
        if (count <= 0 || !positions || !colors || !rotations || !scales || !widths || !heights) return;

        if (!params.textured) {
            Component::Shapes::CreateInstancedRectangles(
                positions, colors, rotations, scales, widths, heights, count
            );
        } else {
            Component::Shapes::CreateInstancedRectanglesTextured(
                params.textureName, positions, colors, rotations, scales, widths, heights, count,
                params.tint, params.uvSize
            );
        }
    }

    void RenderInstancedTrianglesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* sideLengths,
        int count,
        const InstancedParams& params
    ) {
        if (count <= 0 || !positions || !colors || !rotations || !scales || !sideLengths) return;

        if (!params.textured) {
            Component::Shapes::CreateInstancedTriangles(
                positions, colors, rotations, scales, sideLengths, count
            );
        } else {
            Component::Shapes::CreateInstancedTrianglesTextured(
                params.textureName, positions, colors, rotations, scales, sideLengths, count,
                params.tint, params.uvSize
            );
        }
    }

    void RenderInstancedCirclesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* radii,
        int count,
        int segments,
        const InstancedParams& params
    ) {
        if (count <= 0 || !positions || !colors || !rotations || !scales || !radii) return;

        if (!params.textured) {
            Component::Shapes::CreateInstancedCircles(
                positions, colors, rotations, scales, radii, count, segments
            );
        } else {
            Component::Shapes::CreateInstancedCirclesTextured(
                params.textureName, positions, colors, rotations, scales, radii, count, segments,
                params.tint, params.uvSize
            );
        }
    }*/

    // Static buffers for GPU instancing (created once, reused)
    static GLuint s_instanceVAO = 0;
    static GLuint s_instanceVBO = 0;  // Base geometry
    static GLuint s_instanceEBO = 0;  // Indices
    static GLuint s_instanceDataVBO = 0;  // Instance data (positions, colors, etc.)
    static bool s_instanceBuffersInitialized = false;

    //Circle Instancing buffers
    static GLuint s_circInstanceVAO = 0;
    static GLuint s_circInstanceVBO = 0;
    static GLuint s_circInstanceEBO = 0;
    static GLuint s_circInstanceDataVBO = 0;
    static bool   s_circInstanceBuffersInitialized = false;
    static int    s_circSegmentsCurrent = 0;

    // Rectangle instancing buffers
    static GLuint s_rectInstanceVAO = 0;
    static GLuint s_rectInstanceVBO = 0;  // Base geometry
    static GLuint s_rectInstanceEBO = 0;  // Indices
    static GLuint s_rectInstanceDataVBO = 0;  // Instance data (positions, colors, etc.)
    static bool s_rectInstanceBuffersInitialized = false;

    // Triangle instancing buffers
    static GLuint s_triInstanceVAO = 0;
    static GLuint s_triInstanceVBO = 0;
    static GLuint s_triInstanceEBO = 0;
    static GLuint s_triInstanceDataVBO = 0;
    static bool   s_triInstanceBuffersInitialized = false;

   /* glm::mat4 CreateCenteredWorldProjection() {
        float aspect = static_cast<float>(GLApp::width) / GLApp::height;

        float worldWidth, worldHeight;

        if (aspect >= 1.0f) {

            worldHeight = WorldSettings::BASE_SIZE;
            worldWidth = worldHeight * aspect;
        }
        else {

            worldWidth = WorldSettings::BASE_SIZE;
            worldHeight = worldWidth / aspect;
        }

        return glm::ortho(
            -worldWidth / 2.0f, worldWidth / 2.0f,
            -worldHeight / 2.0f, worldHeight / 2.0f,
            -1.0f, 1.0f
        );
    }*/


    //.....................CIRCLE..............................................//
    void InstancingSystem::CreateInstancedCircles(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* radii,
        int count, int segments) {

        if (count <= 0) return;
        if (segments < 3) segments = 3;

        Component::Rendering& renderer = Component::Rendering::Get();

        
        if (!s_circInstanceBuffersInitialized || s_circSegmentsCurrent != segments) {
            if (s_circInstanceBuffersInitialized) {
                glDeleteVertexArrays(1, &s_circInstanceVAO);
                glDeleteBuffers(1, &s_circInstanceVBO);
                glDeleteBuffers(1, &s_circInstanceEBO);
                glDeleteBuffers(1, &s_circInstanceDataVBO);
                s_circInstanceBuffersInitialized = false;
            }

            // Generate unit circle triangle fan vertices (center + perimeter)
            std::vector<CEVec3> base_vertices;
            base_vertices.reserve(static_cast<size_t>(segments) + 1);
            base_vertices.emplace_back(0.0f, 0.0f, 0.0f);
            for (int i = 0; i <= segments; ++i) {
                float angle = 2.0f * static_cast<float>(CEMath::PI) * static_cast<float>(i) / static_cast<float>(segments);
                base_vertices.emplace_back(std::cos(angle), std::sin(angle), 0.0f);
            }

            // Triangle indices
            std::vector<GLushort> base_indices;
            base_indices.reserve(static_cast<size_t>(segments) * 3);
            for (int i = 0; i < segments; ++i) {
                base_indices.push_back(0);
                base_indices.push_back(static_cast<GLushort>(i + 1));
                base_indices.push_back(static_cast<GLushort>(i + 2));
            }

            glGenVertexArrays(1, &s_circInstanceVAO);
            glGenBuffers(1, &s_circInstanceVBO);
            glGenBuffers(1, &s_circInstanceEBO);
            glGenBuffers(1, &s_circInstanceDataVBO);

            glBindVertexArray(s_circInstanceVAO);

            glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, base_vertices.size() * sizeof(CEVec3), base_vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CEVec3), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_circInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, base_indices.size() * sizeof(GLushort), base_indices.data(), GL_STATIC_DRAW);

            // Instance attributes: pos(3), color(3), rotation(1), scale(1), width(1), height(1)
            glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);

            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            s_circSegmentsCurrent = segments;
            s_circInstanceBuffersInitialized = true;
            glBindVertexArray(0);
        }

        // Pack per-instance data; use radius for both width/height
        std::vector<float> instanceData;
        instanceData.reserve(count * 10);
        for (int i = 0; i < count; ++i) {
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);

            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);

            instanceData.push_back(rotations[i]);
            instanceData.push_back(scales[i]);
            instanceData.push_back(radii[i]); // width
            instanceData.push_back(radii[i]); // height
        }

        glBindVertexArray(s_circInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        renderer.shdr_pgm_instanced.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = Component::CreateCenteredWorldProjection();

        renderer.shdr_pgm_instanced.SetUniform("uModel", model);
        renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f);

        // Draw all circle triangles
        int elements = segments * 3;
        glDrawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_SHORT, 0, count);

        glBindVertexArray(0);

    }

    void InstancingSystem::CreateInstancedCirclesTextured(const std::string& textureName,
        const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* radii,
        int count, int segments,
        const CEVec3& tint,
        const CEVec2& uvSize) {
        if (count <= 0) return;
        if (segments < 3) segments = 3;

        Component::Rendering& renderer = Component::Rendering::Get();

        // Initialize/rebuild base geometry when segments change
        if (!s_circInstanceBuffersInitialized || s_circSegmentsCurrent != segments) {
            if (s_circInstanceBuffersInitialized) {
                glDeleteVertexArrays(1, &s_circInstanceVAO);
                glDeleteBuffers(1, &s_circInstanceVBO);
                glDeleteBuffers(1, &s_circInstanceEBO);
                glDeleteBuffers(1, &s_circInstanceDataVBO);
                s_circInstanceBuffersInitialized = false;
            }

            std::vector<CEVec3> base_vertices;
            base_vertices.reserve(static_cast<size_t>(segments) + 1);
            base_vertices.emplace_back(0.0f, 0.0f, 0.0f); // center
            const float step = 2.0f * static_cast<float>(CEMath::PI) / static_cast<float>(segments);
            for (int i = 0; i < segments; ++i) {
                float ang = i * step;
                base_vertices.emplace_back(std::cos(ang), std::sin(ang), 0.0f);
            }

            std::vector<GLushort> base_indices;
            base_indices.reserve(static_cast<size_t>(segments) * 3);
            for (int i = 0; i < segments; ++i) {
                GLushort i0 = 0;
                GLushort i1 = static_cast<GLushort>(1 + i);
                GLushort i2 = static_cast<GLushort>(1 + ((i + 1) % segments));
                base_indices.push_back(i0);
                base_indices.push_back(i1);
                base_indices.push_back(i2);
            }

            glGenVertexArrays(1, &s_circInstanceVAO);
            glGenBuffers(1, &s_circInstanceVBO);
            glGenBuffers(1, &s_circInstanceEBO);
            glGenBuffers(1, &s_circInstanceDataVBO);

            glBindVertexArray(s_circInstanceVAO);

            glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, base_vertices.size() * sizeof(CEVec3), base_vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CEVec3), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_circInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, base_indices.size() * sizeof(GLushort), base_indices.data(), GL_STATIC_DRAW);

            // Instance attributes: pos(3), color(3), rotation(1), scale(1), width(1), height(1)
            glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            s_circSegmentsCurrent = segments;
            s_circInstanceBuffersInitialized = true;
            glBindVertexArray(0);
        }

        // Pack per-instance data; use radius for width/height
        std::vector<float> instanceData;
        instanceData.reserve(count * 10);
        for (int i = 0; i < count; ++i) {
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);

            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);

            instanceData.push_back(rotations[i]);
            instanceData.push_back(scales[i]);
            instanceData.push_back(radii[i]); // width
            instanceData.push_back(radii[i]); // height
        }

        glBindVertexArray(s_circInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        // Bind texture and set shader uniforms
        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureName);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << textureName << std::endl;
            glBindVertexArray(0);
            return;
        }

        renderer.shdr_pgm_instanced.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = Component::CreateCenteredWorldProjection();

        renderer.shdr_pgm_instanced.SetUniform("uModel", model);
        renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);
        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);
        GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_uvSize");
        if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        int elements = segments * 3;
        glDrawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_SHORT, 0, count);
        glBindTextureUnit(0, 0);

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
    }

    void InstancingSystem::RenderInstancedCirclesBatch(
    const CEVec3* positions,
    const CEVec3* colors,
    const float* rotations,
    const float* scales,
    const float* radii,
    int count,
    int segments,
    const InstancedParams& params
    ) {
        if (count <= 0 || !positions || !colors || !rotations || !scales || !radii) return;

        if (!params.textured) {
            InstancingSystem::CreateInstancedCircles(
                positions, colors, rotations, scales, radii, count, segments
            );
        } else {
            InstancingSystem::CreateInstancedCirclesTextured(
                params.textureName, positions, colors, rotations, scales, radii, count, segments,
                params.tint, params.uvSize
            );
        }
    }
} 

//............................Rectangles..........................//

    void InstancingSystem::CreateInstancedRectangles(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* widths, const float* heights,
        int count) {
        
        if (count <= 0) return;

        Component::Rendering& renderer = Component::Rendering::Get();

        // Initialize rectangle instancing buffers once
        if (!s_rectInstanceBuffersInitialized) {
            // Base vertices with positions and texture coordinates
            float base_vertices[] = {
                // positions        // texture coords
                -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, // bottom left
                 0.5f, -0.5f, 0.0f,  1.0f, 1.0f, // bottom right
                 0.5f,  0.5f, 0.0f,  1.0f, 0.0f, // top right
                -0.5f,  0.5f, 0.0f,  0.0f, 0.0f  // top left
            };
            std::array<GLushort, 6> base_indices{ 0, 1, 2, 0, 2, 3 };

            glGenVertexArrays(1, &s_rectInstanceVAO);
            glGenBuffers(1, &s_rectInstanceVBO);
            glGenBuffers(1, &s_rectInstanceEBO);
            glGenBuffers(1, &s_rectInstanceDataVBO);

            glBindVertexArray(s_rectInstanceVAO);

            // Base geometry
            glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices, GL_STATIC_DRAW);
            
            // Position attribute (location 0)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            // Texture coordinate attribute (location 2)
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_rectInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

            // Instance data layout: pos(3), color(3), rotation(1), scale(1), width(1), height(1) = 10 floats
            glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);

            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            glBindVertexArray(0);
            
        }

        // Pack per-instance data
        std::vector<float> instanceData;
        instanceData.reserve(count * 10);
        for (int i = 0; i < count; ++i) {
            // position
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);
            // color
            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);
            // rotation, scale, width, height
            instanceData.push_back(rotations[i]);
            instanceData.push_back(scales[i]);
            instanceData.push_back(widths[i]);
            instanceData.push_back(heights[i]);
        }

        glBindVertexArray(s_rectInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        renderer.shdr_pgm_instanced.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = Component::CreateCenteredWorldProjection();

        renderer.shdr_pgm_instanced.SetUniform("uModel", model);
        renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

        // Disable texture usage for colored rectangles
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);

        // Optional modulation/tint defaults
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f);
        GLint modulate_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_modulate");
        if (modulate_loc != -1) glUniform1f(modulate_loc, 1.0f);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, count);

        glBindVertexArray(0);
    }

    void InstancingSystem::CreateInstancedRectanglesTextured(const std::string& textureAssetName,
        const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* widths, const float* heights,
        int count,
        const CEVec3& tint,
        const CEVec2& uvSize) {
        if (count <= 0) return;

        Component::Rendering& renderer = Component::Rendering::Get();

        // Initialize rectangle instancing buffers once
        if (!s_rectInstanceBuffersInitialized) {
            // Base vertices with positions and texture coordinates
            float base_vertices[] = {
                // positions        // texture coords
                -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, // bottom left
                 0.5f, -0.5f, 0.0f,  1.0f, 1.0f, // bottom right
                 0.5f,  0.5f, 0.0f,  1.0f, 0.0f, // top right
                -0.5f,  0.5f, 0.0f,  0.0f, 0.0f  // top left
            };
            std::array<GLushort, 6> base_indices{ 0, 1, 2, 0, 2, 3 };

            glGenVertexArrays(1, &s_rectInstanceVAO);
            glGenBuffers(1, &s_rectInstanceVBO);
            glGenBuffers(1, &s_rectInstanceEBO);
            glGenBuffers(1, &s_rectInstanceDataVBO);

            glBindVertexArray(s_rectInstanceVAO);

            // Base geometry
            glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices, GL_STATIC_DRAW);
            
            // Position attribute (location 0)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            // Texture coordinate attribute (location 2)
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_rectInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

            // Instance data layout: pos(3), color(3), rotation(1), scale(1), width(1), height(1) = 10 floats
            glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);

            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            s_rectInstanceBuffersInitialized = true;
            glBindVertexArray(0);
        }

        // Pack per-instance data
        std::vector<float> instanceData;
        instanceData.reserve(count * 10);
        for (int i = 0; i < count; ++i) {
            // position
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);
            // color
            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);
            // rotation, scale, width, height
            instanceData.push_back(rotations[i]);
            instanceData.push_back(scales[i]);
            instanceData.push_back(widths[i]);
            instanceData.push_back(heights[i]);
        }

        glBindVertexArray(s_rectInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        renderer.shdr_pgm_instanced.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = Component::CreateCenteredWorldProjection();

        renderer.shdr_pgm_instanced.SetUniform("uModel", model);
        renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

        // Enable texture usage
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        // Bind texture and set sampler to unit 0
        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureAssetName);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << textureAssetName << std::endl;
            glBindVertexArray(0);
            return;
        }
        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        // Optional tint and UV tiling
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);
        GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_uvSize");
        if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

        // Optional modulation
        GLint modulate_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_modulate");
        if (modulate_loc != -1) glUniform1f(modulate_loc, 1.0f);

        // Enable blending for alpha textures
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, count);
        glBindTextureUnit(0, 0);

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

    }

    //void InstancingSystem::CreateInstancedRectanglesTextured(const std::string& textureAssetName,
    //    const CEVec3* positions, const CEVec3* colors,
    //    const float* rotations, const float* scales,
    //    const float* widths, const float* heights,
    //    int count,
    //    const CEVec3& tint,
    //    const CEVec2& uvSize) {
    //    if (count <= 0) return;

    //    Component::Rendering& renderer = Component::Rendering::Get();

    //    // Initialize rectangle instancing buffers once
    //    if (!s_rectInstanceBuffersInitialized) {
    //        std::array<CEVec3, 4> base_vertices{
    //            CEVec3(-0.5f, -0.5f, 0.0f),
    //            CEVec3(0.5f, -0.5f, 0.0f),
    //            CEVec3(0.5f, 0.5f, 0.0f),
    //            CEVec3(-0.5f, 0.5f, 0.0f)
    //        };
    //        std::array<GLushort, 6> base_indices{ 0, 1, 2, 0, 2, 3 };

    //        glGenVertexArrays(1, &s_rectInstanceVAO);
    //        glGenBuffers(1, &s_rectInstanceVBO);
    //        glGenBuffers(1, &s_rectInstanceEBO);
    //        glGenBuffers(1, &s_rectInstanceDataVBO);

    //        glBindVertexArray(s_rectInstanceVAO);

    //        glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceVBO);
    //        glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices.data(), GL_STATIC_DRAW);

    //        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CEVec3), (void*)0);
    //        glEnableVertexAttribArray(0);

    //        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_rectInstanceEBO);
    //        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

    //        glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
    //        // position xyz, color rgb, rotation, scale, width, height = 10 floats
    //        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
    //        glEnableVertexAttribArray(3);
    //        glVertexAttribDivisor(3, 1);

    //        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
    //        glEnableVertexAttribArray(4);
    //        glVertexAttribDivisor(4, 1);

    //        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
    //        glEnableVertexAttribArray(5);
    //        glVertexAttribDivisor(5, 1);

    //        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
    //        glEnableVertexAttribArray(6);
    //        glVertexAttribDivisor(6, 1);

    //        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
    //        glEnableVertexAttribArray(7);
    //        glVertexAttribDivisor(7, 1);

    //        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
    //        glEnableVertexAttribArray(8);
    //        glVertexAttribDivisor(8, 1);

    //        s_rectInstanceBuffersInitialized = true;
    //        glBindVertexArray(0);
    //    }

    //    // Pack per-instance data
    //    std::vector<float> instanceData;
    //    instanceData.reserve(count * 10);
    //    for (int i = 0; i < count; ++i) {
    //        instanceData.push_back(positions[i].x);
    //        instanceData.push_back(positions[i].y);
    //        instanceData.push_back(positions[i].z);

    //        instanceData.push_back(colors[i].x);
    //        instanceData.push_back(colors[i].y);
    //        instanceData.push_back(colors[i].z);

    //        instanceData.push_back(rotations[i]);
    //        instanceData.push_back(scales[i]);
    //        instanceData.push_back(widths[i]);
    //        instanceData.push_back(heights[i]);
    //    }

    //    glBindVertexArray(s_rectInstanceVAO);
    //    glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
    //    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

    //    // Bind texture
    //    GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureAssetName);
    //    if (textureHandle == 0) {
    //        std::cerr << "Failed to load texture: " << textureAssetName << std::endl;
    //        glBindVertexArray(0);
    //        return;
    //    }

    //    renderer.shdr_pgm_instanced.Use();

    //    glm::mat4 model = glm::mat4(1.0f);
    //    glm::mat4 projection = Component::CreateCenteredWorldProjection();

    //    renderer.shdr_pgm_instanced.SetUniform("uModel", model);
    //    renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
    //    renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

    //    // Texture uniforms and tiling/tint
    //    GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
    //    if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);
    //    GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "uTex2d");
    //    if (tex_loc != -1) glUniform1i(tex_loc, 0);

    //    GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
    //    if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);
    //    GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_uvSize");
    //    if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

    //    // Optional modulation
    //    GLint modulate_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_modulate");
    //    if (modulate_loc != -1) glUniform1f(modulate_loc, 1.0f);

    //    // Enable blending for alpha textures
    //    glDepthMask(GL_FALSE);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //    glBindTextureUnit(0, textureHandle);
    //    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, count);
    //    glBindTextureUnit(0, 0);

    //    glBindVertexArray(0);
    //    glDepthMask(GL_TRUE);
    //}


    void InstancingSystem::RenderInstancedRectanglesBatch(
        const CEVec3* positions,
        const CEVec3* colors,
        const float* rotations,
        const float* scales,
        const float* widths,
        const float* heights,
        int count,
        const InstancedParams& params
    ) {
        if (count <= 0 || !positions || !colors || !rotations || !scales || !widths || !heights) return;

        if (!params.textured) {
            InstancingSystem::CreateInstancedRectangles(
                positions, colors, rotations, scales, widths, heights, count
            );
        } else {
            InstancingSystem::CreateInstancedRectanglesTextured(
                params.textureName, positions, colors, rotations, scales, widths, heights, count,
                params.tint, params.uvSize
            );
        }
    }

    void InstancingSystem::CreateInstancedTriangles(const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* sideLengths,
        int count) {

        if (count <= 0) return;

        Component::Rendering& renderer = Component::Rendering::Get();

        // Initialize triangle instancing buffers once
        if (!s_triInstanceBuffersInitialized) {
            // Equilateral triangle centered at origin, unit side (approx)
            std::array<CEVec3, 3> base_vertices{
                CEVec3(0.0f, 0.577350269f, 0.0f),    // top (sqrt(3)/3)
                CEVec3(-0.5f, -0.288675135f, 0.0f),  // bottom-left
                CEVec3(0.5f, -0.288675135f, 0.0f)    // bottom-right
            };
            std::array<GLushort, 3> base_indices{ 0, 1, 2 };

            glGenVertexArrays(1, &s_triInstanceVAO);
            glGenBuffers(1, &s_triInstanceVBO);
            glGenBuffers(1, &s_triInstanceEBO);
            glGenBuffers(1, &s_triInstanceDataVBO);

            glBindVertexArray(s_triInstanceVAO);

            glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CEVec3), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_triInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

            // Instance attributes: pos(3), color(3), rotation(1), scale(1), width(1), height(1) = 10 floats
            glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);

            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            s_triInstanceBuffersInitialized = true;
            glBindVertexArray(0);
        }

        // Pack per-instance data; width/height both use side length for uniform scaling
        std::vector<float> instanceData;
        instanceData.reserve(count * 10);
        for (int i = 0; i < count; ++i) {
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);

            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);

            instanceData.push_back(rotations[i]);
            instanceData.push_back(scales[i]);
            instanceData.push_back(sideLengths[i]);  // width
            instanceData.push_back(sideLengths[i]);  // height
        }

        glBindVertexArray(s_triInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        renderer.shdr_pgm_instanced.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = Component::CreateCenteredWorldProjection();

        renderer.shdr_pgm_instanced.SetUniform("uModel", model);
        renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, 1.0f, 1.0f, 1.0f);

        glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0, count);

        glBindVertexArray(0);

    }

    void InstancingSystem::CreateInstancedTrianglesTextured(const std::string& textureName,
        const CEVec3* positions, const CEVec3* colors,
        const float* rotations, const float* scales,
        const float* sideLengths,
        int count,
        const CEVec3& tint,
        const CEVec2& uvSize) {
        if (count <= 0) return;

        Component::Rendering& renderer = Component::Rendering::Get();

        // Initialize triangle base geometry once
        if (!s_triInstanceBuffersInitialized) {
            std::array<CEVec3, 3> base_vertices{
                CEVec3(0.0f, 0.577350269f, 0.0f),
                CEVec3(-0.5f, -0.288675135f, 0.0f),
                CEVec3(0.5f, -0.288675135f, 0.0f)
            };
            std::array<GLushort, 3> base_indices{ 0, 1, 2 };

            glGenVertexArrays(1, &s_triInstanceVAO);
            glGenBuffers(1, &s_triInstanceVBO);
            glGenBuffers(1, &s_triInstanceEBO);
            glGenBuffers(1, &s_triInstanceDataVBO);

            glBindVertexArray(s_triInstanceVAO);

            glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CEVec3), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_triInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

            // Instance layout: pos(3), color(3), rot(1), scale(1), width(1), height(1)
            glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)0);
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 7));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 8));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 10, (void*)(sizeof(float) * 9));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            s_triInstanceBuffersInitialized = true;
            glBindVertexArray(0);
        }

        // Pack per-instance data
        std::vector<float> instanceData;
        instanceData.reserve(count * 10);
        for (int i = 0; i < count; ++i) {
            instanceData.push_back(positions[i].x);
            instanceData.push_back(positions[i].y);
            instanceData.push_back(positions[i].z);

            instanceData.push_back(colors[i].x);
            instanceData.push_back(colors[i].y);
            instanceData.push_back(colors[i].z);

            instanceData.push_back(rotations[i]);
            instanceData.push_back(scales[i]);
            instanceData.push_back(sideLengths[i]);  // width
            instanceData.push_back(sideLengths[i]);  // height
        }

        glBindVertexArray(s_triInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureName);
        if (textureHandle == 0) {
            std::cerr << "Failed to load texture: " << textureName << std::endl;
            glBindVertexArray(0);
            return;
        }

        renderer.shdr_pgm_instanced.Use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 projection = Component::CreateCenteredWorldProjection();

        renderer.shdr_pgm_instanced.SetUniform("uModel", model);
        renderer.shdr_pgm_instanced.SetUniform("uView", glm::mat4(1.0f));
        renderer.shdr_pgm_instanced.SetUniform("uProjection", projection);

        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);
        GLint tex_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "uTex2d");
        if (tex_loc != -1) glUniform1i(tex_loc, 0);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform3f(tint_loc, tint.x, tint.y, tint.z);
        GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced.GetHandle(), "u_uvSize");
        if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTextureUnit(0, textureHandle);
        glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0, count);
        glBindTextureUnit(0, 0);

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
    }

    void InstancingSystem::RenderInstancedTrianglesBatch(
    const CEVec3* positions,
    const CEVec3* colors,
    const float* rotations,
    const float* scales,
    const float* sideLengths,
    int count,
    const InstancingSystem::InstancedParams& params
    ) {
        if (count <= 0 || !positions || !colors || !rotations || !scales || !sideLengths) return;

        if (!params.textured) {
            InstancingSystem::CreateInstancedTriangles(
                positions, colors, rotations, scales, sideLengths, count
            );
        } else {
            InstancingSystem::CreateInstancedTrianglesTextured(
                params.textureName, positions, colors, rotations, scales, sideLengths, count,
                params.tint, params.uvSize
            );
        }
    }
