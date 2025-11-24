#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec4 textColor;

void main()
{    
    // Sample the font texture (which is a single-channel alpha mask)
    float fontAlpha = texture(text, TexCoords).r;
    
    // Combine the font's alpha with the user-defined color's alpha
    FragColor = vec4(textColor.rgb, fontAlpha * textColor.a);
}
