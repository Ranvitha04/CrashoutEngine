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

    // --- Add front lights ---
    vec3 lightColor = texture(uNoBloomLight, texCoord).rgb;

    // --- Add behind lights with mask ---
    float mask = texture(uOccluderMask, texCoord).r;
    if (mask < 0.999)
    {
        vec3 behind = texture(uNoBloomBehind, texCoord).rgb;
        lightColor += behind * (1.0 - mask);
    }

    // --- Combine all ---
    vec3 color = sceneColor + bloomColor + lightColor;

    // --- Post-processing ---
    if (uActive == 1)
    {
        color += uBrightness;
        color = (color - 0.5) * uContrast + 0.5;

        float g = dot(color, vec3(0.299, 0.587, 0.114));
        color = mix(vec3(g), color, uSaturation);

        color = clamp(color, 0.0, 1.0);
    }

    fragColor = vec4(color, 1.0);
}