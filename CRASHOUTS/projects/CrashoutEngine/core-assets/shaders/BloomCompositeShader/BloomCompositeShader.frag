#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform sampler2D uNoBloomLight;
uniform sampler2D uNoBloomBehind;
uniform sampler2D uOccluderMask;

uniform int uUseBloom;
uniform float uBloomIntensity;

uniform int uActive;
uniform float uBrightness;
uniform float uContrast;
uniform float uSaturation;

void main()
{
    vec3 color = texture(uScene, texCoord).rgb;

    if (uUseBloom == 1)
    {
        vec3 bloom = texture(uBloom, texCoord).rgb;
        color += bloom * uBloomIntensity;
    }

    color += texture(uNoBloomLight, texCoord).rgb;

    float mask = texture(uOccluderMask, texCoord).r;

    if (mask < 0.999)
    {
        vec3 behind = texture(uNoBloomBehind, texCoord).rgb;
        color += behind * (1.0 - mask);
    }

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