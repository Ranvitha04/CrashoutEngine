#version 460 core

// Per-vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

// Per-instance attributes
layout (location = 3) in vec3 aInstancePos;
layout (location = 4) in vec3 aInstanceColor;
layout (location = 5) in float aInstanceRot;   // degrees
layout (location = 6) in float aInstanceScale; // uniform scale
layout (location = 7) in float aInstanceWidth; // width for rectangles
layout (location = 8) in float aInstanceHeight; // height for rectangles

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// Varyings
out vec3 vColor;
out vec2 vTexCoord;

void main() {
    // Basic per-vertex values
    vec3 pos = aPos;
    vec3 color = aColor;

    // Detect instancing: when instance attributes are bound, at least one will be non-zero.
    bool isInstanced = (aInstancePos.x != 0.0 || aInstancePos.y != 0.0 || aInstancePos.z != 0.0 ||
                        aInstanceRot != 0.0 || aInstanceScale != 0.0 ||
                        aInstanceWidth != 0.0 || aInstanceHeight != 0.0);

    if (isInstanced) {
        // Apply instance scale and rotation (around origin), then translate
        float rad = radians(aInstanceRot);
        float c = cos(rad);
        float s = sin(rad);
        mat2 rot = mat2(c, -s, s, c);

        // Use width/height if available, otherwise use uniform scale
        vec2 scale = vec2(aInstanceScale != 0.0 ? aInstanceScale : 1.0);
        if (aInstanceWidth != 0.0 && aInstanceHeight != 0.0) {
            scale = vec2(aInstanceWidth, aInstanceHeight);
        }

        vec2 xy = rot * (pos.xy * scale);
        pos = vec3(xy, pos.z) + aInstancePos;

        color = aInstanceColor;
    }

    // Standard MVP
    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);

    vColor = color;
    vTexCoord = aTexCoord;
}