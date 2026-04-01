#version 460 core

out vec4 FragColor;
in vec2 vUV;

uniform sampler2D uScene;

uniform float uTime;

uniform float uStrength;
uniform float uSpeed;
uniform float uWaveScale1;
uniform float uWaveScale2;
uniform float uNoiseStrength;

uniform vec2 uPlayerPos;
uniform float uPlayerRadius;
uniform float uPlayerStrength;

uniform float uLavaHeight;
uniform float uLavaBoost;

uniform vec2 uShockPos;
uniform float uShockTime;
uniform float uShockStrength;

uniform vec3 uGlowColor;
uniform float uGlowStrength;
uniform int uDebug;

void main()
{
    vec2 uv = vUV;

    if (uDebug == 1 && vUV.x < 0.03 && vUV.y > 0.97) {
        FragColor = vec4(clamp(uStrength * 50.0, 0.0, 1.0), clamp(uSpeed / 10.0, 0.0, 1.0), clamp(uNoiseStrength / 5.0, 0.0, 1.0), 1.0);
        return;
    }

    // =========================
    // BASE HEAT DISTORTION
    // =========================
    float wave1 = sin(uv.y * uWaveScale1 + uTime * uSpeed);
    float wave2 = cos(uv.y * uWaveScale2 + uTime * (uSpeed * 0.65));
    float distortion = (wave1 + wave2) * 0.5;

    // =========================
    //  NOISE BREAKUP
    // =========================
    float noise = fract(sin(dot(uv * 2.0 + uTime * 0.05, vec2(12.9898, 78.233))) * 43758.5453);
    distortion += (noise - 0.5) * uNoiseStrength;

    // =========================
    // LAVA ZONE BOOST
    // =========================
    float lavaMask = 1.0 - smoothstep(uLavaHeight, 1.0, uv.y);
    distortion *= (1.0 + lavaMask * uLavaBoost);

    // =========================
    //  PLAYER AURA (heat bubble)
    // =========================
    float distToPlayer = distance(uv, uPlayerPos);
    float playerMask = 1.0 - smoothstep(0.0, uPlayerRadius, distToPlayer);

    float playerWave = sin(uTime * 6.0 + distToPlayer * 40.0);
    distortion += playerWave * playerMask * uPlayerStrength;

    // =========================
    //  SHOCKWAVE (lava pop)
    // =========================
    float shockDist = distance(uv, uShockPos);
    float shockWave = sin(40.0 * shockDist - uShockTime * 10.0);

    float shockWindow = smoothstep(0.0, 0.2, uShockTime) * (1.0 - smoothstep(0.2, 1.0, uShockTime));
    float shockMask = exp(-shockDist * 10.0) * shockWindow;

    distortion += shockWave * shockMask * uShockStrength;

    // =========================
    //  APPLY DISTORTION
    // =========================
    uv.x += distortion * uStrength;

    vec3 color = texture(uScene, uv).rgb;

    // =========================
    //  SUBTLE GLOW BOOST (lava + player)
    // =========================
    float glow = lavaMask * 0.2 + playerMask * 0.15;
    color += uGlowColor * glow * uGlowStrength;

    FragColor = vec4(color, 1.0);
}
