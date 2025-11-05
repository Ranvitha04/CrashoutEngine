#version 450 core

layout (location = 0) in vec3 aPos;

// Non-instanced attributes 
layout (location = 1) in vec3 aColor;          // Color rendering
layout (location = 2) in vec2 aTexCoord;       // Texture coordinates

// Instance attributes 
layout (location = 3) in vec3 aInstancePos;    // Instance position
layout (location = 4) in vec3 aInstanceColor;  // Instance color
layout (location = 5) in float aInstanceRot;   // Instance rotation
layout (location = 6) in float aInstanceScale; // Instance scale

out vec3 color;
out vec2 texCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    // Check if we're doing instanced rendering by checking if instance data is valid
    // If aInstanceScale is 0, we're not using instancing
    bool isInstanced = (aInstanceScale > 0.0);
    
    if (isInstanced) {
        // Instanced rendering path
        mat4 instanceModel = mat4(1.0);
        
        // Apply scaling
        instanceModel[0][0] = aInstanceScale;
        instanceModel[1][1] = aInstanceScale;
        instanceModel[2][2] = aInstanceScale;
        
        // Apply rotation (around Z-axis)
        float cosR = cos(radians(aInstanceRot));
        float sinR = sin(radians(aInstanceRot));
        
        mat4 rotation = mat4(1.0);
        rotation[0][0] = cosR;
        rotation[0][1] = sinR;
        rotation[1][0] = -sinR;
        rotation[1][1] = cosR;
        
        instanceModel = rotation * instanceModel;
        
        // Apply translation
        instanceModel[3][0] = aInstancePos.x;
        instanceModel[3][1] = aInstancePos.y;
        instanceModel[3][2] = aInstancePos.z;
        
        // Transform vertex position
        vec4 worldPos = instanceModel * vec4(aPos, 1.0);
        gl_Position = uProjection * uView * uModel * worldPos;
        
        // Use instance color and generate basic texture coordinates
        color = aInstanceColor;
        
        // Generate texture coordinates based on vertex position
        // Map from [-0.5, 0.5] to [0.0, 1.0]
        texCoord = aPos.xy + 0.5;
    } else {
        // Non-instanced rendering path (regular shapes and textured shapes)
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        
        // Use vertex color and texture coordinates
        color = aColor;
        texCoord = aTexCoord;
    }
}