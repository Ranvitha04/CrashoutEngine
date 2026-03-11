#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uImage;
uniform vec2 uTexelSize; // 1.0 / textureSize(uImage, 0)
uniform int uPass;       // The current blur pass (0, 1, 2, ...)

void main() {
    // The offset is increased for each pass
    float offset = float(uPass) + 0.5;
    
    vec2 halfTexel = uTexelSize * 0.5;

    // Sample in a diamond pattern
    vec4 sum = texture(uImage, texCoord) * 4.0;
    sum += texture(uImage, texCoord + vec2( offset * uTexelSize.x,  halfTexel.y));
    sum += texture(uImage, texCoord + vec2(-offset * uTexelSize.x, -halfTexel.y));
    sum += texture(uImage, texCoord + vec2( halfTexel.x,  offset * uTexelSize.y));
    sum += texture(uImage, texCoord + vec2(-halfTexel.x, -offset * uTexelSize.y));

    fragColor = sum / 8.0;
}
