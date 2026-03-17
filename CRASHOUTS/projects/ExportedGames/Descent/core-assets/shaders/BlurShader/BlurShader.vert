/*!
@file       BlurShader.vert
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Fullscreen vertex shader for separable blur passes.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

/*!
@brief Vertex shader entry point.
*/

void main() {
    texCoord = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
