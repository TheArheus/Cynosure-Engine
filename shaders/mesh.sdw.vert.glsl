#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_EXT_shader_explicit_arithmetic_types:require
#extension GL_EXT_shader_explicit_arithmetic_types_int16:require
#extension GL_EXT_shader_explicit_arithmetic_types_int8:require

struct vert_in
{
	vec4 Pos;
	vec4 Tangent;
	vec4 Bitangent;
	vec2 TexPos;
	uint Normal;
};

struct mesh_draw_command
{
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
	uint MatIdx;
};

vec4 QuatMul(vec4 lhs, vec4 rhs)
{
	return vec4(lhs.xyz * rhs.w + rhs.xyz * lhs.w + cross(lhs.xyz, rhs.xyz), dot(-lhs.xyz, rhs.xyz) + lhs.w * rhs.w);
}

layout(binding = 0) readonly buffer b1 { vert_in In[]; };
layout(binding = 1) readonly buffer b2 { mesh_draw_command MeshDrawCommands[]; };
layout(push_constant) uniform pushConstant { mat4 ShadowMatrix; };

void main()
{
	vec4 Pos = In[gl_VertexIndex].Pos * MeshDrawCommands[gl_InstanceIndex].Scale + MeshDrawCommands[gl_InstanceIndex].Translate;
	gl_Position = ShadowMatrix * Pos;
}

