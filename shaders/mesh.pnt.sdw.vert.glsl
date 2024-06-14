#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_EXT_shader_explicit_arithmetic_types:require
#extension GL_EXT_shader_explicit_arithmetic_types_int16:require
#extension GL_EXT_shader_explicit_arithmetic_types_int8:require

struct sphere
{
	vec4  Center;
	float Radius;
};

struct aabb
{
	vec4 Min;
	vec4 Max;
};

struct offset
{
	aabb AABB;
	sphere BoundingSphere;

	uint VertexOffset;
	uint VertexCount;

	uint IndexOffset;
	uint IndexCount;

	uint InstanceOffset;
	uint InstanceCount;
};

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

layout(binding = 0) readonly buffer b0 { vert_in In[]; };
layout(binding = 1) readonly buffer b1 { mesh_draw_command MeshDrawCommands[]; };
layout(binding = 2) readonly buffer b2 { offset Offsets[]; };
layout(location = 0) out vec4 OutPos;

layout(push_constant) uniform pushConstant 
{ 
	mat4  ShadowMatrix; 
	vec4  LightPos; 
	float FarZ;
};

void main()
{
	uint DrawID        = gl_DrawID;
	uint VertexIndex   = gl_VertexIndex   + Offsets[0].VertexOffset;
	uint InstanceIndex = gl_InstanceIndex + Offsets[0].InstanceOffset;

	OutPos = In[VertexIndex].Pos * MeshDrawCommands[InstanceIndex].Scale + MeshDrawCommands[InstanceIndex].Translate;
	gl_Position = ShadowMatrix * OutPos;
}
