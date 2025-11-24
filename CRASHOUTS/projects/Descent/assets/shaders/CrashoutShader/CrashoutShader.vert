#version 460 core

// Non-instanced vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;     // CHANGED: vec3 -> vec4
layout (location = 2) in vec2 aTexCoord;

// Instanced attributes
layout (location = 3) in vec3 aInstancePos;
layout (location = 4) in vec4 aInstanceColor; // CHANGED: vec3 -> vec4
layout (location = 5) in float aInstanceRot;
layout (location = 6) in float aInstanceScale;
layout (location = 7) in float aInstanceWidth;
layout (location = 8) in float aInstanceHeight;

out vec4 color;   // CHANGED: vec3 -> vec4
out vec2 texCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 u_uvSize; // Defaults to vec2(1) if unset

void main() {
    bool useInstancing = (aInstanceScale > 0.0) || (aInstanceRot != 0.0) || (length(aInstancePos) > 0.0);

    if (useInstancing) {
        // 1. Scale
        vec2 scaleXY = vec2(aInstanceWidth, aInstanceHeight);
        
        // Only fallback if width AND height are effectively ZERO.
        // This allows negative values (flipping) to persist.
        if (abs(scaleXY.x) < 0.001 && abs(scaleXY.y) < 0.001) {
            scaleXY = vec2(aInstanceScale);
        }

        // 2. Rotation
        float r = radians(aInstanceRot);
        float s = sin(r);
        float c = cos(r);

        // GLSL matrices are column-major. 
        // We want rows: [c -s], [s c]. 
        // So inputs are: c, s, -s, c.
        mat2 rot = mat2(c, s, -s, c); 

        // 3. Transform Position
        vec2 local = aPos.xy * scaleXY;
        local = rot * local;
        vec3 worldPos = vec3(local, aPos.z) + aInstancePos;

        gl_Position = uProjection * uView * uModel * vec4(worldPos, 1.0);

        // 4. Pass Data
        color = aInstanceColor;
        
        // IMPORTANT: Use the attribute UVs, not position-based UVs.
        // The C++ system sends specific UVs for sprite sheets.
        vec2 uvSize = (u_uvSize.x > 0.0 && u_uvSize.y > 0.0) ? u_uvSize : vec2(1.0);
        texCoord = aTexCoord * uvSize;
        
    } else {
        // Non-instanced fallback
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        color = aColor;
        texCoord = aTexCoord;
    }
}