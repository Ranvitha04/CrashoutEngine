#include <cmath>
#include "instanceSystem.h"
#include "shapes.h"
#include "CEMath.h"
#include <glm/gtc/type_ptr.hpp>
#include <random>

// Global variables needed for ECS and texturing
static CrashoutsEngine::ECS* globalECS = nullptr;
const std::string textureDucky = "ducky.png";
static bool gEcsRectsInitialized = false;
static bool gEcsTexturesEnabled = false;

// Function to initialize the globalECS variable
static void EnsureGlobalECSInitialized() {
    if (!globalECS) {
        globalECS = CrashoutsEngine::ECS::getInstance();
    }
}

namespace InstancingSystem {

    //Circle Instancing buffers
    static GLuint s_circInstanceVAO = 0;
    static GLuint s_circInstanceVBO = 0;
    static GLuint s_circInstanceEBO = 0;
    static GLuint s_circInstanceDataVBO = 0;
    static bool   s_circInstanceBuffersInitialized = false;
    static int    s_circSegmentsCurrent = 0;

    // Rectangle instancing buffers
    static GLuint s_rectInstanceVAO = 0;
    static GLuint s_rectInstanceVBO = 0;  
    static GLuint s_rectInstanceEBO = 0;  
    static GLuint s_rectInstanceDataVBO = 0;  
    static bool s_rectInstanceBuffersInitialized = false;

    // Triangle instancing buffers
    static GLuint s_triInstanceVAO = 0;
    static GLuint s_triInstanceVBO = 0;
    static GLuint s_triInstanceEBO = 0;
    static GLuint s_triInstanceDataVBO = 0;
    static bool   s_triInstanceBuffersInitialized = false;

 

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

            // Acquire pooled buffers (or create new) via renderer
            s_circInstanceVAO = Component::Rendering::Get().GetPooledVAO();
            s_circInstanceVBO = Component::Rendering::Get().GetPooledVBO();
            s_circInstanceEBO = Component::Rendering::Get().GetPooledEBO();
            s_circInstanceDataVBO = Component::Rendering::Get().GetPooledVBO();

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

            s_circInstanceVAO = renderer.GetPooledVAO();
            s_circInstanceVBO = renderer.GetPooledVBO();
            s_circInstanceEBO = renderer.GetPooledEBO();
            s_circInstanceDataVBO = renderer.GetPooledVBO();

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

    static void ComputeWorldDimensions(float& worldWidth, float& worldHeight) {
        const float BASE_SIZE = 100.0f;
        float aspect = static_cast<float>(GLApp::width) / static_cast<float>(GLApp::height);
        if (aspect >= 1.0f) {
            worldHeight = BASE_SIZE;
            worldWidth = worldHeight * aspect;
        }
        else {
            worldWidth = BASE_SIZE;
            worldHeight = worldWidth / aspect;
        }
    }

// Rectangles (Mode 4)
    static std::vector<CEVec3> gM4RectPos;
    static std::vector<CEVec3> gM4RectColor;    
    static std::vector<float>  gM4RectRot;
    static std::vector<float>  gM4RectW;
    static std::vector<float>  gM4RectH;

    // Triangles (Mode 4)
    static std::vector<CEVec3> gM4TriPos;
    static std::vector<CEVec3> gM4TriColor;     
    static std::vector<float>  gM4TriRot;
    static std::vector<float>  gM4TriScale;     
    static std::vector<float>  gM4TriSide;     

    // Circles (Mode 4)
    static std::vector<CEVec3> gM4CircPos;
    static std::vector<CEVec3> gM4CircColor;    
    static std::vector<float>  gM4CircRot;
    static std::vector<float>  gM4CircScale;    
    static std::vector<float>  gM4CircRadius;
    static int gM4CircSegments = 24;

