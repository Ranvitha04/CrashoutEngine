#version 460 core

out vec4 fragColor;

in vec2 texCoord;

in vec2 vLightPos;
in float vLightRadius;
in vec3 vLightColor;
in float vIntensity;
in float vShadowIndex;

uniform vec2 uWorldSize;
uniform sampler2D uShadowTex;
uniform float uShadowTexHeight;
uniform int uUseShadows;

void main() {
    vec2 d = texCoord - vLightPos;
    vec2 dWorld = vec2(d.x * uWorldSize.x, d.y * uWorldSize.y);
    float dist = length(dWorld);
    float radius = vLightRadius;
    float falloff = smoothstep(radius, 0.0, dist);

    if (uUseShadows == 1 && vShadowIndex >= 0.0 && radius > 0.0001) {
        float angle = atan(d.y, d.x);
        angle = (angle < 0.0) ? (angle + 6.28318530718) : angle;
        float u = angle / 6.28318530718;
        float v = (vShadowIndex + 0.5) / max(uShadowTexHeight, 1.0);
        float shadowNorm = texture(uShadowTex, vec2(u, v)).r;
        float distNorm = dist / radius;
        if (distNorm > shadowNorm) {
            falloff = 0.0;
        }
    }
    vec3 color = vLightColor * vIntensity * falloff;
    fragColor = vec4(color, 1.0);
}

