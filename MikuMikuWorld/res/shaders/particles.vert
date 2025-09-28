#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aUV1;

out vec2 uv1;
out vec4 color;
out float blend;

uniform mat4 viewProjection;

void main()
{
    uv1         = aUV1.xy;
    color       = aColor;
	blend		= aUV1.z;
    gl_Position = viewProjection * vec4(aPos, 1.0);
}