    void InstancingSystem::InitializeMode4RandomShapes(int perTypeCount) {
        //gMode4Initialized = false;

        //ClearRectangleData();

        // Clear all
        gM4RectPos.clear(); gM4RectColor.clear(); gM4RectRot.clear(); gM4RectW.clear(); gM4RectH.clear();
        gM4TriPos.clear();  gM4TriColor.clear();  gM4TriRot.clear();  gM4TriScale.clear(); gM4TriSide.clear();
        gM4CircPos.clear(); gM4CircColor.clear(); gM4CircRot.clear(); gM4CircScale.clear(); gM4CircRadius.clear();

        // Reserve
        gM4RectPos.reserve(perTypeCount);gM4RectColor.reserve(perTypeCount);
        gM4RectRot.reserve(perTypeCount); gM4RectW.reserve(perTypeCount); gM4RectH.reserve(perTypeCount);
        gM4TriPos.reserve(perTypeCount);  gM4TriColor.reserve(perTypeCount);
        gM4TriRot.reserve(perTypeCount);  gM4TriScale.reserve(perTypeCount); gM4TriSide.reserve(perTypeCount);
        gM4CircPos.reserve(perTypeCount); gM4CircColor.reserve(perTypeCount);
        gM4CircRot.reserve(perTypeCount); gM4CircScale.reserve(perTypeCount); gM4CircRadius.reserve(perTypeCount);

        float worldW, worldH;
        ComputeWorldDimensions(worldW, worldH);

        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
        std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
        std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);

        std::uniform_real_distribution<float> col01(0.2f, 1.0f);
        std::uniform_real_distribution<float> rectW(1.0f, 6.0f);
        std::uniform_real_distribution<float> rectH(1.0f, 6.0f);
        std::uniform_real_distribution<float> triSide(1.0f, 6.0f);
        std::uniform_real_distribution<float> circRadius(1.0f, 4.0f);

        // Rectangles
        for (int i = 0; i < perTypeCount; ++i) {
            gM4RectPos.emplace_back(posX(rng), posY(rng), 0.0f);
            gM4RectColor.emplace_back(col01(rng), col01(rng), col01(rng));
            gM4RectRot.emplace_back(rotDeg(rng));
            gM4RectW.emplace_back(rectW(rng));
            gM4RectH.emplace_back(rectH(rng));
        }

        // Triangles (equilateral side length)
      /*  for (int i = 0; i < perTypeCount; ++i) {
            gM4TriPos.emplace_back(posX(rng), posY(rng), 0.0f);
            gM4TriColor.emplace_back(col01(rng), col01(rng), col01(rng));
            gM4TriRot.emplace_back(rotDeg(rng));
            gM4TriScale.emplace_back(1.0f);
            gM4TriSide.emplace_back(triSide(rng));
        }*/

        // Circles
        for (int i = 0; i < perTypeCount; ++i) {
            gM4CircPos.emplace_back(posX(rng), posY(rng), 0.0f);
            gM4CircColor.emplace_back(col01(rng), col01(rng), col01(rng));
            gM4CircRot.emplace_back(rotDeg(rng));
            gM4CircScale.emplace_back(1.0f);
            gM4CircRadius.emplace_back(circRadius(rng));
        }

