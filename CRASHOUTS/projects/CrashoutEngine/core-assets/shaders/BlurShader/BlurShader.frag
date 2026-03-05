#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uImage;
uniform int uHorizontal;

const float weights[5] = float[](
    0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
);

void main() {
    vec2 texOffset = 1.0 / vec2(textureSize(uImage, 0));
    vec3 result = texture(uImage, texCoord).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        if (uHorizontal == 1) {
            result += texture(uImage, texCoord + vec2(texOffset.x * i, 0.0)).rgb * weights[i];
            result += texture(uImage, texCoord - vec2(texOffset.x * i, 0.0)).rgb * weights[i];
        } else {
            result += texture(uImage, texCoord + vec2(0.0, texOffset.y * i)).rgb * weights[i];
            result += texture(uImage, texCoord - vec2(0.0, texOffset.y * i)).rgb * weights[i];
        }
    }
    fragColor = vec4(result, 1.0);
}
