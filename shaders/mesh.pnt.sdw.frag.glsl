#version 450

layout(location = 0) in vec4 InPos;
layout(push_constant) uniform pushConstant 
{ 
	mat4  ShadowMatrix; 
	vec4  LightPos; 
	float FarZ;
};

void main() 
{
	float Distance = length(vec3(InPos) - vec3(LightPos));
	gl_FragDepth = Distance / FarZ;
}
