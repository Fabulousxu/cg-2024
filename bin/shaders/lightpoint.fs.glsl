#version 330 core

out vec4 FragColor;

in vec3 Color;
in float FlashDelTime;

uniform float time;
uniform sampler2D glowTex;

void main()
{
    vec2 texCoord = gl_PointCoord;
    vec4 textureColor = texture(glowTex, texCoord);

    float glowAlpha = 0.75 + 0.25 * sin((time + FlashDelTime) * 5);

    FragColor = vec4(Color, glowAlpha) * textureColor;

    if (FragColor.a < 0.02)
        discard;
}
