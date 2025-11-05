#version 460 core

layout (location = 0) in vec3 aPos;         // X, Y
layout (location = 2) in vec2 aTexCoord;    // U, V

out vec2 texCoord;                          // Pass to fragment shader

void main() {
    // Pass texture coordinate to fragment shader
    texCoord = aTexCoord;

    // Set new screen position
    gl_Position = vec4(aPos, 0.0, 1.0);
}
