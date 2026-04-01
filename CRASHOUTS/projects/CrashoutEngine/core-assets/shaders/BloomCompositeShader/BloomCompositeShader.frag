/*!
@file       BloomCompositeShader.frag
@author     Ranvitha
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Combines scene, bloom, and lighting buffers with optional post-processing.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uScene;
uniform sampler2D uBloom;         // blurred bloom texture
uniform sampler2D uNoBloomLight;  // front lights without bloom
uniform sampler2D uNoBloomBehind; // back lights without bloom
uniform sampler2D uOccluderMask;  // occlusion mask

uniform int uUseBloom;
uniform float uBloomIntensity;

uniform float uThreshold;     // bloom threshold
uniform int uActive;          // enable post-processing
uniform float uBrightness;
uniform float uContrast;
uniform float uSaturation;
uniform float uShadowDarkness;
uniform vec3 uShadowColor;
uniform float uShadowColorStrength;

/*!
@brief Fragment shader entry point.
*/

void main()
{
    vec3 sceneColor = texture(uScene, texCoord).rgb;

    // --- Bloom threshold ---
    float maxChannel = max(sceneColor.r, max(sceneColor.g, sceneColor.b));
    vec3 bloomColor = vec3(0.0);
    if (uUseBloom == 1 && maxChannel > uThreshold)
    {
        bloomColor = texture(uBloom, texCoord).rgb * uBloomIntensity;
    }

    vec4 lightFront = texture(uNoBloomLight, texCoord);
    vec4 lightBehind = texture(uNoBloomBehind, texCoord);
    vec3 lightColor = lightFront.rgb;

    // --- Add behind lights with mask ---
    float mask = texture(uOccluderMask, texCoord).r;
    if (mask < 0.999)
    {
        lightColor += lightBehind.rgb * (1.0 - mask);
    }

    // --- Combine all ---
    vec3 color = sceneColor + bloomColor + lightColor;

    float shadowMask = lightFront.a;
    if (mask < 0.999) shadowMask = max(shadowMask, lightBehind.a * (1.0 - mask));
    shadowMask = clamp(shadowMask, 0.0, 1.0);

    float shadowAmount = clamp(shadowMask * max(uShadowDarkness, 0.0), 0.0, 1.0);
    vec3 shadowMul = mix(vec3(1.0), clamp(uShadowColor, 0.0, 1.0), clamp(uShadowColorStrength, 0.0, 1.0));
    color *= mix(vec3(1.0), shadowMul, shadowAmount);

    // --- Post-processing ---
    if (uActive == 1)
    {
        color += uBrightness;
        color = (color - 0.5) * uContrast + 0.5;
        vec3 grayscale = vec3(dot(color, vec3(0.299, 0.587, 0.114)));
        color = mix(grayscale, color, uSaturation);
        color = clamp(color, 0.0, 1.0);
    }

    fragColor = vec4(color, 1.0);
}
