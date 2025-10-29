#version 460 core

in vec3 vColor;
in vec2 vTexCoord;

uniform sampler2D uTex2d;
uniform int u_useTexture; // 0 = color only, 1 = use texture
uniform vec3 u_tint;
uniform int u_modulate;   // 0 = ignore vColor in textured path, 1 = multiply

out vec4 frag_color;

void main() {
    if (u_useTexture != 0) {
        vec4 tex = texture(uTex2d, vTexCoord);
        vec3 rgb = tex.rgb * u_tint;
        if (u_modulate != 0) {
            rgb *= vColor;
        }
        frag_color = vec4(rgb, tex.a);
    } else {
        frag_color = vec4(vColor * u_tint, 1.0);
    }
}