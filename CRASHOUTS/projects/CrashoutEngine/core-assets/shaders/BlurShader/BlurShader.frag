#version 460 core

out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D uImage;
uniform int uHorizontal;

const float weights[3] = float[](
    0.2941176,
    0.2352941,
    0.1176470
);

void main()
{
    vec2 texOffset = 1.0 / vec2(textureSize(uImage, 0));

    vec3 result = texture(uImage, texCoord).rgb * weights[0];

    if (uHorizontal == 1)
    {
        result += texture(uImage, texCoord + vec2(texOffset.x,0)).rgb * weights[1];
        result += texture(uImage, texCoord - vec2(texOffset.x,0)).rgb * weights[1];

        result += texture(uImage, texCoord + vec2(texOffset.x*2,0)).rgb * weights[2];
        result += texture(uImage, texCoord - vec2(texOffset.x*2,0)).rgb * weights[2];
    }
    else
    {
        result += texture(uImage, texCoord + vec2(0,texOffset.y)).rgb * weights[1];
        result += texture(uImage, texCoord - vec2(0,texOffset.y)).rgb * weights[1];

        result += texture(uImage, texCoord + vec2(0,texOffset.y*2)).rgb * weights[2];
        result += texture(uImage, texCoord - vec2(0,texOffset.y*2)).rgb * weights[2];
    }

    fragColor = vec4(result, 1.0);
}