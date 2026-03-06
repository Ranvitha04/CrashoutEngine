#version 460 core

out vec4 fragColor;

// Texture coordinates
in vec2 texCoord;

// Light properties (passed from vertex shader)
in vec2 vLightPos;
in float vLightRadius;
in vec3 vLightColor;
in float vIntensity;
in float vShadowIndex;
in vec3 vShapeParams;

// Uniforms
uniform vec2 uWorldSize; // Size of the world in units
uniform sampler2D uShadowTex; // Shadow distance texture
uniform float uShadowTexHeight; // Height of shadow texture
uniform int uUseShadows; // Whether to use shadows (1 = yes)

void main() {
    // Calculate vector from fragment position to light position in texture space
    vec2 d = texCoord - vLightPos;
    // Convert to world space coordinates
    vec2 dWorld = vec2(d.x * uWorldSize.x, d.y * uWorldSize.y);
    // Distance from fragment to light in world space
    float dist = length(dWorld);
    float radius = vLightRadius;
    float falloff = 0.0;
    if (vShapeParams.x < 0.5) {
        falloff = smoothstep(radius, 0.0, dist);
    }
    else {
        vec2 halfSize = vShapeParams.yz * radius;
        vec2 q = abs(dWorld) - halfSize;
        float outsideDist = length(max(q, vec2(0.0)));
        float feather = max(radius * 0.2, 0.0001);
        falloff = smoothstep(feather, 0.0, outsideDist);
    }

    // Check for shadows if enabled and light has valid shadow index
    if (vShapeParams.x < 0.5 && uUseShadows == 1 && vShadowIndex >= 0.0 && radius > 0.0001) {
        // Calculate angle from light to fragment
        float angle = atan(d.y, d.x);
        // Normalize angle to [0, 2*pi]
        angle = (angle < 0.0) ? (angle + 6.28318530718) : angle;
        // Convert angle to texture U coordinate (0 to 1)
        float u = angle / 6.28318530718;
        // V coordinate based on shadow index
        float v = (vShadowIndex + 0.5) / max(uShadowTexHeight, 1.0);
        // Sample shadow distance from texture
        float shadowNorm = texture(uShadowTex, vec2(u, v)).r;
        // Normalized distance from light
        float distNorm = dist / radius;
        // If fragment is beyond shadow distance, no light
        if (distNorm > shadowNorm) {
            falloff = 0.0;
        }
    }
    // Final light color: base color * intensity * falloff
    vec3 color = vLightColor * vIntensity * falloff;
    // Output additive light contribution
    fragColor = vec4(color, 1.0);
}
