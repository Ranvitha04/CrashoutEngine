#version 460 core

in vec4 color;
in vec2 texCoord;
in vec2 vWorldPos;
in float vWave;
out vec4 fragColor;

uniform sampler2D uTex2d;
uniform int  u_useTexture;
uniform vec4 u_tint;
uniform float u_time;
uniform float u_flowStrength;
uniform float u_flowSpeed;
uniform float u_flowFrequency;

void main() {
    float flow = max(0.0, u_flowStrength);
    float n = 0.0;
    if (flow > 0.0001) {
        float f = u_flowFrequency;
        float sp = u_flowSpeed;
        n = sin(vWorldPos.x * f + u_time * sp) * cos(vWorldPos.y * (f * 0.8) - u_time * (sp * 1.3));
    }

    vec4 result;
    if (u_useTexture == 1) {
        vec2 uv = texCoord;
        if (flow > 0.0001) {
            uv += vec2(n, -n) * (0.02 * flow);
        }
        vec4 tex = texture(uTex2d, uv);
        result = tex * color * u_tint;
    }
    else {
        result = color * u_tint;
    }
    if (flow > 0.0001) {
        result.rgb *= (1.0 + 0.18 * flow * n);
    }
    result.rgb *= (1.0 + (vWave * 0.35 * flow));
    if (result.a < 0.01) discard;
    fragColor = result;
}