        //gMode4Initialized = true;
    }

    // Build Mode 5 ECS-driven rectangles + triangles; circles generated separately
    void InitializeMode5EcsRandomShapes(
        CrashoutsEngine::ECS* ecs,
        int perTypeCount,
        bool texturesEnabled, const std::string& textureName,
        // Rectangles out
        std::vector<CEVec3>& rectPos, std::vector<CEVec3>& rectColor,
        std::vector<float>& rectRot, std::vector<float>& rectScale,
        std::vector<float>& rectW, std::vector<float>& rectH,
        // Triangles out
        std::vector<CEVec3>& triPos, std::vector<CEVec3>& triColor,
        std::vector<float>& triRot, std::vector<float>& triScale, std::vector<float>& triSide,
        // Circles out (not ECS)
        std::vector<CEVec3>& circPos, std::vector<CEVec3>& circColor,
        std::vector<float>& circRot, std::vector<float>& circScale, std::vector<float>& circRadius,
        int& circSegments
    ) {
        rectPos.clear(); rectColor.clear(); rectRot.clear(); rectScale.clear(); rectW.clear(); rectH.clear();
        triPos.clear(); triColor.clear(); triRot.clear(); triScale.clear(); triSide.clear();
        circPos.clear(); circColor.clear(); circRot.clear(); circScale.clear(); circRadius.clear();

        rectScale.reserve(perTypeCount);
        rectPos.reserve(perTypeCount); rectColor.reserve(perTypeCount); rectRot.reserve(perTypeCount);
        rectW.reserve(perTypeCount); rectH.reserve(perTypeCount);
        triPos.reserve(perTypeCount); triColor.reserve(perTypeCount); triRot.reserve(perTypeCount);
        triScale.reserve(perTypeCount); triSide.reserve(perTypeCount);
        circPos.reserve(perTypeCount); circColor.reserve(perTypeCount); circRot.reserve(perTypeCount);
        circScale.reserve(perTypeCount); circRadius.reserve(perTypeCount);

        float worldW = 0.0f, worldH = 0.0f;
        ComputeWorldDimensions(worldW, worldH);

        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
        std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
        std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);
        std::uniform_real_distribution<float> widthDist(worldW * 0.01f, worldW * 0.05f);
        std::uniform_real_distribution<float> heightDist(worldH * 0.01f, worldH * 0.05f);
        std::uniform_real_distribution<float> triSideDist(worldW * 0.01f, worldW * 0.05f);
        std::uniform_int_distribution<int> colorDist(70, 255);
        std::uniform_real_distribution<float> circRadiusDist(worldW * 0.01f, worldW * 0.04f);

        // Rectangles via ECS + derived instancing arrays
        for (int i = 0; i < perTypeCount; ++i) {
            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);
            float w = widthDist(rng);
            float h = heightDist(rng);

            rectPos.emplace_back(pos);
            rectColor.emplace_back(
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f
            );
            rectRot.emplace_back(rotation);
            rectScale.emplace_back(1.0f);
            rectW.emplace_back(w);
            rectH.emplace_back(h);

            if (ecs) {
                auto e = ecs->CreateEntity();
                ecs->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Rect_" + std::to_string(e) });
                ecs->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
                    rotation, CEVec2{1.0f, 1.0f}, pos
                });

                Component::ShapeComponent shape{};
                shape.type = Component::ShapeType::Rectangle;
                shape.width = w;
                shape.height = h;
                shape.color = CEVec3{
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng))
                };
                shape.enableTexture = texturesEnabled;
                shape.textureName = textureName;
                shape.uvSize = CEVec2{ 1.0f, 1.0f };

                ecs->AddComponent<Component::ShapeComponent>(e, shape);
            }
        }

        // Triangles via ECS + derived instancing arrays
        for (int i = 0; i < perTypeCount; ++i) {
            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);
            float side = triSideDist(rng);

            triPos.emplace_back(pos);
            triColor.emplace_back(
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f
            );
            triRot.emplace_back(rotation);
            triScale.emplace_back(1.0f);
            triSide.emplace_back(side);

            if (ecs) {
                auto e = ecs->CreateEntity();
                ecs->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Tri_" + std::to_string(e) });
                ecs->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
                    rotation, CEVec2{1.0f, 1.0f}, pos
                });

                Component::ShapeComponent shape{};
                shape.type = Component::ShapeType::Triangle;
                shape.width = side;
                shape.height = side;
                shape.color = CEVec3{
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng))
                };
                shape.enableTexture = texturesEnabled;
                shape.textureName = textureName;
                shape.uvSize = CEVec2{ 1.0f, 1.0f };

                ecs->AddComponent<Component::ShapeComponent>(e, shape);
            }
        }

        // Circles are generated without ECS
        for (int i = 0; i < perTypeCount; ++i) {
            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);
            float radius = circRadiusDist(rng);

            circPos.emplace_back(pos);
            circColor.emplace_back(
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f
            );
            circRot.emplace_back(rotation);
            circScale.emplace_back(1.0f);
            circRadius.emplace_back(radius);
        }

        if (circSegments <= 0) circSegments = 24;
    }
} 

//............................Rectangles..........................//

