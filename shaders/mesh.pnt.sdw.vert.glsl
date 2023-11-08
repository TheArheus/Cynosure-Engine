#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_EXT_shader_explicit_arithmetic_types:require
#extension GL_EXT_shader_explicit_arithmetic_types_int16:require
#extension GL_EXT_shader_explicit_arithmetic_types_int8:require

struct material
{
	vec4  LightEmmit;
	float Specular;
	uint  TextureIdx;
	uint  NormalMapIdx;
	uint  LightType;
};

struct vert_in
{
	vec4 Pos;
	vec4 Tangent;
	vec4 Bitangent;
	vec2 TexPos;
	uint Normal;
};

struct global_world_data
{
	mat4  View;
	mat4  DebugView;
	mat4  Proj;
	mat4  LightView[DEPTH_CASCADES_COUNT];
	mat4  LightProj[DEPTH_CASCADES_COUNT];
	vec4  CameraPos;
	vec4  CameraDir;
	vec4  GlobalLightPos;
	float GlobalLightSize;
	uint  DirectionalLightSourceCount;
	uint  PointLightSourceCount;
	uint  SpotLightSourceCount;
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	bool  DebugColors;
};

struct mesh_draw_command
{
	material Mat;
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
};

vec4 QuatMul(vec4 lhs, vec4 rhs)
{
	return vec4(lhs.xyz * rhs.w + rhs.xyz * lhs.w + cross(lhs.xyz, rhs.xyz), dot(-lhs.xyz, rhs.xyz) + lhs.w * rhs.w);
}

layout(binding = 0) readonly buffer block0
{
	vert_in In[];
};

layout(binding = 1) readonly buffer block1
{
	mesh_draw_command MeshDrawCommands[];
};
layout(push_constant) uniform pushConstant 
{ 
	mat4  ShadowMatrix; 
	vec4  LightPos; 
	float FarZ;
};
layout(location = 0) out vec4 OutPos;

void main()
{
	OutPos = In[gl_VertexIndex].Pos * MeshDrawCommands[gl_InstanceIndex].Scale + MeshDrawCommands[gl_InstanceIndex].Translate;
	gl_Position = ShadowMatrix * OutPos;
}

