#version 300 es

precision mediump float;

in vec2 baseUV;
in vec2 maskUV;
in vec4 color;

out vec4 fragColor;

uniform sampler2D baseTex;
uniform sampler2D maskTex;

void main()
{
    vec4 maskColor = texture(maskTex, maskUV) * color;
    if (maskColor.a < 0.039)
        discard;
    fragColor = texture(baseTex, baseUV) * color;
}