#version 450

#extension GL_EXT_scalar_block_layout: require

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

struct global_world_data
{
	mat4  View;
	mat4  DebugView;
	mat4  Proj;
	vec4 _Padding0;
	mat4  LightView[DEPTH_CASCADES_COUNT];
	mat4  LightProj[DEPTH_CASCADES_COUNT];
	vec4  CameraPos;
	vec4  CameraDir;
	vec4  GlobalLightPos;
	float GlobalLightSize;
	uint  DirectionalLightSourceCount;
	uint  PointLightSourceCount;
	uint  SpotLightSourceCount;
	float CascadeSplits[DEPTH_CASCADES_COUNT + 1];
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	uint  DebugColors;
	uint  LightSourceShadowsEnabled;
};

struct material
{
	vec4 LightEmmit;
	bool HasTexture;
	uint TextureIdx;
	bool HasNormalMap;
	uint NormalMapIdx;
	bool HasSpecularMap;
	uint SpecularMapIdx;
	bool HasHeightMap;
	uint HeightMapIdx;
	uint LightType;
};

struct mesh_draw_command
{
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
	uint MatIdx;
};

layout(binding = 0, std430) readonly uniform b0 { global_world_data WorldUpdate; };
layout(binding = 1) readonly buffer  b1 { vert_in In[]; };
layout(binding = 2) readonly buffer  b2 { mesh_draw_command MeshData[]; };
layout(binding = 3) readonly buffer  b3 { material MeshMaterials[]; };

layout(location = 0) out vec4 OutCol;

void main()
{
	vec4 Position = In[gl_VertexIndex].Pos * MeshData[gl_InstanceIndex].Scale + MeshData[gl_InstanceIndex].Translate;
	gl_Position = WorldUpdate.Proj * WorldUpdate.View * Position;
	OutCol = MeshMaterials[MeshData[gl_InstanceIndex].MatIdx].LightEmmit;
}

