/*!
@file   ppmanager.cpp
@author Ranvitha
@author Shahrin Idzuan
@author Renfred Lee
@date   05/11/2025

@brief  This file contains values and functions to manage post-processing.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/* includes
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
#include "glapp.h"
#include "activecomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "tagcomponent.h"
#include "scriptcomponent.h"
#include "collisionmanager.h"
#include "hierarchycomponent.h"
#include "jobsystem.h"
#include "performanceSystem.h" // <--- ADDED PROFiler HEADER
#include <iostream>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "textureasset.h"

/* ----------------------------------------------------------------------------- */
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
        float g_shadowDarkness = 1.0f;
        CEVec3 g_shadowColor{ 0.0f, 0.0f, 0.0f };
        float g_shadowColorStrength = 1.0f;
        float g_shadowMapStrength = 1.0f;
        float g_raymarchShadowStrength = 1.0f;
        float g_timeSeconds = 0.0f;
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
        static GLuint sceneOccluderFbo = 0;
        static GLuint sceneOccluderTexture = 0;
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
            bool ignoreOccluders{ true };
            int shadowTechnique{ 0 };
            int raymarchSteps{ 24 };
            float raymarchThreshold{ 0.5f };
            bool flickerEnabled{ false };
            float flickerStrength{ 0.0f };
            float flickerSpeed{ 0.0f };
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
            float shadowTechnique;
            float raymarchSteps;
            float raymarchThreshold;
            float flickerStrength;
            float flickerSpeed;
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

            int blurW = std::max(1, cachedWidth / 4);
            int blurH = std::max(1, cachedHeight / 4);

            CreateFboWithTexturesMRT(frontLightsFbo, frontLightsTex, cachedWidth, cachedHeight);
            CreateFboWithTexturesMRT(behindLightsFbo, behindLightsTex, cachedWidth, cachedHeight);
            CreateFboWithTexture(occluderFbo, occluderTexture, cachedWidth, cachedHeight);
            CreateFboWithTexture(sceneOccluderFbo, sceneOccluderTexture, cachedWidth, cachedHeight);
            CreateFboWithTexture(combineFbo, combineTexture, cachedWidth, cachedHeight);

            CreateFboWithTexture(brightFbo, brightTexture, blurW, blurH);
            CreateFboWithTexture(pingpongFbo[0], pingpongTex[0], blurW, blurH);
            CreateFboWithTexture(pingpongFbo[1], pingpongTex[1], blurW, blurH);

            if (shadowTex == 0) glGenTextures(1, &shadowTex);

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
        }

        void InitPP() {
            glGenVertexArrays(1, &quadvao);
            glGenBuffers(1, &quadvbo);

            glBindVertexArray(quadvao);
            glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticesNDC), &quadVerticesNDC, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            glGenBuffers(1, &lightInstanceVbo);
            glBindBuffer(GL_ARRAY_BUFFER, lightInstanceVbo);
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
            glVertexAttribDivisor(2, 1);

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(2 * sizeof(float)));
            glVertexAttribDivisor(3, 1);

            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
            glVertexAttribDivisor(4, 1);

            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
            glVertexAttribDivisor(5, 1);

            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(7 * sizeof(float)));
            glVertexAttribDivisor(6, 1);

            // layout(location = 7) float iAffectBloom
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
            glVertexAttribDivisor(7, 1);

            // layout(location = 8) float iShadowTechnique
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(9 * sizeof(float)));
            glVertexAttribDivisor(8, 1);

            // layout(location = 9) float iRaymarchSteps
            glEnableVertexAttribArray(9);
            glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(10 * sizeof(float)));
            glVertexAttribDivisor(9, 1);

            // layout(location = 10) float iRaymarchThreshold
            glEnableVertexAttribArray(10);
            glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
            glVertexAttribDivisor(10, 1);

            glEnableVertexAttribArray(11);
            glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(12 * sizeof(float)));
            glVertexAttribDivisor(11, 1);

            glEnableVertexAttribArray(12);
            glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(13 * sizeof(float)));
            glVertexAttribDivisor(12, 1);

            glBindVertexArray(0); // Unbind

            // Occluder quad VAO (dynamic)
            glGenVertexArrays(1, &occluderVao);
            glGenBuffers(1, &occluderVbo);
            glBindVertexArray(occluderVao);
            glBindBuffer(GL_ARRAY_BUFFER, occluderVbo);
            glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
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

            glDeleteFramebuffers(1, &ppFboID);
            glDeleteTextures(1, &ppTextureID);
            glDeleteFramebuffers(1, &frontLightsFbo);
            glDeleteTextures(2, frontLightsTex);
            glDeleteFramebuffers(1, &behindLightsFbo);
            glDeleteTextures(2, behindLightsTex);
            glDeleteFramebuffers(1, &occluderFbo);
            glDeleteTextures(1, &occluderTexture);
            glDeleteFramebuffers(1, &sceneOccluderFbo);
            glDeleteTextures(1, &sceneOccluderTexture);
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

            ECS& gECS = *ECS::getInstance();
            ECS::ComponentArray<PPComponent>& ppArray = gECS.GetComponentArray<PPComponent>();

            if (ppArray.indexToEntity.size() > 0) {
                PPComponent& p = *ppArray.dense.back();
                if (!p.active) {
                    if (active != p.active) active = p.active;
                    return;
                }

                g_timeSeconds += static_cast<float>(GLApp::delta_time);
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
                if (g_shadowDarkness != p.shadowDarkness) g_shadowDarkness = p.shadowDarkness;
                if (g_shadowColor != p.shadowColor) g_shadowColor = p.shadowColor;
                if (g_shadowColorStrength != p.shadowColorStrength) g_shadowColorStrength = p.shadowColorStrength;
                if (g_shadowMapStrength != p.shadowMapStrength) g_shadowMapStrength = p.shadowMapStrength;
                if (g_raymarchShadowStrength != p.raymarchShadowStrength) g_raymarchShadowStrength = p.raymarchShadowStrength;
                if (active != p.active) active = p.active;
            }
        }

        static void RenderFullscreenCached(GLSLShader& shader, GLuint tex0, GLint loc0, GLuint tex1 = 0, GLint loc1 = -1) {
            shader.Use();
            if (loc0 != -1) {
                glUniform1i(loc0, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex0);
            }
            if (loc1 != -1) {
                glUniform1i(loc1, 1);
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

            static GLint loc_active = glGetUniformLocation(ppShader.GetHandle(), "u_active");
            static GLint loc_bright = glGetUniformLocation(ppShader.GetHandle(), "u_brightness");
            static GLint loc_contrast = glGetUniformLocation(ppShader.GetHandle(), "u_contrast");
            static GLint loc_saturation = glGetUniformLocation(ppShader.GetHandle(), "u_saturation");
            static GLint loc_sceneTex = glGetUniformLocation(ppShader.GetHandle(), "u_sceneTexture");

            if (loc_active == -1) loc_active = glGetUniformLocation(ppShader.GetHandle(), "u_active");
            if (loc_bright == -1) loc_bright = glGetUniformLocation(ppShader.GetHandle(), "u_brightness");
            if (loc_contrast == -1) loc_contrast = glGetUniformLocation(ppShader.GetHandle(), "u_contrast");
            if (loc_saturation == -1) loc_saturation = glGetUniformLocation(ppShader.GetHandle(), "u_saturation");
            if (loc_sceneTex == -1) loc_sceneTex = glGetUniformLocation(ppShader.GetHandle(), "u_sceneTexture");

            glUniform1i(loc_active, active ? 1 : 0);
            glUniform1f(loc_bright, g_brightness);
            glUniform1f(loc_contrast, g_contrast);
            glUniform1f(loc_saturation, g_saturation);

            RenderFullscreenCached(ppShader, sceneTextureID, loc_sceneTex);
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
            glBlendFunci(0, GL_ONE, GL_ONE);
            glBlendFunci(1, GL_ONE, GL_ONE);

            auto* lightAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("LightShader");
            if (!lightAsset) {
                glDisable(GL_BLEND);
                return;
            }
            GLSLShader& lightShader = lightAsset->shaderObject;
            lightShader.Use();
            float camWidth = currentCamera.GetVisibleWorldWidth();
            float camHeight = currentCamera.GetVisibleWorldHeight();

            static GLint loc_worldSize = -1, loc_useShadows = -1, loc_shadowTexHeight = -1, loc_shadowTex = -1, loc_sceneOccluder = -1;
            static GLint loc_shadowDarkness = -1, loc_shadowMapStrength = -1, loc_raymarchShadowStrength = -1;
            static GLint loc_lightFlickerStrength = -1, loc_lightFlickerSpeed = -1, loc_time = -1;
            if (loc_worldSize == -1) {
                loc_worldSize = glGetUniformLocation(lightShader.GetHandle(), "uWorldSize");
                loc_useShadows = glGetUniformLocation(lightShader.GetHandle(), "uUseShadows");
                loc_shadowTexHeight = glGetUniformLocation(lightShader.GetHandle(), "uShadowTexHeight");
                loc_shadowTex = glGetUniformLocation(lightShader.GetHandle(), "uShadowTex");
                loc_sceneOccluder   = glGetUniformLocation(lightShader.GetHandle(), "uSceneOccluder");
                loc_shadowDarkness = glGetUniformLocation(lightShader.GetHandle(), "uShadowDarkness");
                loc_shadowMapStrength = glGetUniformLocation(lightShader.GetHandle(), "uShadowMapStrength");
                loc_raymarchShadowStrength = glGetUniformLocation(lightShader.GetHandle(), "uRaymarchShadowStrength");
                loc_lightFlickerStrength = glGetUniformLocation(lightShader.GetHandle(), "uLightFlickerStrength");
                loc_lightFlickerSpeed = glGetUniformLocation(lightShader.GetHandle(), "uLightFlickerSpeed");
                loc_time = glGetUniformLocation(lightShader.GetHandle(), "uTime");
            }

            glUniform2f(loc_worldSize, camWidth, camHeight);
            glUniform1i(loc_useShadows, shadowsEnabled ? 1 : 0);
            glUniform1f(loc_shadowTexHeight, (float)shadowTexHeight);
            glUniform1i(loc_shadowTex, 1);
            glUniform1i(loc_sceneOccluder, 5);
            if (loc_shadowDarkness != -1) glUniform1f(loc_shadowDarkness, g_shadowDarkness);
            if (loc_shadowMapStrength != -1) glUniform1f(loc_shadowMapStrength, g_shadowMapStrength);
            if (loc_raymarchShadowStrength != -1) glUniform1f(loc_raymarchShadowStrength, g_raymarchShadowStrength);
            if (loc_time != -1) glUniform1f(loc_time, g_timeSeconds);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowTex);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, sceneOccluderTexture);

            static std::vector<LightInstance> instances;
            instances.clear();
            instances.reserve(lightDraws.size());

            float flickerStrengthUniform = 0.0f;
            float flickerSpeedUniform = 0.0f;

            for (size_t i = 0; i < lightDraws.size(); ++i) {
                const auto& ld = lightDraws[i];
                if (!ld.active || ld.renderBehind != renderBehind) continue;

                LightInstance inst{};
                inst.posX = ld.uvCenter.x;
                inst.posY = ld.uvCenter.y;
                inst.radiusWorld = ld.radiusWorld;
                inst.colorR = ld.color.x;
                inst.colorG = ld.color.y;
                inst.colorB = ld.color.z;
                inst.intensity = ld.intensity;
                inst.affectBloom = ld.affectBloom ? 1.0f : 0.0f;
                inst.shadowTechnique = (float)ld.shadowTechnique;
                inst.raymarchSteps = (float)ld.raymarchSteps;
                inst.raymarchThreshold = ld.raymarchThreshold;
                inst.shadowIndex = (ld.shadowTechnique == 0 && shadowRowByLight[i] >= 0) ? (float)shadowRowByLight[i] : -1.0f;
                inst.flickerStrength = ld.flickerStrength;
                inst.flickerSpeed = ld.flickerSpeed;
                instances.push_back(inst);

                if (flickerStrengthUniform <= 0.0001f && ld.flickerEnabled) {
                    flickerStrengthUniform = ld.flickerStrength;
                    flickerSpeedUniform = ld.flickerSpeed;
                }
            }

            if (loc_lightFlickerStrength != -1) glUniform1f(loc_lightFlickerStrength, flickerStrengthUniform);
            if (loc_lightFlickerSpeed != -1) glUniform1f(loc_lightFlickerSpeed, flickerSpeedUniform);

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
                        lightDraws[i].ignoreOccluders = lc->ignoreOccluders;
                        lightDraws[i].uvCenter = uvCenter;
                        lightDraws[i].radiusUV = radiusUV;
                        lightDraws[i].worldPos = worldPos;
                        lightDraws[i].radiusWorld = lc->radius;
                        lightDraws[i].color = lc->color;
                        lightDraws[i].intensity = lc->intensity;
                        lightDraws[i].shadowTechnique = lc->shadowTechnique;
                        lightDraws[i].raymarchSteps = lc->raymarchSteps;
                        lightDraws[i].raymarchThreshold = lc->raymarchThreshold;
                        lightDraws[i].flickerEnabled = lc->flickerEnabled;
                        lightDraws[i].flickerStrength = lc->flickerEnabled ? lc->flickerStrength : 0.0f;
                        lightDraws[i].flickerSpeed = lc->flickerSpeed;

                        // Occluder rectangles are built on the main thread after this pass.
                    }
                });

            // --- Build shadow map rows ---
            static std::vector<int> shadowRowByLight;
            static std::vector<size_t> shadowLights;
            shadowRowByLight.assign(lightDraws.size(), -1);
            shadowLights.clear();
            shadowLights.reserve(lightDraws.size());

            if (g_shadowsEnabled) {
                for (size_t i = 0; i < lightDraws.size(); ++i) {
                    if (!lightDraws[i].active) continue;
                    if (lightDraws[i].shadowTechnique != 0) continue;
                    shadowRowByLight[i] = static_cast<int>(shadowLights.size());
                    shadowLights.push_back(i);
                }
            }

            // ==========================================
            // 1. SETUP & CPU SHADOWS
            // ==========================================
            int shadowRays = std::max(8, g_shadowRays);
            int shadowRows = static_cast<int>(shadowLights.size());
            if (shadowRows < 1) shadowRows = 1;

            static int maxShadowRows = 1;
            if (shadowRows > maxShadowRows) maxShadowRows = shadowRows;

            if (shadowTexWidth != shadowRays || shadowTexHeight < maxShadowRows) {
                shadowTexWidth = shadowRays;
                shadowTexHeight = maxShadowRows;
                glBindTexture(GL_TEXTURE_2D, shadowTex);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, shadowTexWidth, shadowTexHeight, 0, GL_RED, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            static std::vector<float> shadowData;
            if (shadowData.size() < static_cast<size_t>(shadowTexWidth * shadowTexHeight)) {
                shadowData.resize(static_cast<size_t>(shadowTexWidth) * static_cast<size_t>(shadowTexHeight));
            }
            std::fill(shadowData.begin(), shadowData.end(), 1.0f);

            if (g_shadowsEnabled && !shadowLights.empty()) {
                float camWidth = currentCamera.GetVisibleWorldWidth();
                float camHeight = currentCamera.GetVisibleWorldHeight();
                float maxScreenRadius = std::max(camWidth, camHeight) * 1.5f;

                unsigned int groundMask = 2;
                auto itGround = Component::TagComponent::tags.find("Ground");
                if (itGround != Component::TagComponent::tags.end()) groundMask = itGround->second;

                static std::vector<std::vector<CrashoutsEngine::Entity>> allLightCandidates;
                if (allLightCandidates.size() < shadowLights.size()) {
                    allLightCandidates.resize(shadowLights.size());
                }
                for (size_t i = 0; i < shadowLights.size(); ++i) allLightCandidates[i].clear();

                static std::vector<CrashoutsEngine::Entity> ignoredParents;
                ignoredParents.assign(shadowLights.size(), 0);

                for (size_t idx = 0; idx < shadowLights.size(); ++idx) {
                    size_t lightIndex = shadowLights[idx];
                    const LightDraw& ld = lightDraws[lightIndex];
                    Entity lightEnt = lightArray.indexToEntity[lightIndex];

                    if (ld.ignoreParentOccluders && ecs.HasComponent<Component::ParentComponent>(lightEnt)) {
                        auto* pc = ecs.GetComponent<Component::ParentComponent>(lightEnt);
                        if (pc && pc->parent != 0) ignoredParents[idx] = pc->parent;
                    }

                    float queryRadius = std::min(ld.radiusWorld, maxScreenRadius);
                    Collision::AABBRect lightRegion(
                        CEVec3(ld.worldPos.x - queryRadius, ld.worldPos.y - queryRadius, 0.f),
                        CEVec3(ld.worldPos.x + queryRadius, ld.worldPos.y + queryRadius, 0.f)
                    );

                    allLightCandidates[idx].reserve(64);
                    Collision::CollisionHandler::GetInstance().QueryTrees(lightRegion, allLightCandidates[idx]);

                    size_t w = 0;
                    for (size_t r = 0; r < allLightCandidates[idx].size(); ++r) {
                        Entity cand = allLightCandidates[idx][r];
                        auto* cc = ecs.GetComponent<Component::ColliderComponent>(cand);
                        if (!cc || !cc->collider) continue;
                        if (!cc->collider->enabled || cc->collider->isTrigger) continue;

                        if (ld.ignoreOccluders) {
                            auto* tag = ecs.GetComponent<Component::TagComponent>(cand);
                            if (!tag) continue;
                            if ((tag->tagMask & groundMask) == 0) continue;

                            auto* scriptComp = ecs.GetComponent<Component::ScriptComponent>(cand);
                            if (scriptComp) {
                                bool isWeightedPlatform = false;
                                for (const auto& s : scriptComp->Scripts) {
                                    if (s.ClassName == "WeightedPlatform") {
                                        isWeightedPlatform = true;
                                        break;
                                    }
                                }
                                if (isWeightedPlatform) continue;
                            }
                        }

                        auto* tc = ecs.GetComponent<Component::TransformComponent>(cand);
                        if (tc) {
                            cc->collider->UpdateCollider(tc->position);
                            if (cc->collider->type == Collision::ColliderType::OBB ||
                                cc->collider->type == Collision::ColliderType::POLYGON ||
                                cc->collider->type == Collision::ColliderType::TRIANGLE) {
                                cc->collider->UpdateRotation(tc->rotation);
                            }
                        }

                        allLightCandidates[idx][w++] = cand;
                    }
                    allLightCandidates[idx].resize(w);
                }

                static std::vector<CEVec3> precalcRayDirs;
                if (precalcRayDirs.size() != static_cast<size_t>(shadowRays)) {
                    precalcRayDirs.resize(static_cast<size_t>(shadowRays));
                }
                for (int r = 0; r < shadowRays; ++r) {
                    float angle = (static_cast<float>(r) + 0.5f) / static_cast<float>(shadowRays);
                    angle *= 6.28318530718f;
                    float ux = std::cos(angle);
                    float uy = std::sin(angle);

                    CEVec3 dir(ux * camWidth, uy * camHeight, 0.f);
                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                    if (len > 0.0001f) {
                        dir.x /= len;
                        dir.y /= len;
                    }
                    precalcRayDirs[static_cast<size_t>(r)] = dir;
                }

                int totalRays = static_cast<int>(shadowLights.size()) * shadowRays;
                if (totalRays > 0) {
                    CrashoutsEngine::JobSystem::Get().ParallelFor(totalRays,
                        [&](unsigned int start, unsigned int end) {
                            for (unsigned int taskIdx = start; taskIdx < end; ++taskIdx) {
                                int shadowArrIdx = static_cast<int>(taskIdx) / shadowRays;
                                int r = static_cast<int>(taskIdx) % shadowRays;

                                size_t lightIndex = shadowLights[static_cast<size_t>(shadowArrIdx)];
                                const LightDraw& ld = lightDraws[lightIndex];
                                Entity lightEnt = lightArray.indexToEntity[lightIndex];
                                int row = shadowRowByLight[lightIndex];
                                if (row < 0) continue;

                                const auto& localCandidates = allLightCandidates[static_cast<size_t>(shadowArrIdx)];
                                CrashoutsEngine::Entity ignoredParent = ignoredParents[static_cast<size_t>(shadowArrIdx)];

                                CEVec3 dir = precalcRayDirs[static_cast<size_t>(r)];
                                CEVec3 startPos = ld.worldPos;
                                CEVec3 endPos = startPos + dir * ld.radiusWorld;
                                CEVec3 hitPoint{};
                                CrashoutsEngine::Entity hitEnt = 0;

                                bool hit = Collision::CollisionHandler::GetInstance().RaycastAgainstList(
                                    startPos, endPos, localCandidates, hitPoint, hitEnt, lightEnt, ignoredParent
                                );

                                float dist = ld.radiusWorld;
                                if (hit) {
                                    CEVec3 delta = hitPoint - startPos;
                                    dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                                    if (dist > ld.radiusWorld) dist = ld.radiusWorld;
                                }

                                float norm = (ld.radiusWorld > 0.0001f) ? (dist / ld.radiusWorld) : 1.0f;
                                shadowData[static_cast<size_t>(row) * static_cast<size_t>(shadowTexWidth) + static_cast<size_t>(r)] = norm;
                            }
                        });
                }

                glBindTexture(GL_TEXTURE_2D, shadowTex);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shadowTexWidth, shadowTexHeight, GL_RED, GL_FLOAT, shadowData.data());
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            bool needRaymarch = false;
            if (g_shadowsEnabled) {
                for (size_t i = 0; i < lightDraws.size(); ++i) {
                    if (!lightDraws[i].active) continue;
                    if (lightDraws[i].shadowTechnique == 1) {
                        needRaymarch = true;
                        break;
                    }
                }
            }

            if (needRaymarch) {
                glBindFramebuffer(GL_FRAMEBUFFER, sceneOccluderFbo);
                glViewport(0, 0, gameTextureWidth, gameTextureHeight);
                glDisable(GL_DEPTH_TEST);
                glClearColor(0.f, 0.f, 0.f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT);

                auto* occAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("OccluderMaskShader");
                if (occAsset) {
                    struct OccVertex { float x, y, u, v; };
                    GLSLShader& occShader = occAsset->shaderObject;
                    occShader.Use();
                    glEnable(GL_BLEND);
                    glBlendEquation(GL_MAX);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glDisable(GL_CULL_FACE);

                    glUniform1i(glGetUniformLocation(occShader.GetHandle(), "uUseTexture"), 0);
                    glUniform1f(glGetUniformLocation(occShader.GetHandle(), "uAlpha"), 1.0f);

                    unsigned int groundMask = 2;
                    auto it = Component::TagComponent::tags.find("Ground");
                    if (it != Component::TagComponent::tags.end()) groundMask = it->second;

                    glBindVertexArray(occluderVao);
                    glBindBuffer(GL_ARRAY_BUFFER, occluderVbo);

                    auto& colArray = ecs.GetComponentArray<Component::ColliderComponent>();
                    for (size_t ci = 0; ci < colArray.dense.size(); ++ci) {
                        auto* cc = colArray.dense[ci];
                        if (!cc || !cc->collider) continue;

                        Entity ent = colArray.indexToEntity[ci];
                        if (ecs.HasComponent<ActiveComponent>(ent) && !ecs.GetComponent<ActiveComponent>(ent)->isActive) continue;

                        auto* tag = ecs.GetComponent<Component::TagComponent>(ent);
                        if (!tag) continue;
                        if ((tag->tagMask & groundMask) == 0) continue;

                        auto* scriptComp = ecs.GetComponent<Component::ScriptComponent>(ent);
                        if (scriptComp) {
                            for (const auto& s : scriptComp->Scripts) {
                                if (s.ClassName == "WeightedPlatform") {
                                    goto SkipColliderOcc;
                                }
                            }
                        }

                        if (!cc->collider->enabled || cc->collider->isTrigger) goto SkipColliderOcc;

                        {
                            CEVec3 minW{}, maxW{};
                            switch (cc->collider->ColliderType) {
                            case Collision::ColliderType::AABB: {
                                auto* a = static_cast<Collision::AABBRect*>(cc->collider.get());
                                minW = a->min;
                                maxW = a->max;
                                break;
                            }
                            case Collision::ColliderType::CIRCLE: {
                                auto* c = static_cast<Collision::Circle*>(cc->collider.get());
                                minW = { c->position.x - c->radius, c->position.y - c->radius, 0.f };
                                maxW = { c->position.x + c->radius, c->position.y + c->radius, 0.f };
                                break;
                            }
                            case Collision::ColliderType::OBB:
                            case Collision::ColliderType::POLYGON:
                            case Collision::ColliderType::TRIANGLE: {
                                auto* p = static_cast<Collision::Polygon*>(cc->collider.get());
                                CEVec3 extents = Collision::Polygon::CalculateOBBExtents(*p);
                                minW = p->position - extents;
                                maxW = p->position + extents;
                                break;
                            }
                            default:
                                goto SkipColliderOcc;
                            }

                            glm::vec2 world[4] = {
                                { minW.x, minW.y },
                                { maxW.x, minW.y },
                                { maxW.x, maxW.y },
                                { minW.x, maxW.y }
                            };

                            glm::vec2 ndc[4];
                            bool ok = true;
                            for (int k = 0; k < 4; ++k) {
                                glm::vec4 clipC = proj * view * glm::vec4(world[k].x, world[k].y, 0.0f, 1.0f);
                                if (clipC.w == 0.0f) { ok = false; break; }
                                glm::vec3 ndcC = glm::vec3(clipC) / clipC.w;
                                ndc[k] = { ndcC.x, ndcC.y };
                            }
                            if (!ok) goto SkipColliderOcc;

                            OccVertex verts[6] = {
                                { ndc[0].x, ndc[0].y, 0.0f, 1.0f },
                                { ndc[1].x, ndc[1].y, 1.0f, 1.0f },
                                { ndc[2].x, ndc[2].y, 1.0f, 0.0f },
                                { ndc[0].x, ndc[0].y, 0.0f, 1.0f },
                                { ndc[2].x, ndc[2].y, 1.0f, 0.0f },
                                { ndc[3].x, ndc[3].y, 0.0f, 0.0f }
                            };

                            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
                            glDrawArrays(GL_TRIANGLES, 0, 6);
                            Component::Rendering::Get().drawCalls++;
                        }

                    SkipColliderOcc:
                        continue;
                    }

                    glBindVertexArray(0);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    glBlendEquation(GL_FUNC_ADD);
                    glDisable(GL_BLEND);
                    glEnable(GL_CULL_FACE);
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
            auto* occAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("OccluderMaskShader");
            if (occAsset) {
                struct OccVertex { float x, y, u, v; };
                GLSLShader& occShader = occAsset->shaderObject;
                occShader.Use();

                static GLint loc_useTex = -1, loc_alpha = -1, loc_tex2d = -1;
                if (loc_useTex == -1) {
                    loc_useTex = glGetUniformLocation(occShader.GetHandle(), "uUseTexture");
                    loc_alpha = glGetUniformLocation(occShader.GetHandle(), "uAlpha");
                    loc_tex2d = glGetUniformLocation(occShader.GetHandle(), "uTex2d");
                }

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
                    if (ecs.HasComponent<ActiveComponent>(ent) && !ecs.GetComponent<ActiveComponent>(ent)->isActive) continue;

                    Entity occEnt = ent;
                    if (!lc->ignoreParentOccluders && ecs.HasComponent<Component::ParentComponent>(ent)) {
                        Entity p = ent;
                        while (ecs.HasComponent<Component::ParentComponent>(p)) {
                            auto* pc = ecs.GetComponent<Component::ParentComponent>(p);
                            p = (pc && pc->parent != 0) ? pc->parent : 0;
                            if (p == 0) break;
                            if (ecs.HasComponent<TransformComponent>(p) && ecs.HasComponent<ShapeComponent>(p)) {
                                occEnt = p;
                                break;
                            }
                        }
                    }

                    auto* tc = ecs.GetComponent<TransformComponent>(occEnt);
                    auto* sc = ecs.GetComponent<ShapeComponent>(occEnt);
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
                    CEVec3 worldPos = tc->position;

                    glm::vec2 local[4] = {
                        { -half.x, -half.y },
                        {  half.x, -half.y },
                        {  half.x,  half.y },
                        { -half.x,  half.y }
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
                        { ndc[0].x, ndc[0].y, u0, v1 },
                        { ndc[1].x, ndc[1].y, u1, v1 },
                        { ndc[2].x, ndc[2].y, u1, v0 },
                        { ndc[0].x, ndc[0].y, u0, v1 },
                        { ndc[2].x, ndc[2].y, u1, v0 },
                        { ndc[3].x, ndc[3].y, u0, v0 }
                    };

                    glUniform1i(loc_useTex, sc->enableTexture ? 1 : 0);
                    glUniform1f(loc_alpha, sc->color.w());
                    if (sc->enableTexture) {
                        glUniform1i(loc_tex2d, 0);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, texHandle);
                    }

                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    Component::Rendering::Get().drawCalls++;
                }

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBlendEquation(GL_FUNC_ADD);
                glDisable(GL_BLEND);
                glEnable(GL_CULL_FACE);
            }

            // ==========================================
            // 4. GPU COMBINE PASS
            // ==========================================
            {
                FrameProfiler::Scoped sComb(PerformanceSystem::Get().GetProfiler(), "PP_GPU_Combine");
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

                static GLint loc_cAmbient = -1, loc_cShadowDarkness = -1, loc_cShadowColor = -1, loc_cShadowColorStrength = -1;
                static GLint loc_cLightBehind = -1, loc_cOccluder = -1, loc_cScene = -1, loc_cLight = -1;
                if (loc_cAmbient == -1) {
                    loc_cAmbient = glGetUniformLocation(combineShader.GetHandle(), "uAmbient");
                    loc_cShadowDarkness = glGetUniformLocation(combineShader.GetHandle(), "uShadowDarkness");
                    loc_cShadowColor = glGetUniformLocation(combineShader.GetHandle(), "uShadowColor");
                    loc_cShadowColorStrength = glGetUniformLocation(combineShader.GetHandle(), "uShadowColorStrength");
                    loc_cLightBehind = glGetUniformLocation(combineShader.GetHandle(), "uLightBehind");
                    loc_cOccluder = glGetUniformLocation(combineShader.GetHandle(), "uOccluderMask");
                    loc_cScene = glGetUniformLocation(combineShader.GetHandle(), "uScene");
                    loc_cLight = glGetUniformLocation(combineShader.GetHandle(), "uLight");
                }

                glUniform1f(loc_cAmbient, g_ambient);
                if (loc_cShadowDarkness != -1) glUniform1f(loc_cShadowDarkness, g_shadowDarkness);
                if (loc_cShadowColor != -1) glUniform3f(loc_cShadowColor, g_shadowColor.x, g_shadowColor.y, g_shadowColor.z);
                if (loc_cShadowColorStrength != -1) glUniform1f(loc_cShadowColorStrength, g_shadowColorStrength);
                glUniform1i(loc_cLightBehind, 2);
                glUniform1i(loc_cOccluder, 3);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, behindLightsTex[0]); // Bloom Behind
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, occluderTexture);

                RenderFullscreenCached(combineShader, sceneTextureID, loc_cScene, frontLightsTex[0], loc_cLight); // Bloom Front
                glBindTexture(GL_TEXTURE_2D, 0);
            } // END GPU COMBINE

            // ==========================================
            // 5. GPU BLOOM
            // ==========================================
            GLuint bloomTex = brightTexture; // Declared outside so Composite can use it
            {
                FrameProfiler::Scoped sBloom(PerformanceSystem::Get().GetProfiler(), "PP_GPU_Bloom");

                int blurW = std::max(1, gameTextureWidth / 4);
                int blurH = std::max(1, gameTextureHeight / 4);
                glBindFramebuffer(GL_FRAMEBUFFER, brightFbo);
                glViewport(0, 0, blurW, blurH);
                glClear(GL_COLOR_BUFFER_BIT);

                auto* brightAsset = Assets::AssetManager::GetInstance().Get<Assets::ShaderAsset>("BrightShader");
                if (!brightAsset) {
                    RenderFallback(combineTexture);
                    return;
                }
                GLSLShader& brightShader = brightAsset->shaderObject;
                brightShader.Use();

                static GLint loc_bThreshold = -1, loc_bScene = -1;
                if (loc_bThreshold == -1) {
                    loc_bThreshold = glGetUniformLocation(brightShader.GetHandle(), "uThreshold");
                    loc_bScene = glGetUniformLocation(brightShader.GetHandle(), "uScene");
                }
                glUniform1f(loc_bThreshold, g_bloomThreshold);
                RenderFullscreenCached(brightShader, combineTexture, loc_bScene);

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

                    static GLint loc_blurTexel = -1, loc_blurPass = -1, loc_blurImage = -1;
                    if (loc_blurTexel == -1) {
                        loc_blurTexel = glGetUniformLocation(blurShader.GetHandle(), "uTexelSize");
                        loc_blurPass = glGetUniformLocation(blurShader.GetHandle(), "uPass");
                        loc_blurImage = glGetUniformLocation(blurShader.GetHandle(), "uImage");
                    }

                    glUniform2f(loc_blurTexel, 1.0f / blurW, 1.0f / blurH);

                    for (int i = 0; i < iterations; ++i) {
                        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFbo[i % 2]);
                        glViewport(0, 0, blurW, blurH);
                        glClear(GL_COLOR_BUFFER_BIT);

                        glUniform1i(loc_blurPass, i);
                        RenderFullscreenCached(blurShader, blurInput, loc_blurImage);
                        blurInput = pingpongTex[i % 2];
                    }
                }

                bloomTex = (iterations > 0) ? blurInput : brightTexture;
            } // END GPU BLOOM

            // ==========================================
            // 6. GPU FINAL COMPOSITE
            // ==========================================
            {
                FrameProfiler::Scoped sComp(PerformanceSystem::Get().GetProfiler(), "PP_GPU_Composite");
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

                static GLint loc_fUseBloom = -1, loc_fBloomInt = -1, loc_fActive = -1, loc_fBright = -1;
                static GLint loc_fContrast = -1, loc_fSat = -1, loc_fNoBloomL = -1, loc_fNoBloomB = -1, loc_fOcc = -1;
                static GLint loc_fScene = -1, loc_fBloom = -1;
                static GLint loc_fShadowDarkness = -1, loc_fShadowColor = -1, loc_fShadowColorStrength = -1;

                if (loc_fUseBloom == -1) {
                    loc_fUseBloom = glGetUniformLocation(finalShader.GetHandle(), "uUseBloom");
                    loc_fBloomInt = glGetUniformLocation(finalShader.GetHandle(), "uBloomIntensity");
                    loc_fActive = glGetUniformLocation(finalShader.GetHandle(), "uActive");
                    loc_fBright = glGetUniformLocation(finalShader.GetHandle(), "uBrightness");
                    loc_fContrast = glGetUniformLocation(finalShader.GetHandle(), "uContrast");
                    loc_fSat = glGetUniformLocation(finalShader.GetHandle(), "uSaturation");
                    loc_fNoBloomL = glGetUniformLocation(finalShader.GetHandle(), "uNoBloomLight");
                    loc_fNoBloomB = glGetUniformLocation(finalShader.GetHandle(), "uNoBloomBehind");
                    loc_fOcc = glGetUniformLocation(finalShader.GetHandle(), "uOccluderMask");
                    loc_fScene = glGetUniformLocation(finalShader.GetHandle(), "uScene");
                    loc_fBloom = glGetUniformLocation(finalShader.GetHandle(), "uBloom");
                    loc_fShadowDarkness = glGetUniformLocation(finalShader.GetHandle(), "uShadowDarkness");
                    loc_fShadowColor = glGetUniformLocation(finalShader.GetHandle(), "uShadowColor");
                    loc_fShadowColorStrength = glGetUniformLocation(finalShader.GetHandle(), "uShadowColorStrength");
                }

                glUniform1i(loc_fUseBloom, g_bloomEnabled ? 1 : 0);
                glUniform1f(loc_fBloomInt, g_bloomIntensity);
                glUniform1i(loc_fActive, active ? 1 : 0);
                glUniform1f(loc_fBright, g_brightness);
                glUniform1f(loc_fContrast, g_contrast + 1.f);
                glUniform1f(loc_fSat, g_saturation + 1.f);
                if (loc_fShadowDarkness != -1) glUniform1f(loc_fShadowDarkness, g_shadowDarkness);
                if (loc_fShadowColor != -1) glUniform3f(loc_fShadowColor, g_shadowColor.x, g_shadowColor.y, g_shadowColor.z);
                if (loc_fShadowColorStrength != -1) glUniform1f(loc_fShadowColorStrength, g_shadowColorStrength);
                glUniform1i(loc_fNoBloomL, 2);
                glUniform1i(loc_fNoBloomB, 3);
                glUniform1i(loc_fOcc, 4);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, frontLightsTex[1]); // NoBloom Front
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, behindLightsTex[1]); // NoBloom Behind
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, occluderTexture);

                RenderFullscreenCached(finalShader, combineTexture, loc_fScene, bloomTex, loc_fBloom);
                glBindTexture(GL_TEXTURE_2D, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            } // END GPU COMPOSITE
        }
    }
}
