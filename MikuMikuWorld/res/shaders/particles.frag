#version 330 core

precision mediump float;

in vec2 uv1;
in vec4 color;

out vec4 fragColor;
uniform sampler2D diffuse;
uniform float blendFactor;

void main()
{
    vec4 texColor = texture(diffuse, uv1) * color;
    fragColor = vec4(texColor.rgb * texColor.a, texColor.a * blendFactor);
}
