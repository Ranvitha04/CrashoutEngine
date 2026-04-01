/*!
@file       LightShader.frag
@author     Ranvitha
@date       22/03/2026
@brief      Per-pixel lighting with per-light flicker and proper shadow darkness control.
*/

#version 460 core

layout (location = 0) out vec4 fragBloom;
layout (location = 1) out vec4 fragNoBloom;

in vec2 texCoord;

in vec2 vLightPos;
in float vLightRadius;
in vec3 vLightColor;
in float vIntensity;
in float vShadowIndex;
in float vAffectBloom;
in float vShadowTechnique;
in float vRaymarchSteps;
in float vRaymarchThreshold;
in float vFlickerStrength;
in float vFlickerSpeed;

uniform vec2 uWorldSize;
uniform sampler2D uShadowTex;
uniform sampler2D uSceneOccluder;
uniform float uShadowTexHeight;
uniform int uUseShadows;

uniform float uShadowMapStrength;
uniform float uRaymarchShadowStrength;

//  FINAL SHADOW CONTROL (from PPComponent)
uniform float uShadowDarkness;

//  PER-LIGHT FLICKER (from LightComponent)
uniform float uLightFlickerStrength;
uniform float uLightFlickerSpeed;

//  GLOBAL TIME
uniform float uTime;

//////////////////////////////////////////////////////
// Fire-like flicker (stable + natural)
//////////////////////////////////////////////////////
float fireFlicker(vec2 p)
{
    float speed = (vFlickerSpeed > 0.0) ? vFlickerSpeed : uLightFlickerSpeed;
    float t = uTime * speed;

    float wave = sin(t * 2.0 + p.x * 12.0 + p.y * 8.0);
    float noise = fract(sin(dot(p + t, vec2(12.9898,78.233))) * 43758.5453);

    float combined = mix(wave * 0.5 + 0.5, noise, 0.4);

    // Bias toward bright peaks (fire behavior)
    return pow(combined, 1.5);
}

//////////////////////////////////////////////////////

void main()
{
    vec2 d = texCoord - vLightPos;
    vec2 dWorld = vec2(d.x * uWorldSize.x, d.y * uWorldSize.y);

    float dist = length(dWorld);
    float radius = vLightRadius;

    //////////////////////////////////////////////////////
    //  Base falloff
    //////////////////////////////////////////////////////
    float falloff = smoothstep(radius, 0.0, dist);
    float baseFalloff = falloff;

    //////////////////////////////////////////////////////
    //  PER-LIGHT FLICKER
    //////////////////////////////////////////////////////
    float flick = 1.0;

    float flickerStrength = (vFlickerStrength > 0.0) ? vFlickerStrength : uLightFlickerStrength;

    if (flickerStrength > 0.001)
    {
        float f = fireFlicker(vLightPos + texCoord);

        // Stronger flicker near center (important)
        float centerBias = 1.0 - (dist / radius);
        centerBias = clamp(centerBias, 0.0, 1.0);

        f = mix(1.0, f, centerBias);

        flick = mix(1.0, f, flickerStrength);

        //  Apply to brightness
        falloff *= flick;

        //  Slight radius pulsing (alive effect)
        radius *= mix(1.0, f, 0.3);
    }

    float shadowMask = 0.0;

    //////////////////////////////////////////////////////
    //  SHADOWS
    //////////////////////////////////////////////////////
    if (uUseShadows == 1 && radius > 0.0001 && falloff > 0.0001)
    {
        //////////////////////////////////////////////////////
        // RAYMARCH
        //////////////////////////////////////////////////////
        if (vShadowTechnique > 0.5)
        {
            int steps = int(clamp(vRaymarchSteps, 4.0, 128.0));
            float tMax = clamp(dist / radius, 0.0, 1.0);

            vec2 stepDir = d / float(steps);

            bool occluded = false;
            float threshold = clamp(vRaymarchThreshold, 0.01, 1.0);

            for (int i = 1; i <= steps; ++i)
            {
                float t = float(i) / float(steps);
                if (t > tMax) break;

                vec2 uv = vLightPos + stepDir * float(i);

                if (texture(uSceneOccluder, uv).r > threshold)
                {
                    occluded = true;
                    break;
                }
            }

            if (occluded)
            {
                float strength = clamp(uRaymarchShadowStrength, 0.0, 1.0);

                //  sync shadow with flicker
                strength *= mix(1.0, flick, 0.5);

                shadowMask = max(shadowMask, strength * baseFalloff);
            }
        }

        //////////////////////////////////////////////////////
        //  SHADOW MAP
        //////////////////////////////////////////////////////
        else if (vShadowIndex >= 0.0)
        {
            float angle = atan(d.y, d.x);
            angle = (angle < 0.0) ? (angle + 6.28318530718) : angle;

            float u = angle / 6.28318530718;
            float v = (vShadowIndex + 0.5) / max(uShadowTexHeight, 1.0);

            float shadowNorm = texture(uShadowTex, vec2(u, v)).r;
            float distNorm = dist / radius;

            if (distNorm > shadowNorm)
            {
                float strength = clamp(uShadowMapStrength, 0.0, 1.0);

                // sync shadow with flicker
                strength *= mix(1.0, flick, 0.5);

                shadowMask = max(shadowMask, strength * baseFalloff);
            }
        }
    }

    //////////////////////////////////////////////////////
    //  FINAL LIGHT + SHADOW
    //////////////////////////////////////////////////////
    vec3 lightColor = vLightColor * vIntensity * falloff;

    //  TRUE SHADOW DARKNESS CONTROL
    float shadowFactor = max(0.0, 1.0 - (shadowMask * max(uShadowDarkness, 0.0)));

    // Optional contrast boost
    shadowFactor = pow(shadowFactor, 1.2);

    vec3 color = lightColor * shadowFactor;

    //////////////////////////////////////////////////////
    // OUTPUT
    //////////////////////////////////////////////////////
    if (vAffectBloom > 0.5)
    {
        fragBloom = vec4(color, shadowMask);
        fragNoBloom = vec4(0.0, 0.0, 0.0, shadowMask);
    }
    else
    {
        fragBloom = vec4(0.0, 0.0, 0.0, shadowMask);
        fragNoBloom = vec4(color, shadowMask);
    }
}
