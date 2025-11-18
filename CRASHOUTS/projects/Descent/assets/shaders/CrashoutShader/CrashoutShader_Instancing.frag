#version 330 core

in vec3 color;       // must match vertex shader's `out vec3 color`
in vec2 texCoord;    // must match vertex shader's `out vec2 texCoord`

uniform sampler2D uTex2d;
uniform int u_useTexture; // set via glUniform1i
uniform int u_modulate;   // set via glUniform1i
uniform vec3 u_tint;      // optional tint

out vec4 FragColor;

void main() {
    vec4 baseColor = vec4(color, 1.0);

    if (u_useTexture == 1) {
        vec4 texColor = texture(uTex2d, texCoord);

        // If modulate is on, multiply vertex color with texture; otherwise use texture only
        vec4 shaded = (u_modulate == 1) ? (texColor * baseColor) : texColor;

        // Apply tint (multiply, consistent with engine usage)
        FragColor = vec4(shaded.rgb * u_tint, shaded.a);
    } else {
        // No texture: use vertex color + tint
        FragColor = vec4(baseColor.rgb * u_tint, baseColor.a);
    }
}