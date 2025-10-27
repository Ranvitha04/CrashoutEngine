#version 460 core

// Non-instanced vertex attributes (set by TextureRenderer/Shapes)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

// Instanced attributes (optional; set by instanced draws in Shapes)
layout (location = 3) in vec3 aInstancePos;    // Instance position
layout (location = 4) in vec3 aInstanceColor;  // Instance color
layout (location = 5) in float aInstanceRot;   // Instance rotation (degrees)
layout (location = 6) in float aInstanceScale; // Instance uniform scale
layout (location = 7) in float aInstanceWidth; // Optional non-uniform width
layout (location = 8) in float aInstanceHeight;// Optional non-uniform height

out vec3 color;
out vec2 texCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 u_uvSize; // Optional; defaults to vec2(1) if unset

void main() {
    bool useInstancing = (aInstanceScale > 0.0) || (aInstanceRot != 0.0) || (length(aInstancePos) > 0.0);

    if (useInstancing) {
        vec2 scaleXY = vec2(aInstanceWidth, aInstanceHeight);
        if (!(scaleXY.x > 0.0 && scaleXY.y > 0.0)) {
            scaleXY = vec2(aInstanceScale);
        }

        float r = radians(aInstanceRot);
        float s = sin(r);
        float c = cos(r);           // FIX: define 'c' used below

        mat2 rot = mat2(c, -s,
                        s,  c);

        vec2 local = aPos.xy * scaleXY;
        local = rot * local;

        vec3 worldPos = vec3(local, aPos.z) + aInstancePos;
        gl_Position = uProjection * uView * uModel * vec4(worldPos, 1.0);

        color = aInstanceColor;
        vec2 uvSize = (u_uvSize.x > 0.0 && u_uvSize.y > 0.0) ? u_uvSize : vec2(1.0);
        texCoord = (aPos.xy + 0.5) * uvSize;
    } else {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        color = aColor;
        texCoord = aTexCoord;
    }
}
