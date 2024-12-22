#version 460

struct transform
{
	vec2 Pos;
	vec2 Scale;
	vec2 Offset;
	vec2 Dims;
	vec3 Color;
};

layout(binding = 1) uniform sampler2D InTexture;
layout(push_constant) uniform pushConstant { transform Transform; };

layout(location = 0) in  vec2 TextCoord;
layout(location = 0) out vec4 OutputColor;

void main()
{
	OutputColor = vec4(texture(InTexture, TextCoord).rgb, 1.0);
}
