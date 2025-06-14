#version 300 es

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aUV1;

out vec2 uv1;
out vec4 color;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    uv1         = aUV1;
    color       = aColor;
    gl_Position = projection * vec4(aPos, 1.0);
}