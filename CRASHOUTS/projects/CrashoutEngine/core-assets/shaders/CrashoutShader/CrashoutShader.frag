#version 460 core

in vec4 color;    // CHANGED: vec3 -> vec4
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D uTex2d;
uniform int  u_useTexture; // 0: color only, 1: sample texture
uniform vec4 u_tint;       // CHANGED: vec3 -> vec4

void main() {
    vec4 result;

    if (u_useTexture == 1) {
        vec4 tex = texture(uTex2d, texCoord);
        
        vec4 shaded = tex * color;
        
        // Apply global tint (RGB and Alpha)
        result = shaded * u_tint;
    } else {
        // No texture: just vertex color * tint
        result = color * u_tint;
    }
    if (result.a < 0.1) {
        discard;
    }
    //result.rgb *= result.a;

    fragColor = result;
}