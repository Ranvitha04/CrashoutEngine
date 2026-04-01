#version 460 core

out vec4 FragColor;

in vec2 vUV;

uniform sampler2D uScene;     // your rendered scene (color buffer)
uniform float uTime;

// controls (tweak these)
uniform float uStrength;   // distortion amount
uniform float uSpeed;      // animation speed
uniform float uScale;      // wave frequency
uniform vec3 uTintColor;
uniform float uTintStrength;
uniform int uDebug;

void main()
{
    vec2 uv = vUV;

    if (uDebug == 1 && vUV.x < 0.03 && vUV.y > 0.97) {
        FragColor = vec4(clamp(uStrength * 50.0, 0.0, 1.0), clamp(uSpeed / 10.0, 0.0, 1.0), clamp(uScale / 100.0, 0.0, 1.0), 1.0);
        return;
    }

    // vertical heat waves
    float wave = sin(uv.y * uScale + uTime * uSpeed);

    // add secondary variation (less uniform look)
    float wave2 = cos(uv.y * (uScale * 0.5) + uTime * (uSpeed * 0.7));

    float distortion = (wave + wave2) * 0.5;

    //  apply distortion horizontally
    uv.x += distortion * uStrength;

    //  optional: stronger near bottom (lava area)
    float mask = 1.0 - smoothstep(0.3, 1.0, uv.y);
    uv.x += distortion * uStrength * mask * 2.0;

    vec4 col = texture(uScene, uv);
    float heatMask = clamp(abs(distortion) * mask, 0.0, 1.0);
    col.rgb = mix(col.rgb, col.rgb + uTintColor * heatMask, clamp(uTintStrength, 0.0, 1.0));
    FragColor = col;
}
