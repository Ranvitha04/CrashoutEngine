#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uScene;

// Time
uniform float uTime;

// Vignette controls
uniform float uVignetteStrength;
uniform float uVignetteRadius;

// NEW: center offset (-1 to 1 range)
uniform vec2 uCenterOffset;

// Pulse
uniform float uPulseSpeed;
uniform float uPulseAmount;

void main()
{
    vec3 color = texture(uScene, TexCoords).rgb;

    // Convert UV → [-1,1]
    vec2 pos = TexCoords * 2.0 - 1.0;

    // apply offset (this is what makes it "follow")
    pos -= uCenterOffset;

    float dist = length(pos);

    //  stronger, more noticeable pulse
    float pulse = sin(uTime * uPulseSpeed) * uPulseAmount
                + sin(uTime * uPulseSpeed * 2.3) * uPulseAmount * 0.5;

    float radius = uVignetteRadius + pulse;

    float vignette = smoothstep(radius, radius - uVignetteStrength, dist);

    color *= vignette;

    FragColor = vec4(color, 1.0);
}