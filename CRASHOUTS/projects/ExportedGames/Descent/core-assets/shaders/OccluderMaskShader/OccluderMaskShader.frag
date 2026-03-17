/*!
@file       OccluderMaskShader.frag
@author     Renfred (RTIS)
@author     Ranvitha
@date       13/03/2026
@brief      Writes a grayscale occluder mask value based on alpha and optional texture sampling.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uTex2d;
uniform int uUseTexture;
uniform float uAlpha;

/*!
@brief Fragment shader entry point.
*/

void main() {
    float a = uAlpha;
    if (uUseTexture == 1) {
        a *= texture(uTex2d, texCoord).a;
    }
    fragColor = vec4(a, a, a, 1.0);
}
