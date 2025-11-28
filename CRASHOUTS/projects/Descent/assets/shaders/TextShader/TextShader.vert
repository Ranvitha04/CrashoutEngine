#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>

out vec2 TexCoords;
uniform mat4 projection;
uniform mat4 view;
uniform float uDepth;

void main()
{
    gl_Position = projection * view * vec4(vertex.xy, uDepth, 1.0);
    TexCoords = vertex.zw;
}
