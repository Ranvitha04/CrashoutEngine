/*!
@file       TextShader.frag
@author     Renfred Lee
@date       13/03/2026
@brief      Fragment shader for font atlas sampling and text color output.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec4 textColor;

/*!
@brief Fragment shader entry point.
*/

void main()
{    
    vec4 sampled = texture(text, TexCoords);
    float fontAlpha = sampled.r; 
    FragColor = vec4(textColor.rgb, fontAlpha * textColor.a);
}
