#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec4 textColor;

void main()
{    
    vec4 sampled = texture(text, TexCoords);
    float fontAlpha = sampled.r; 
    FragColor = vec4(textColor.rgb, fontAlpha * textColor.a);
}