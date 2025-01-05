#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;
uniform int lightNum;

void main()
{
    vec3 color = 0.3 + lightNum * lightColor;

    FragColor = vec4(color, 1.0);
}