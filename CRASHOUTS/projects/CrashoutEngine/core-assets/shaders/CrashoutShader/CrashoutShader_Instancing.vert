#version 330 core

// Vertex Attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;        // CHANGED: vec3 -> vec4
layout (location = 2) in vec2 aTexCoord;

// Instance Attributes
layout (location = 3) in vec2  aInstancePos;
layout (location = 4) in float aInstanceScale;
layout (location = 5) in float aInstanceRot;
layout (location = 6) in vec4  aInstanceColor; // CHANGED: vec3 -> vec4
layout (location = 7) in float aInstanceWidth;
layout (location = 8) in float aInstanceHeight;
layout (location = 9) in vec2  aInstanceOffset;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 u_uvSize;

// Outputs to Fragment Shader
out vec4 color;      // CHANGED: vec3 -> vec4
out vec2 texCoord;

void main() {
    
    bool useInstancing = (aInstanceScale > 0.0) || (aInstanceRot != 0.0) || (length(aInstancePos) > 0.0);

    if (useInstancing) {
        vec2 scaleXY = (aInstanceWidth > 0.0 && aInstanceHeight > 0.0)
            ? vec2(aInstanceWidth, aInstanceHeight)
            : vec2(aInstanceScale);

        float r = radians(aInstanceRot);
        float c = cos(r);
        float s = sin(r);
        mat2 rot = mat2(c, -s,
                        s,  c);

        vec2 local = rot * (aPos.xy * scaleXY);
        vec3 worldPos = vec3(local, aPos.z) + vec3(aInstancePos, 0.0); // Fixed vec3 cast

        gl_Position = uProjection * uView * uModel * vec4(worldPos, 1.0);

        color = aInstanceColor; // Now preserves Alpha from instance data
        
        vec2 uvSize = (u_uvSize.x > 0.0 && u_uvSize.y > 0.0) ? u_uvSize : vec2(1.0);
        texCoord = (aTexCoord * uvSize) + aInstanceOffset;
    } else {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        
        color = aColor; // Now preserves Alpha from vertex data
        
        texCoord = aTexCoord;
    }
}