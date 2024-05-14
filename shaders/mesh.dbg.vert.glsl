#version 460

#extension GL_EXT_scalar_block_layout: require

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
	uint  PointLightSourceCount;
	uint  SpotLightSourceCount;
	float CascadeSplits[DEPTH_CASCADES_COUNT + 1];
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	bool  DebugColors;
	bool  LightSourceShadowsEnabled;
};

struct material
{
	vec4 LightDiffuse;
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

layout(set = 0, binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };
layout(set = 0, binding = 1) readonly buffer b1 { vert_in In[]; };
layout(set = 0, binding = 2) readonly buffer b2 { mesh_draw_command MeshData[]; };
layout(set = 0, binding = 3) readonly buffer b3 { material MeshMaterials[]; };
layout(set = 0, binding = 4) readonly buffer b4 { offset Offsets[]; };

layout(location = 0) out vec4 OutCol;

void main()
{
	uint DrawID        = gl_DrawID;
	uint VertexIndex   = gl_VertexIndex   + Offsets[DrawID].VertexOffset;
	uint InstanceIndex = gl_InstanceIndex + Offsets[DrawID].InstanceOffset;

	vec4 Position = In[VertexIndex].Pos * MeshData[InstanceIndex].Scale + MeshData[InstanceIndex].Translate;
	gl_Position = WorldUpdate.Proj * WorldUpdate.View * Position;
	OutCol = MeshMaterials[MeshData[InstanceIndex].MatIdx].LightEmmit;
}

