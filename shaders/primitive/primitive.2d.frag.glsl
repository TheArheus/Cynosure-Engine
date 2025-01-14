#version 460

layout(binding = 1) uniform sampler2D InTexture;

layout(location = 0) in vec2 TextCoord;
layout(location = 1) in vec4 Color;  // NOTE: Alpha is 1.0 for if primitive is textured
layout(location = 0) out vec4 OutputColor;

void main()
{
	OutputColor = vec4(Color.xyz, 1.0);
	if(Color.a == 1)
		OutputColor = vec4(texture(InTexture, TextCoord) * vec4(Color.xyz, 1.0));
}
