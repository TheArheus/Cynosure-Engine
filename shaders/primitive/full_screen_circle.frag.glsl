#version 460

struct transform
{
	vec2 Pos;
	vec2 Dim;
	vec3 Color;
};

layout(push_constant) uniform pushConstant { transform Transform; };

layout(location = 0) in  vec2 TextCoord;
layout(location = 0) out vec4 OutputColor;

void main()
{
	vec2 UV = TextCoord * 2.0 - 1.0;
	if(UV.x * UV.x + UV.y * UV.y > 1.0) discard;
	OutputColor = vec4(Transform.Color, 1.0);
}
