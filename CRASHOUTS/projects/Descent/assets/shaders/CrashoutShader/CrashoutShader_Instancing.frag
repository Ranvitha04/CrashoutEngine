#version 330 core

in vec4 color;      // CHANGED: vec3 -> vec4
in vec2 texCoord;

uniform sampler2D uTex2d;
uniform int u_useTexture; 
uniform int u_modulate;   
uniform vec4 u_tint;      // CHANGED: vec3 -> vec4 (to allow fading via uniform)

out vec4 FragColor;

void main() {
    vec4 result;

    // --- 1. Calculate Straight Color (RGB + A separate) ---
    if (u_useTexture == 1) {
        vec4 texColor = texture(uTex2d, texCoord);

        // Modulate Texture with Vertex Color
        vec4 shaded = (u_modulate == 1) ? (texColor * color) : texColor;

        // Apply Global Tint here (combine all alpha/color sources)
        result = shaded * u_tint;

    } else {
        // No texture: Vertex Color * Global Tint
        result = color * u_tint;
    }

    // --- 2. Alpha Discard (Optional) ---
    // Even with pre-multiplied alpha, discarding very low alpha 
    // saves the GPU from processing pixels that are effectively invisible.
    if (result.a < 0.01) {
        discard;
    }

    // --- 3. THE FIX: Pre-Multiply Alpha ---
    // We do this at the very end to ensure 'result.rgb' includes 
    // the influence of the Texture, Vertex Color, AND the Tint.
    result.rgb *= result.a;

    FragColor = result;
}