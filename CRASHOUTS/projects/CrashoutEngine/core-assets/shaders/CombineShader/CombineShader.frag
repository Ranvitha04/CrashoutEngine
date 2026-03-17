/*!
@file       CombineShader.frag
@author     Renfred (RTIS)
@author     Ranvitha
@date       13/03/2026
@brief      Combines scene and light buffers using an occluder mask and ambient term.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uScene;
uniform sampler2D uLight;
uniform sampler2D uLightBehind;
uniform sampler2D uOccluderMask;
uniform float uAmbient;

/*!
@brief Fragment shader entry point.
*/

void main()
{
    vec4 scene = texture(uScene, texCoord);

    vec3 color = scene.rgb * uAmbient;

    color += texture(uLight, texCoord).rgb;

    float mask = texture(uOccluderMask, texCoord).r;

    if (mask < 0.999)
        color += texture(uLightBehind, texCoord).rgb * (1.0 - mask);

    fragColor = vec4(color, scene.a);
}
