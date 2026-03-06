#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uScene;
uniform sampler2D uLight;
uniform sampler2D uLightBehind;
uniform sampler2D uOccluderMask;
uniform float uAmbient;

void main() {
    vec4 scene = texture(uScene, texCoord);
    vec3 light = texture(uLight, texCoord).rgb;
    vec3 behind = texture(uLightBehind, texCoord).rgb;
    float mask = texture(uOccluderMask, texCoord).r;
    vec3 color = scene.rgb * uAmbient + light + behind * (1.0 - mask);
    fragColor = vec4(color, scene.a);
}
