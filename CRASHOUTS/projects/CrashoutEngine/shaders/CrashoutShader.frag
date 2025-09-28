#version 450 core

in vec3 color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D uTex2d;
uniform int u_useTexture = 0;  
uniform int u_modulate   = 1;   
uniform vec3 u_tint      = vec3(1.0, 1.0, 1.0);

void main() {
    vec4 baseColor;

    if (u_useTexture == 1) {
        // Sample texture
        vec4 texColor = texture(uTex2d, texCoord);

        if (u_modulate == 1) {
            baseColor = texColor * vec4(color, 1.0);
        } else {
            baseColor = texColor;
        }

        // Apply tint
        baseColor *= vec4(u_tint, 1.0);

        // If texture is almost transparent, fallback to vertex color * tint
        if (texColor.a < 0.01) {
            baseColor = vec4(color * u_tint, 1.0);
        }
    } else {
        // No texture ? use vertex color * tint
        baseColor = vec4(color * u_tint, 1.0);
    }

    fragColor = baseColor;
}
