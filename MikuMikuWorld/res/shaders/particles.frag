#version 330 core

precision highp float;

in vec2 uv1;
in vec4 color;
in float blend;

out vec4 fragColor;
uniform sampler2D diffuse;
uniform float blendFactor;

void main()
{
    vec4 texColor = texture(diffuse, uv1) * color;
	
	float blendFactor = (0.5 >= blend) ? 1.0 : 0.0;
	float alpha = blendFactor * texColor.w;
	
    fragColor = vec4(texColor.xyz * texColor.www, alpha);
}