static std::vector<CEVec3> gInstPositions;
static std::vector<CEVec3> gInstColors;
static std::vector<float> gInstRotations;
static std::vector<float> gInstScales;
static std::vector<float> gInstWidths;
static std::vector<float> gInstHeights;

void InstancingSystem::InitializeEcsRectanglesDataRandom(int count) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;

    float worldW = 0.0f, worldHeight = 0.0f;
    InstancingSystem::ComputeWorldDimensions(worldW, worldHeight);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    std::uniform_real_distribution<float> posY(-worldHeight * 0.5f, worldHeight * 0.5f);
    std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);

    // Sizes scaled to world dimensions, similar to non-ECS rectangles
    std::uniform_real_distribution<float> widthDist(worldW * 0.01f, worldW * 0.05f);
    std::uniform_real_distribution<float> heightDist(worldHeight * 0.01f, worldHeight * 0.05f);

    // 0�255 range; renderer multiplies by 1/255.f later
    std::uniform_int_distribution<int> colorDist(70, 255);

    for (int i = 0; i < count; ++i) {
        auto e = globalECS->CreateEntity();

        CEVec3 pos{ posX(rng), posY(rng), 0.0f };
        float rotation = rotDeg(rng);
        float w = widthDist(rng);
        float h = heightDist(rng);

        // Name + Transform
        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Rect_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
            rotation, CEVec2{1.0f, 1.0f}, pos
            });

        // Shape
        Component::ShapeComponent shape{};
        shape.type = Component::ShapeType::Rectangle;
        shape.width = w;
        shape.height = h;
        shape.color = CEVec3{
            static_cast<float>(colorDist(rng)),
            static_cast<float>(colorDist(rng)),
            static_cast<float>(colorDist(rng))
        };

        shape.enableTexture = gEcsTexturesEnabled;
        shape.textureName = textureDucky; // "ducky.png"
        shape.uvSize = CEVec2{ 1.0f, 1.0f };

        globalECS->AddComponent<Component::ShapeComponent>(e, shape);
    }

    gEcsRectsInitialized = true;
}

