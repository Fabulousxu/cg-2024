#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 FragPos;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Color = aColor;

    vec4 viewPos = view * vec4(FragPos, 1.0);

    gl_Position = projection * view * vec4(FragPos, 1.0);
    gl_PointSize = 1.2 / -viewPos.z;
}