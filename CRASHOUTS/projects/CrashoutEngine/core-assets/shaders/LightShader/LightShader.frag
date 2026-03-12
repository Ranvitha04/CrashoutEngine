#version 460 core

layout (location = 0) out vec4 fragBloom;
layout (location = 1) out vec4 fragNoBloom;

in vec2 texCoord;

in vec2 vLightPos;
in float vLightRadius;
in vec3 vLightColor;
in float vIntensity;
in float vShadowIndex;
in float vAffectBloom;
in float vShadowTechnique;
in float vRaymarchSteps;
in float vRaymarchThreshold;

uniform vec2 uWorldSize;
uniform sampler2D uShadowTex;
uniform sampler2D uSceneOccluder;
uniform float uShadowTexHeight;
uniform int uUseShadows;

void main() {
    vec2 d = texCoord - vLightPos;
    vec2 dWorld = vec2(d.x * uWorldSize.x, d.y * uWorldSize.y);
    float dist = length(dWorld);
    float radius = vLightRadius;

    float falloff = smoothstep(radius, 0.0, dist);

    if (uUseShadows == 1 && radius > 0.0001 && falloff > 0.0001) {
        if (vShadowTechnique > 0.5) {
            int steps = int(clamp(vRaymarchSteps, 4.0, 128.0));
            float tMax = clamp(dist / radius, 0.0, 1.0);
            vec2 stepDir = d / float(steps);
            bool occluded = false;
            float threshold = clamp(vRaymarchThreshold, 0.01, 1.0);
            for (int i = 1; i <= steps; ++i) {
                float t = float(i) / float(steps);
                if (t > tMax) break;
                vec2 uv = vLightPos + stepDir * float(i);
                if (texture(uSceneOccluder, uv).r > threshold) {
                    occluded = true;
                    break;
                }
            }
            if (occluded) falloff = 0.0;
        }
        else if (vShadowIndex >= 0.0) {
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
    }
    vec3 color = vLightColor * vIntensity * falloff;
    
    if (vAffectBloom > 0.5) {
        fragBloom = vec4(color, 1.0);
        fragNoBloom = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        fragBloom = vec4(0.0, 0.0, 0.0, 1.0);
        fragNoBloom = vec4(color, 1.0);
    }
}

