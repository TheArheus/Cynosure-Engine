#version 450

layout(location = 0) in vec4 InPos;
layout(push_constant) uniform pushConstant 
{ 
	vec4  LightPos; 
	float FarZ;
	uint  LightIdx;
};

void main() 
{
	float Distance = length(vec3(InPos) - vec3(LightPos));
	gl_FragDepth = Distance / FarZ;
}
