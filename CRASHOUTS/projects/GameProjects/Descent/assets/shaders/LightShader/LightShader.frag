#version 460 core

out vec4 fragColor;

in vec2 texCoord;

in vec2 vLightPos;
in float vLightRadius;
in vec3 vLightColor;
in float vIntensity;
uniform float uAspect;

void main() {
    vec2 d = texCoord - vLightPos;
    d.x *= uAspect;
    float dist = length(d);
    float radius = vLightRadius * uAspect;
    float falloff = smoothstep(radius, 0.0, dist);
    vec3 color = vLightColor * vIntensity * falloff;
    fragColor = vec4(color, 1.0);
}