void InstancingSystem::InitializeInstancedRectanglesData(int count) {

    ClearRectangleData();

    gInstPositions.reserve(count);
    gInstColors.reserve(count);
    gInstRotations.reserve(count);
    gInstScales.reserve(count);
    gInstWidths.reserve(count);
    gInstHeights.reserve(count);

    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(-worldW / 2.0f, worldW / 2.0f);
    std::uniform_real_distribution<float> distY(-worldH / 2.0f, worldH / 2.0f);
    std::uniform_real_distribution<float> distC(0.2f, 1.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
    std::uniform_real_distribution<float> distScale(0.5f, 2.0f);
    std::uniform_real_distribution<float> distWidth(1.0f, 4.0f);
    std::uniform_real_distribution<float> distHeight(1.0f, 4.0f);

    for (int i = 0; i < count; ++i) {
        gInstPositions.emplace_back(distX(gen), distY(gen), 0.0f);
        gInstColors.emplace_back(distC(gen), distC(gen), distC(gen));
        gInstRotations.emplace_back(distRot(gen));
        gInstScales.emplace_back(distScale(gen));
        gInstWidths.emplace_back(distWidth(gen));
        gInstHeights.emplace_back(distHeight(gen));
    }
    //gRectanglesInitialized = true;
}

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

            s_rectInstanceVAO     = Component::Rendering::Get().GetPooledVAO();
            s_rectInstanceVBO     = Component::Rendering::Get().GetPooledVBO();
            s_rectInstanceEBO     = Component::Rendering::Get().GetPooledEBO();
            s_rectInstanceDataVBO = Component::Rendering::Get().GetPooledVBO();

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

            s_rectInstanceVAO = renderer.GetPooledVAO();
            s_rectInstanceVBO = renderer.GetPooledVBO();
            s_rectInstanceEBO = renderer.GetPooledEBO();
            s_rectInstanceDataVBO = renderer.GetPooledVBO();

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

    void InstancingSystem::ClearRectangleData() {

        gInstPositions.clear();
        gInstColors.clear();
        gInstRotations.clear();
        gInstScales.clear();
        gInstWidths.clear();
        gInstHeights.clear();


    }

    void InstancingSystem::RenderInstancedRectangles(const InstancedParams& params) {
        int count = static_cast<int>(gInstPositions.size());
        std::cout << count << std::endl;
        if (count <= 0) return;

        const CEVec3* positions = gInstPositions.data();
        const CEVec3* colors = gInstColors.data();
        const float* rotations = gInstRotations.data();
        const float* scales = gInstScales.data();
        const float* widths = gInstWidths.data();
        const float* heights = gInstHeights.data();

        if (!positions || !colors || !rotations || !scales || !widths || !heights) return;

        if (!params.textured) {
            InstancingSystem::CreateInstancedRectangles(
                positions, colors, rotations, scales, widths, heights, count
            );
        }
        else {
            InstancingSystem::CreateInstancedRectanglesTextured(
                params.textureName, positions, colors, rotations, scales, widths, heights, count,
                params.tint, params.uvSize
            );
        }
    }

    
    void InstancingSystem::RenderMode4Rectangles(const InstancedParams& params) {
        int count = static_cast<int>(gM4RectPos.size());
        if (count <= 0) return;


        static std::vector<float> s_rectScales;
        if (static_cast<int>(s_rectScales.size()) != count) {
            s_rectScales.assign(count, 1.0f);
        } else {
            std::fill(s_rectScales.begin(), s_rectScales.end(), 1.0f);
        }

        if (!params.textured) {
            InstancingSystem::CreateInstancedRectangles(
                gM4RectPos.data(), gM4RectColor.data(),
                gM4RectRot.data(), s_rectScales.data(),
                gM4RectW.data(), gM4RectH.data(),
                count
            );
        } else {
            InstancingSystem::CreateInstancedRectanglesTextured(
                params.textureName,
                gM4RectPos.data(), gM4RectColor.data(),
                gM4RectRot.data(), s_rectScales.data(),
                gM4RectW.data(), gM4RectH.data(),
                count,
                params.tint, params.uvSize
            );
        }
    }

    void InstancingSystem::RenderMode4Triangles(const InstancedParams& params) {
    int count = static_cast<int>(gM4TriPos.size());
    if (count <= 0) return;

    if (!params.textured) {
        InstancingSystem::CreateInstancedTriangles(
            gM4TriPos.data(), gM4TriColor.data(),
            gM4TriRot.data(), gM4TriScale.data(),
            gM4TriSide.data(),
            count
        );
    } else {
        InstancingSystem::CreateInstancedTrianglesTextured(
            params.textureName,
            gM4TriPos.data(), gM4TriColor.data(),
            gM4TriRot.data(), gM4TriScale.data(),
            gM4TriSide.data(),
            count,
            params.tint, params.uvSize
        );
    }
}

void InstancingSystem::RenderMode4Circles(const InstancedParams& params) {
    int count = static_cast<int>(gM4CircPos.size());
    if (count <= 0) return;

    if (!params.textured) {
        InstancingSystem::CreateInstancedCircles(
            gM4CircPos.data(), gM4CircColor.data(),
            gM4CircRot.data(), gM4CircScale.data(),
            gM4CircRadius.data(),
            count,
            gM4CircSegments
        );
    } else {
        InstancingSystem::CreateInstancedCirclesTextured(
            params.textureName,
            gM4CircPos.data(), gM4CircColor.data(),
            gM4CircRot.data(), gM4CircScale.data(),
            gM4CircRadius.data(),
            count,
            gM4CircSegments,
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

            s_triInstanceVAO     = Component::Rendering::Get().GetPooledVAO();
            s_triInstanceVBO     = Component::Rendering::Get().GetPooledVBO();
            s_triInstanceEBO     = Component::Rendering::Get().GetPooledEBO();
            s_triInstanceDataVBO = Component::Rendering::Get().GetPooledVBO();

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

            s_triInstanceVAO = renderer.GetPooledVAO();
            s_triInstanceVBO = renderer.GetPooledVBO();
            s_triInstanceEBO = renderer.GetPooledEBO();
            s_triInstanceDataVBO = renderer.GetPooledVBO();

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
