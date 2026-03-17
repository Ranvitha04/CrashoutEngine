/*!
@file       PPShader.vert
@author     Renfred Lee
@date       13/03/2026
@brief      Fullscreen vertex shader for post-processing passes.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

layout (location = 0) in vec2 aPos;         // X, Y
layout (location = 1) in vec2 aTexCoord;    // U, V

out vec2 texCoord;                          // Pass to fragment shader

/*!
@brief Vertex shader entry point.
*/

void main() {
    // Pass texture coordinate to fragment shader
    texCoord = aTexCoord;

    // Set new screen position
    gl_Position = vec4(aPos, 0.0, 1.0);
}
