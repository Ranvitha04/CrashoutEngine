#version 330 core

in vec4 color;      // CHANGED: vec3 -> vec4
in vec2 texCoord;

uniform sampler2D uTex2d;
uniform int u_useTexture; 
uniform int u_modulate;   
uniform vec4 u_tint;      // CHANGED: vec3 -> vec4 (to allow fading via uniform)

out vec4 FragColor;

void main() {
    // 1. Base Color (Vertex Color)
    // We use the alpha passed from the vertex shader
    vec4 baseColor = color; 

    if (u_useTexture == 1) {
        vec4 texColor = texture(uTex2d, texCoord);

        // 2. Texture & Modulation
        // Multiply RGBA together. This allows texture alpha to stack with vertex alpha.
        vec4 shaded = (u_modulate == 1) ? (texColor * baseColor) : texColor;

        // 3. Global Tint
        // Multiply the final result by the global tint (including tint Alpha)
        FragColor = shaded * u_tint;
    } else {
        // No texture: use vertex color * global tint
        FragColor = baseColor * u_tint;
    }
}