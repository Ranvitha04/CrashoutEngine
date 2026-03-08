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

void main() {
    vec4 scene = texture(uScene, texCoord);
    vec3 color = scene.rgb;
    if (uUseBloom == 1) {
        vec3 bloom = texture(uBloom, texCoord).rgb;
        color += bloom * uBloomIntensity;
    }

    vec3 noBloomFront = texture(uNoBloomLight, texCoord).rgb;
    float mask = texture(uOccluderMask, texCoord).r;
    vec3 noBloomBehind = texture(uNoBloomBehind, texCoord).rgb * (1.0 - mask);
    color += noBloomFront + noBloomBehind;

    if (uActive == 1) {
        color += uBrightness;
        color = (color - 0.5) * uContrast + 0.5;
        vec3 grayscale = vec3(dot(color, vec3(0.299, 0.587, 0.114)));
        color = mix(grayscale, color, uSaturation);
        color = clamp(color, 0.0, 1.0);
    }

    fragColor = vec4(color, 1.0);
}
