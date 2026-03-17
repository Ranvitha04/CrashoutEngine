/*!
@file       TextShader.vert
@author     Renfred Lee
@date       13/03/2026
@brief      Vertex shader for text rendering (positions glyph quads and passes UVs).

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>

out vec2 TexCoords;
uniform mat4 projection;
uniform mat4 view;
uniform float uDepth;

/*!
@brief Vertex shader entry point.
*/

void main()
{
    gl_Position = projection * view * vec4(vertex.xy, uDepth, 1.0);
    TexCoords = vertex.zw;
}
