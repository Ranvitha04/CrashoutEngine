/*!
@file   ppmanager.cpp
@author lee.r@digipen.edu
@date   05/11/2025

@brief  This file contains values and functions to manage post-processing.

All content ? 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "engine_pch.h"
#include "entitymanager.h"
#include "ppcomponent.h"
#include "assetmanager.h"
#include "shaderasset.h"
#include "rendering.h"
#include "ppmanager.h"
#include "engine.h"
#include "camera.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "collisionmanager.h"
#include "hierarchycomponent.h"
#include "jobsystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "textureasset.h"

/*                                                                   
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {

    namespace PostProcessing {

        using namespace Component;

        float g_brightness = 0.f;
        float g_contrast = 0.f;
        float g_saturation = 0.f;
        float g_ambient = 0.15f;
        bool g_bloomEnabled = true;
        float g_bloomThreshold = 1.0f;
        float g_bloomIntensity = 0.8f;
        int g_bloomIterations = 4;
        bool g_shadowsEnabled = true;
        int g_shadowRays = 48;
        bool active = false;

        GLuint quadvao = 0;
        GLuint quadvbo = 0;
        static GLuint lightInstanceVbo = 0;
        static GLuint occluderVao = 0;
        static GLuint occluderVbo = 0;
        GLuint ppFboID = 0;
        GLuint ppTextureID = 0;

        static GLuint frontLightsFbo = 0;
        static GLuint frontLightsTex[2]{ 0, 0 }; // [0] = Bloom, [1] = NoBloom
        static GLuint behindLightsFbo = 0;
        static GLuint behindLightsTex[2]{ 0, 0 }; // [0] = Bloom, [1] = NoBloom
        static GLuint occluderFbo = 0;
        static GLuint occluderTexture = 0;
        static GLuint combineFbo = 0;
        static GLuint combineTexture = 0;
        static GLuint brightFbo = 0;
        static GLuint brightTexture = 0;
        static GLuint pingpongFbo[2]{ 0, 0 };
        static GLuint pingpongTex[2]{ 0, 0 };
        static GLuint shadowTex = 0;
        static int shadowTexWidth = 0;
        static int shadowTexHeight = 0;
        static int cachedWidth = 0;
        static int cachedHeight = 0;

        struct LightDraw {
            bool active{ false };
            bool affectBloom{ true };
            bool renderBehind{ false };
            bool ignoreParentOccluders{ true };
            glm::vec2 uvCenter{ 0.0f, 0.0f };
            float radiusUV{ 0.0f };
            CEVec3 worldPos{ 0.0f, 0.0f, 0.0f };
            float radiusWorld{ 0.0f };
            CEVec3 color{ 1.0f, 1.0f, 1.0f };
            float intensity{ 1.0f };
            int shadowRow{ -1 };
        };

        struct LightInstance {
            float posX;
            float posY;
            float radiusWorld;
            float colorR;
            float colorG;
            float colorB;
            float intensity;
            float shadowIndex;
            float affectBloom;
        };


        static void CreateTexture(GLuint& tex, int w, int h, GLenum internalFormat, GLenum format) {
            if (tex == 0) glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        static void CreateFboWithTexturesMRT(GLuint& fbo, GLuint tex[2], int w, int h) {
            if (fbo == 0) glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            for (int i = 0; i < 2; ++i) {
                if (tex[i] == 0) glGenTextures(1, &tex[i]);
                glBindTexture(GL_TEXTURE_2D, tex[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tex[i], 0);
            }

            GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "ERROR: Post-Processing MRT Framebuffer is not complete!" << std::endl;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        static void CreateFboWithTexture(GLuint& fbo, GLuint& tex, int w, int h) {
            if (fbo == 0) glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            CreateTexture(tex, w, h, GL_RGBA16F, GL_RGBA);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "ERROR: Post-Processing Framebuffer is not complete!" << std::endl;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        static void EnsureTargets() {
            if (gameTextureWidth == 0 || gameTextureHeight == 0) return;
            if (cachedWidth == gameTextureWidth && cachedHeight == gameTextureHeight) return;

            cachedWidth = gameTextureWidth;
            cachedHeight = gameTextureHeight;
            int halfW = std::max(1, cachedWidth / 2);
            int halfH = std::max(1, cachedHeight / 2);

            CreateFboWithTexturesMRT(frontLightsFbo, frontLightsTex, cachedWidth, cachedHeight);
            CreateFboWithTexturesMRT(behindLightsFbo, behindLightsTex, cachedWidth, cachedHeight);
            CreateFboWithTexture(occluderFbo, occluderTexture, cachedWidth, cachedHeight);
            CreateFboWithTexture(combineFbo, combineTexture, cachedWidth, cachedHeight);
            CreateFboWithTexture(brightFbo, brightTexture, halfW, halfH);
            CreateFboWithTexture(pingpongFbo[0], pingpongTex[0], halfW, halfH);
            CreateFboWithTexture(pingpongFbo[1], pingpongTex[1], halfW, halfH);

            if (shadowTex == 0) glGenTextures(1, &shadowTex);

#if USE_IMGUI_EDITOR
            if (ppFboID == 0) glGenFramebuffers(1, &ppFboID);
            glBindFramebuffer(GL_FRAMEBUFFER, ppFboID);
            if (ppTextureID == 0) glGenTextures(1, &ppTextureID);
            glBindTexture(GL_TEXTURE_2D, ppTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cachedWidth, cachedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppTextureID, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "ERROR: Post-Processing Framebuffer is not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
        }

        void InitPP() {
            // --- Create the Full-Screen Quad ---
            glGenVertexArrays(1, &quadvao);
            glGenBuffers(1, &quadvbo);

            glBindVertexArray(quadvao);
            glBindBuffer(GL_ARRAY_BUFFER, quadvbo);

            // Pass the vertices from your header
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticesNDC), &quadVerticesNDC, GL_STATIC_DRAW);

            // Attribute 0: Position (vec2)
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

            // Attribute 1: TexCoords (vec2)
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            // Instance buffer for lights
            glGenBuffers(1, &lightInstanceVbo);
            glBindBuffer(GL_ARRAY_BUFFER, lightInstanceVbo);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

            // layout(location = 2) vec2 iLightPos
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
            glVertexAttribDivisor(2, 1);

            // layout(location = 3) float iLightRadius
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(2 * sizeof(float)));
            glVertexAttribDivisor(3, 1);

            // layout(location = 4) vec3 iLightColor
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
            glVertexAttribDivisor(4, 1);

            // layout(location = 5) float iIntensity
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
            glVertexAttribDivisor(5, 1);

            // layout(location = 6) float iShadowIndex
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
            glVertexAttribDivisor(6, 1);

            // layout(location = 7) float iAffectBloom
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
            glVertexAttribDivisor(7, 1);

            glBindVertexArray(0); // Unbind

            // Occluder quad VAO (dynamic)
            glGenVertexArrays(1, &occluderVao);
            glGenBuffers(1, &occluderVbo);
            glBindVertexArray(occluderVao);
            glBindBuffer(GL_ARRAY_BUFFER, occluderVbo);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glBindVertexArray(0);

            EnsureTargets();
        }

        void ShutdownPP() {
            glDeleteVertexArrays(1, &quadvao);
            glDeleteBuffers(1, &quadvbo);
            glDeleteBuffers(1, &lightInstanceVbo);
            glDeleteVertexArrays(1, &occluderVao);
            glDeleteBuffers(1, &occluderVbo);

#if USE_IMGUI_EDITOR
            glDeleteFramebuffers(1, &ppFboID);
            glDeleteTextures(1, &ppTextureID);
#endif
            glDeleteFramebuffers(1, &frontLightsFbo);
            glDeleteTextures(2, frontLightsTex);
            glDeleteFramebuffers(1, &behindLightsFbo);
            glDeleteTextures(2, behindLightsTex);
            glDeleteFramebuffers(1, &occluderFbo);
            glDeleteTextures(1, &occluderTexture);
            glDeleteFramebuffers(1, &combineFbo);
            glDeleteTextures(1, &combineTexture);
            glDeleteFramebuffers(1, &brightFbo);
            glDeleteTextures(1, &brightTexture);
            glDeleteFramebuffers(2, pingpongFbo);
            glDeleteTextures(2, pingpongTex);
            if (shadowTex != 0) {
                glDeleteTextures(1, &shadowTex);
                shadowTex = 0;
                shadowTexWidth = 0;
                shadowTexHeight = 0;
            }
        }

        void UpdatePP() {
            EnsureTargets();

            // Get reference
            ECS& gECS = *ECS::getInstance();
            ECS::ComponentArray<PPComponent>& ppArray = gECS.GetComponentArray<PPComponent>();

            if (ppArray.indexToEntity.size() > 0) {
                PPComponent& p = *ppArray.dense.back();
                if (!p.active) {
                    if (active != p.active) active = p.active;
                    return;
                }

                if (g_brightness != p.brightness) g_brightness = p.brightness;
                if (g_contrast != p.contrast) g_contrast = p.contrast;
                if (g_saturation != p.saturation) g_saturation = p.saturation;
                if (g_ambient != p.ambient) g_ambient = p.ambient;
                if (g_bloomEnabled != p.bloomEnabled) g_bloomEnabled = p.bloomEnabled;
                if (g_bloomThreshold != p.bloomThreshold) g_bloomThreshold = p.bloomThreshold;
                if (g_bloomIntensity != p.bloomIntensity) g_bloomIntensity = p.bloomIntensity;
                if (g_bloomIterations != p.bloomIterations) g_bloomIterations = p.bloomIterations;
                if (g_shadowsEnabled != p.shadowsEnabled) g_shadowsEnabled = p.shadowsEnabled;
                if (g_shadowRays != p.shadowRays) g_shadowRays = p.shadowRays;
                if (active != p.active) active = p.active;
            }
        }

        static void RenderFullscreen(GLSLShader& shader, GLuint tex0, const char* uniform0, GLuint tex1 = 0, const char* uniform1 = nullptr) {
            shader.Use();
            if (uniform0) {
                glUniform1i(glGetUniformLocation(shader.GetHandle(), uniform0), 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex0);
            }
            if (uniform1) {
                glUniform1i(glGetUniformLocation(shader.GetHandle(), uniform1), 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, tex1);
            }

            glBindVertexArray(quadvao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            Component::Rendering::Get().drawCalls++;
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        static void RenderFallback(GLuint sceneTextureID) {
            auto* ppAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("PPShader");
            if (!ppAsset) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, ppFboID);
            glViewport(0, 0, gameTextureWidth, gameTextureHeight);
            glClear(GL_COLOR_BUFFER_BIT);

            GLSLShader& ppShader = ppAsset->shaderObject;
            ppShader.Use();
            glUniform1i(glGetUniformLocation(ppShader.GetHandle(), "u_active"), active ? 1 : 0);
            glUniform1f(glGetUniformLocation(ppShader.GetHandle(), "u_brightness"), g_brightness);
            glUniform1f(glGetUniformLocation(ppShader.GetHandle(), "u_contrast"), g_contrast);
            glUniform1f(glGetUniformLocation(ppShader.GetHandle(), "u_saturation"), g_saturation);
            RenderFullscreen(ppShader, sceneTextureID, "u_sceneTexture");
        }

        static void renderLightsMRT(GLuint targetFbo, bool renderBehind, auto const& lightDraws, auto const& shadowRowByLight, int shadowTexHeight, bool shadowsEnabled, GLuint shadowTex, const RenderCamera::Camera& currentCamera) {
            glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);
            glViewport(0, 0, gameTextureWidth, gameTextureHeight);

            GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);

            glDisable(GL_DEPTH_TEST);
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);
            glEnable(GL_BLEND);
            // Set independent blending for each render target
            glBlendFunci(0, GL_ONE, GL_ONE); // For bloom lights
            glBlendFunci(1, GL_ONE, GL_ONE); // For non-bloom lights


            auto* lightAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("LightShader");
            if (!lightAsset) {
                glDisable(GL_BLEND);
                return;
            }
            GLSLShader& lightShader = lightAsset->shaderObject;
            lightShader.Use();
            float camWidth = currentCamera.GetVisibleWorldWidth();
            float camHeight = currentCamera.GetVisibleWorldHeight();
            glUniform2f(glGetUniformLocation(lightShader.GetHandle(), "uWorldSize"), camWidth, camHeight);
            glUniform1i(glGetUniformLocation(lightShader.GetHandle(), "uUseShadows"), (shadowsEnabled && !shadowRowByLight.empty()) ? 1 : 0);
            glUniform1f(glGetUniformLocation(lightShader.GetHandle(), "uShadowTexHeight"), (float)shadowTexHeight);
            glUniform1i(glGetUniformLocation(lightShader.GetHandle(), "uShadowTex"), 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowTex);

            static std::vector<LightInstance> instances;
            instances.clear();
            instances.reserve(lightDraws.size());

            for (size_t i = 0; i < lightDraws.size(); ++i) {
                const LightDraw& ld = lightDraws[i];
                if (!ld.active || ld.renderBehind != renderBehind) continue;

                LightInstance inst{};
                inst.posX = ld.uvCenter.x;
                inst.posY = ld.uvCenter.y;
                inst.radiusWorld = ld.radiusWorld;
                inst.colorR = ld.color.x;
                inst.colorG = ld.color.y;
                inst.colorB = ld.color.z;
                inst.intensity = ld.intensity;
                inst.shadowIndex = (shadowRowByLight[i] >= 0) ? (float)shadowRowByLight[i] : -1.0f;
                inst.affectBloom = ld.affectBloom ? 1.0f : 0.0f;
                instances.push_back(inst);
            }

            if (!instances.empty()) {
                glBindVertexArray(quadvao);
                glBindBuffer(GL_ARRAY_BUFFER, lightInstanceVbo);
                glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(LightInstance), instances.data(), GL_DYNAMIC_DRAW);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(instances.size()));
                Component::Rendering::Get().drawCalls++;
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            glDisable(GL_BLEND);
        }

        void RenderPP(GLuint sceneTextureID) {
            EnsureTargets();

            ECS& ecs = *ECS::getInstance();

            // Determine active camera
#if USE_IMGUI_EDITOR
            auto& currentCamera = (engineState == EngineState::STOP) ? RenderCamera::engineCamera : RenderCamera::gameCamera;
#else
            auto& currentCamera = RenderCamera::gameCamera;
#endif
            glm::mat4 view = currentCamera.GetViewMatrix();
            glm::mat4 proj = currentCamera.GetProjectionMatrix();

            auto& lightArray = ecs.GetComponentArray<LightComponent>();
            static std::vector<LightDraw> lightDraws;
            lightDraws.clear();
            lightDraws.resize(lightArray.dense.size());

            // Precompute light screen-space data and occluder rects off the render loop.
            CrashoutsEngine::JobSystem::Get().ParallelFor(static_cast<unsigned int>(lightArray.dense.size()),
                [&](unsigned int start, unsigned int end) {
                    for (unsigned int i = start; i < end; ++i) {
                        LightComponent* lc = lightArray.dense[i];
                        if (!lc || !lc->active || !lc->renderSelf) {
                            lightDraws[i].active = false;
                            continue;
                        }

                        Entity ent = lightArray.indexToEntity[i];
                        CEVec3 worldPos = lc->position;
                        if (lc->useTransform && ecs.HasComponent<TransformComponent>(ent)) {
                            worldPos = ecs.GetComponent<TransformComponent>(ent)->position;
                        }

                        glm::vec4 clip = proj * view * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
                        if (clip.w == 0.0f) {
                            lightDraws[i].active = false;
                            continue;
                        }
                        glm::vec3 ndc = glm::vec3(clip) / clip.w;
                        glm::vec2 uvCenter = glm::vec2(ndc.x, ndc.y) * 0.5f + 0.5f;

                        // Radius in screen UV
                        glm::vec4 clipR = proj * view * glm::vec4(worldPos.x + lc->radius, worldPos.y, worldPos.z, 1.0f);
                        glm::vec3 ndcR = glm::vec3(clipR) / clipR.w;
                        glm::vec2 uvR = glm::vec2(ndcR.x, ndcR.y) * 0.5f + 0.5f;
                        float radiusUV = glm::length(uvR - uvCenter);

                        // Off-screen culling in UV space
                        if (radiusUV <= 0.0f ||
                            (uvCenter.x + radiusUV) < 0.0f || (uvCenter.x - radiusUV) > 1.0f ||
                            (uvCenter.y + radiusUV) < 0.0f || (uvCenter.y - radiusUV) > 1.0f) {
                            lightDraws[i].active = false;
                            continue;
                        }

                        lightDraws[i].active = true;
                        lightDraws[i].affectBloom = lc->affectBloom;
                        lightDraws[i].renderBehind = lc->renderBehind;
                        lightDraws[i].ignoreParentOccluders = lc->ignoreParentOccluders;
                        lightDraws[i].uvCenter = uvCenter;
                        lightDraws[i].radiusUV = radiusUV;
                        lightDraws[i].worldPos = worldPos;
                        lightDraws[i].radiusWorld = lc->radius;
                        lightDraws[i].color = lc->color;
                        lightDraws[i].intensity = lc->intensity;

                        // Occluder rectangles are built on the main thread after this pass.
                    }
                });

            // --- Build shadow map rows ---
            static std::vector<int> shadowRowByLight;
            shadowRowByLight.assign(lightDraws.size(), -1);
            static std::vector<size_t> shadowLights;
            shadowLights.clear();
            shadowLights.reserve(lightDraws.size());

            if (g_shadowsEnabled) {
                for (size_t i = 0; i < lightDraws.size(); ++i) {
                    if (!lightDraws[i].active) continue;
                    shadowRowByLight[i] = static_cast<int>(shadowLights.size());
                    shadowLights.push_back(i);
                }
            }

            int shadowRays = std::max(8, g_shadowRays);
            int shadowRows = static_cast<int>(shadowLights.size());

            if (shadowRows < 1) shadowRows = 1;
            if (shadowTexWidth != shadowRays || shadowTexHeight != shadowRows) {
                shadowTexWidth = shadowRays;
                shadowTexHeight = shadowRows;
                glBindTexture(GL_TEXTURE_2D, shadowTex);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, shadowTexWidth, shadowTexHeight, 0, GL_RED, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            static std::vector<float> shadowData;
            shadowData.assign(shadowTexWidth * shadowTexHeight, 1.0f);

            if (!shadowLights.empty()) {
                float camWidth = currentCamera.GetVisibleWorldWidth();
                float camHeight = currentCamera.GetVisibleWorldHeight();

                // Get camera bounds to clamp massive lights
                CEVec3 camPos = currentCamera.GetPosition();
                float maxScreenRadius = std::max(camWidth, camHeight) * 1.5f;

                // 1. PRE-COMPUTE CANDIDATES ON THE MAIN THREAD (Thread Safe!)
                // We use a vector of vectors so each light has its own dedicated list of colliders to check.
                std::vector<std::vector<CrashoutsEngine::Entity>> allLightCandidates(shadowLights.size());

                for (size_t idx = 0; idx < shadowLights.size(); ++idx) {
                    size_t lightIndex = shadowLights[idx];
                    const LightDraw& ld = lightDraws[lightIndex];
                    int row = shadowRowByLight[lightIndex];
                    if (row < 0) continue;

                    // Clamp the query region so massive lights don't query the entire map!
                    float queryRadius = std::min(ld.radiusWorld, maxScreenRadius);

                    Collision::AABBRect lightRegion(
                        CEVec3{ ld.worldPos.x - queryRadius, ld.worldPos.y - queryRadius, 0.f },
                        CEVec3{ ld.worldPos.x + queryRadius, ld.worldPos.y + queryRadius, 0.f }
                    );

                    allLightCandidates[idx].reserve(64);
                    Collision::CollisionHandler::GetInstance().quadTree.Query(lightRegion, allLightCandidates[idx]);

                    // PRE-UPDATE COLLIDERS ON MAIN THREAD
                    // This prevents multiple threads from calling UpdateCollider on the same entity later.
                    for (auto ent : allLightCandidates[idx]) {
                        auto* cc = ecs.GetComponent<Component::ColliderComponent>(ent);
                        auto* tc = ecs.GetComponent<Component::TransformComponent>(ent);
                        if (cc && cc->collider && tc) {
                            cc->collider->UpdateCollider(tc->position);
                            if (cc->collider->type == Collision::ColliderType::OBB ||
                                cc->collider->type == Collision::ColliderType::POLYGON ||
                                cc->collider->type == Collision::ColliderType::TRIANGLE) {
                                cc->collider->UpdateRotation(tc->rotation);
                            }
                        }
                    }
                }

                // 2. RUN THE HEAVY RAYCAST MATH ON WORKER THREADS
                CrashoutsEngine::JobSystem::Get().ParallelFor(static_cast<unsigned int>(shadowLights.size()),
                    [&](unsigned int start, unsigned int end) {
                        for (unsigned int idx = start; idx < end; ++idx) {
                            size_t lightIndex = shadowLights[idx];
                            const LightDraw& ld = lightDraws[lightIndex];
                            Entity ent = lightArray.indexToEntity[lightIndex];
                            int row = shadowRowByLight[lightIndex];
                            if (row < 0) continue;

                            const auto& localCandidates = allLightCandidates[idx];

                            // --- CACHE PARENT LOGIC ONCE ---
                            CrashoutsEngine::Entity ignoredParent = 0;
                            if (ld.ignoreParentOccluders) {
                                ignoredParent = ent;
                                if (ecs.HasComponent<Component::ParentComponent>(ent)) {
                                    auto* pc = ecs.GetComponent<Component::ParentComponent>(ent);
                                    if (pc && pc->parent != 0) ignoredParent = pc->parent;
                                }
                            }

                            // --- CAST ALL RAYS AGAINST THE PRE-FILTERED LIST ---
                            for (int r = 0; r < shadowRays; ++r) {
                                float angle = (static_cast<float>(r) + 0.5f) / static_cast<float>(shadowRays);
                                angle *= 6.28318530718f;
                                float ux = std::cos(angle);
                                float uy = std::sin(angle);

                                // Map UV direction to world direction
                                CEVec3 dir{ ux * camWidth, uy * camHeight, 0.f };
                                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                                if (len > 0.0001f) {
                                    dir.x /= len;
                                    dir.y /= len;
                                }

                                CEVec3 startPos = ld.worldPos;
                                CEVec3 endPos = startPos + dir * ld.radiusWorld;
                                CEVec3 hitPoint{};
                                CrashoutsEngine::Entity hitEnt = 0;

                                // --- USE THE NEW FAST RAYCAST ---
                                bool hit = Collision::CollisionHandler::GetInstance().RaycastAgainstList(
                                    startPos, endPos, localCandidates, hitPoint, hitEnt, ent, ignoredParent
                                );

                                float dist = ld.radiusWorld;
                                if (hit) {
                                    CEVec3 delta = hitPoint - startPos;
                                    dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                                    if (dist > ld.radiusWorld) dist = ld.radiusWorld;
                                }

                                float norm = (ld.radiusWorld > 0.0001f) ? (dist / ld.radiusWorld) : 1.0f;
                                shadowData[row * shadowRays + r] = norm;
                            }
                        }
                    });

                glBindTexture(GL_TEXTURE_2D, shadowTex);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shadowTexWidth, shadowTexHeight, GL_RED, GL_FLOAT, shadowData.data());
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            // --- Light Passes ---
            renderLightsMRT(frontLightsFbo, false, lightDraws, shadowRowByLight, shadowTexHeight, g_shadowsEnabled, shadowTex, currentCamera);
            renderLightsMRT(behindLightsFbo, true, lightDraws, shadowRowByLight, shadowTexHeight, g_shadowsEnabled, shadowTex, currentCamera);

            // --- Occluder Mask Pass ---
            glBindFramebuffer(GL_FRAMEBUFFER, occluderFbo);
            glViewport(0, 0, gameTextureWidth, gameTextureHeight);
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);

            {
                struct OccVertex { float x, y, u, v; };
                auto* occAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("OccluderMaskShader");
                if (!occAsset) {
                    // Skip occluder pass if shader is missing
                }
                else {
                    GLSLShader& occShader = occAsset->shaderObject;
                occShader.Use();
                glEnable(GL_BLEND);
                glBlendEquation(GL_MAX);
                glBlendFunc(GL_ONE, GL_ONE);
                glDisable(GL_CULL_FACE);

                glBindVertexArray(occluderVao);
                glBindBuffer(GL_ARRAY_BUFFER, occluderVbo);

                for (size_t i = 0; i < lightDraws.size(); ++i) {
                    if (!lightDraws[i].active || !lightDraws[i].renderBehind) continue;
                    LightComponent* lc = lightArray.dense[i];
                    if (!lc) continue;
                    Entity ent = lightArray.indexToEntity[i];
                    if (!ecs.HasComponent<TransformComponent>(ent)) continue;
                    if (!ecs.HasComponent<ShapeComponent>(ent)) continue;

                    auto* tc = ecs.GetComponent<TransformComponent>(ent);
                    auto* sc = ecs.GetComponent<ShapeComponent>(ent);
                    if (!tc || !sc) continue;

                    GLuint texHandle = 0;
                    if (sc->enableTexture) {
                        texHandle = Component::TextureRenderer::GetTextureHandle(sc->textureName);
                        if (texHandle == 0) continue;
                    }

                    float u0 = sc->uvOffset.x;
                    float v0 = sc->uvOffset.y;
                    float u1 = u0 + sc->uvSize.x;
                    float v1 = v0 + sc->uvSize.y;

                    glm::vec2 half(tc->size.x * 0.5f, tc->size.y * 0.5f);
                    float rot = tc->rotation;
                    float c = std::cos(rot);
                    float s = std::sin(rot);
                    CEVec3 worldPos = lc->position;
                    if (lc->useTransform) worldPos = tc->position;

                    glm::vec2 local[4] = {
                        { -half.x, -half.y }, // BL
                        {  half.x, -half.y }, // BR
                        {  half.x,  half.y }, // TR
                        { -half.x,  half.y }  // TL
                    };

                    glm::vec2 ndc[4];
                    bool ok = true;
                    for (int k = 0; k < 4; ++k) {
                        glm::vec2 r = {
                            local[k].x * c - local[k].y * s,
                            local[k].x * s + local[k].y * c
                        };
                        glm::vec4 clipC = proj * view * glm::vec4(worldPos.x + r.x, worldPos.y + r.y, worldPos.z, 1.0f);
                        if (clipC.w == 0.0f) { ok = false; break; }
                        glm::vec3 ndcC = glm::vec3(clipC) / clipC.w;
                        ndc[k] = { ndcC.x, ndcC.y };
                    }
                    if (!ok) continue;

                    OccVertex verts[6] = {
                        { ndc[0].x, ndc[0].y, u0, v1 }, // BL
                        { ndc[1].x, ndc[1].y, u1, v1 }, // BR
                        { ndc[2].x, ndc[2].y, u1, v0 }, // TR
                        { ndc[0].x, ndc[0].y, u0, v1 }, // BL
                        { ndc[2].x, ndc[2].y, u1, v0 }, // TR
                        { ndc[3].x, ndc[3].y, u0, v0 }  // TL
                    };

                    glUniform1i(glGetUniformLocation(occShader.GetHandle(), "uUseTexture"), sc->enableTexture ? 1 : 0);
                    glUniform1f(glGetUniformLocation(occShader.GetHandle(), "uAlpha"), sc->color.w());
                    if (sc->enableTexture) {
                        glUniform1i(glGetUniformLocation(occShader.GetHandle(), "uTex2d"), 0);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, texHandle);
                    }

                    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    Component::Rendering::Get().drawCalls++;
                }


                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBlendEquation(GL_FUNC_ADD);
                glDisable(GL_BLEND);
                glEnable(GL_CULL_FACE);
                }
            }

            // --- Combine Pass ---
            glBindFramebuffer(GL_FRAMEBUFFER, combineFbo);
            glViewport(0, 0, gameTextureWidth, gameTextureHeight);
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);

            auto* combineAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("CombineShader");
            if (!combineAsset) {
                RenderFallback(sceneTextureID);
                return;
            }
            GLSLShader& combineShader = combineAsset->shaderObject;
            combineShader.Use();
            glUniform1f(glGetUniformLocation(combineShader.GetHandle(), "uAmbient"), g_ambient);
            glUniform1i(glGetUniformLocation(combineShader.GetHandle(), "uLightBehind"), 2);
            glUniform1i(glGetUniformLocation(combineShader.GetHandle(), "uOccluderMask"), 3);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, behindLightsTex[0]);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, occluderTexture);
            RenderFullscreen(combineShader, sceneTextureID, "uScene", frontLightsTex[0], "uLight");
            glBindTexture(GL_TEXTURE_2D, 0);

            // --- Bright Pass ---
            int halfW = std::max(1, gameTextureWidth / 2);
            int halfH = std::max(1, gameTextureHeight / 2);
            glBindFramebuffer(GL_FRAMEBUFFER, brightFbo);
            glViewport(0, 0, halfW, halfH);
            glClear(GL_COLOR_BUFFER_BIT);

            auto* brightAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("BrightShader");
            if (!brightAsset) {
                RenderFallback(combineTexture);
                return;
            }
            GLSLShader& brightShader = brightAsset->shaderObject;
            brightShader.Use();
            glUniform1f(glGetUniformLocation(brightShader.GetHandle(), "uThreshold"), g_bloomThreshold);
            RenderFullscreen(brightShader, combineTexture, "uScene");

            // --- Blur Pass (Kawase) ---
            GLuint blurInput = brightTexture;
            int iterations = std::max(0, g_bloomIterations);
            auto* blurAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("BlurShader");
            if (!blurAsset) {
                RenderFallback(combineTexture);
                return;
            }
            GLSLShader& blurShader = blurAsset->shaderObject;

            if (iterations > 0) {
                blurShader.Use();
                glUniform2f(glGetUniformLocation(blurShader.GetHandle(), "uTexelSize"), 1.0f / halfW, 1.0f / halfH);

                for (int i = 0; i < iterations; ++i) {
                    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFbo[i % 2]);
                    glViewport(0, 0, halfW, halfH);
                    glClear(GL_COLOR_BUFFER_BIT);

                    glUniform1i(glGetUniformLocation(blurShader.GetHandle(), "uPass"), i);

                    RenderFullscreen(blurShader, blurInput, "uImage");
                    blurInput = pingpongTex[i % 2];
                }
            }
            
            GLuint bloomTex = (iterations > 0) ? blurInput : brightTexture;

            // --- Final Composite (to currently bound framebuffer) ---
            glBindFramebuffer(GL_FRAMEBUFFER, ppFboID);
            glViewport(0, 0, gameTextureWidth, gameTextureHeight);
            glClear(GL_COLOR_BUFFER_BIT);

            auto* finalAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("BloomCompositeShader");
            if (!finalAsset) {
                RenderFallback(combineTexture);
                return;
            }
            GLSLShader& finalShader = finalAsset->shaderObject;
            finalShader.Use();
            glUniform1i(glGetUniformLocation(finalShader.GetHandle(), "uUseBloom"), g_bloomEnabled ? 1 : 0);
            glUniform1f(glGetUniformLocation(finalShader.GetHandle(), "uBloomIntensity"), g_bloomIntensity);
            glUniform1i(glGetUniformLocation(finalShader.GetHandle(), "uActive"), active ? 1 : 0);
            glUniform1f(glGetUniformLocation(finalShader.GetHandle(), "uBrightness"), g_brightness);
            glUniform1f(glGetUniformLocation(finalShader.GetHandle(), "uContrast"), g_contrast + 1.f);
            glUniform1f(glGetUniformLocation(finalShader.GetHandle(), "uSaturation"), g_saturation + 1.f);
            glUniform1i(glGetUniformLocation(finalShader.GetHandle(), "uNoBloomLight"), 2);
            glUniform1i(glGetUniformLocation(finalShader.GetHandle(), "uNoBloomBehind"), 3);
            glUniform1i(glGetUniformLocation(finalShader.GetHandle(), "uOccluderMask"), 4);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, frontLightsTex[1]);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, behindLightsTex[1]);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, occluderTexture);
            RenderFullscreen(finalShader, combineTexture, "uScene", bloomTex, "uBloom");
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    }

}








