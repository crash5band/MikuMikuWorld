#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aUV1;
layout (location = 3) in vec2 aUV2;

out vec2 baseUV;
out vec2 maskUV;
out vec4 color;

uniform mat4 projection;

void main()
{
    baseUV      = aUV1;
    maskUV      = aUV2;
    color       = aColor;
    gl_Position = projection * vec4(aPos, 1.0);
}