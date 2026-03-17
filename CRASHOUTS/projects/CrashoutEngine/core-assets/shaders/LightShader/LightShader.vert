/*!
@file       LightShader.vert
@author     Ranvitha
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Fullscreen vertex shader that passes per-light instance attributes to the fragment stage.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 iLightPos;
layout (location = 3) in float iLightRadius;
layout (location = 4) in vec3 iLightColor;
layout (location = 5) in float iIntensity;
layout (location = 6) in float iShadowIndex;
layout (location = 7) in float iAffectBloom;
layout (location = 8) in float iShadowTechnique;
layout (location = 9) in float iRaymarchSteps;
layout (location = 10) in float iRaymarchThreshold;

out vec2 texCoord;
out vec2 vLightPos;
out float vLightRadius;
out vec3 vLightColor;
out float vIntensity;
out float vShadowIndex;
out float vAffectBloom;
out float vShadowTechnique;
out float vRaymarchSteps;
out float vRaymarchThreshold;

/*!
@brief Vertex shader entry point.
*/

void main()
{
    texCoord = aTexCoord;

    vLightPos = iLightPos;
    vLightRadius = iLightRadius;
    vLightColor = iLightColor;
    vIntensity = iIntensity;
    vShadowIndex = iShadowIndex;
    vAffectBloom = iAffectBloom;
    vShadowTechnique = iShadowTechnique;
    vRaymarchSteps = iRaymarchSteps;
    vRaymarchThreshold = iRaymarchThreshold;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
