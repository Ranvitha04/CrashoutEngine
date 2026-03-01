#version 460 core

out vec4 fragColor;

in vec2 texCoord;

in vec2 vLightPos;
in float vLightRadius;
in vec3 vLightColor;
in float vIntensity;

void main() {
    float dist = distance(texCoord, vLightPos);
    float falloff = smoothstep(vLightRadius, 0.0, dist);
    vec3 color = vLightColor * vIntensity * falloff;
    fragColor = vec4(color, 1.0);
}
