#version 460 core

in vec3 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D uTex2d;
uniform int   u_useTexture; // 0: color only, 1: sample texture
uniform int   u_modulate;   // FIX: make boolean-like switch (set via glUniform1i)
uniform vec3  u_tint;

void main() {
    if (u_useTexture == 1) {
        vec4 tex = texture(uTex2d, texCoord);

        if(tex.a < 0.1) { discard; }

        vec3 rgb = tex.rgb * u_tint;
        if (u_modulate != 0) {
            rgb *= color;    // multiply texture by vertex color when modulating
        }
        fragColor = vec4(rgb, tex.a);
    } else {
        fragColor = vec4(color * u_tint, 1.0);
    }
}