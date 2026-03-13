/*!
@file       BrightShader.frag
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Extracts bright fragments from the scene for bloom processing.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uScene;
uniform float uThreshold;

/*!
@brief Fragment shader entry point.
*/

void main() {
    vec3 color = texture(uScene, texCoord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > uThreshold) {
        fragColor = vec4(color, 1.0);
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
