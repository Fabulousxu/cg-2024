#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in float aFlashDelTime;

out vec3 FragPos;
out vec3 Color;
out float FlashDelTime;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Color = aColor;
    FlashDelTime = aFlashDelTime;

    vec4 viewPos = view * vec4(FragPos, 1.0);

    gl_Position = projection * view * vec4(FragPos, 1.0);
    gl_PointSize = 9.0 / -viewPos.z;
}
