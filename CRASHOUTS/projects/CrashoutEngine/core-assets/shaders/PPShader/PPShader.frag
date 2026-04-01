/*!
@file       PPShader.frag
@author     Renfred Lee
@date       13/03/2026
@brief      Post-processing fragment shader providing optional color grading (brightness/contrast/saturation).

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

out vec4 fragColor;

in vec2 texCoord;                   // From vertex shader

uniform sampler2D u_sceneTexture;   // Exported scene as a texture

// Color grading controls
uniform float u_brightness = 0.0;   // 0.0 = no change
uniform float u_contrast = 0.0;     // 0.0 = no change
uniform float u_saturation = 0.0;   // 0.0 = no change
uniform bool  u_active = false;   

uniform bool u_vignetteEnabled = false;
uniform float u_vignetteIntensity = 0.6;
uniform float u_vignetteRadius = 0.75;
uniform float u_vignetteSoftness = 0.35;
uniform vec3 u_vignetteColor = vec3(0.0);
uniform float u_aspect = 1.7777778;
uniform vec2 u_vignetteCenter = vec2(0.5, 0.5);

/*!
@brief Fragment shader entry point.
*/

void main() {
    // Get the pixel color from game scene
    vec3 color = texture(u_sceneTexture, texCoord).rgb;

    if(u_active){
        // Apply Brightness
        color += u_brightness;

        // Apply Contrast
        // Moves color away from (or towards) middle-gray (0.5)
        color = (color - 0.5) * u_contrast + 0.5;

        // Apply Saturation
        // Converts to grayscale then blends back to original based on saturation uniform
        vec3 grayscale = vec3(dot(color, vec3(0.299, 0.587, 0.114)));
        color = mix(grayscale, color, u_saturation);

        // Final output, Clamp to make sure colors are valid 0.0 - 1.0
        color = clamp(color, 0.0, 1.0);
    }

    if (u_vignetteEnabled) {
        vec2 centered = texCoord - u_vignetteCenter;
        centered.x *= max(u_aspect, 0.0001);
        float maxDist = 0.5 * sqrt(u_aspect * u_aspect + 1.0);
        float dist = length(centered) / max(maxDist, 0.0001);
        float softness = max(u_vignetteSoftness, 0.0001);
        float radius = max(u_vignetteRadius, 0.0);
        float vig = smoothstep(radius, radius + softness, dist);
        float amount = clamp(vig * max(u_vignetteIntensity, 0.0), 0.0, 1.0);
        color = mix(color, u_vignetteColor, amount);
    }
    
    // Set the final pixel color
    fragColor = vec4(color, 1.0);

}
