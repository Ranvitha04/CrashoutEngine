#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uTex2d;
uniform int uUseTexture;
uniform float uAlpha;

void main() {
    float a = uAlpha;
    if (uUseTexture == 1) {
        a *= texture(uTex2d, texCoord).a;
    }
    fragColor = vec4(a, a, a, 1.0);
}



