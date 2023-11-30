#version 450

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_EXT_shader_explicit_arithmetic_types:require
#extension GL_EXT_shader_explicit_arithmetic_types_int16:require
#extension GL_EXT_shader_explicit_arithmetic_types_int8:require

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

struct vert_in
{
	vec4 Pos;
	vec4 Tangent;
	vec4 Bitangent;
	vec2 TexPos;
	uint Normal;
};

struct vert_out
{
	vec4 Coord;
	vec4 Norm;
	vec4 Col;
	vec2 TextCoord;
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

layout(set = 0, binding = 0, std430) readonly uniform b0 { global_world_data WorldUpdate; };
layout(set = 0, binding = 1) readonly buffer  b1 { vert_in In[]; };
layout(set = 0, binding = 2) readonly buffer  b2 { mesh_draw_command MeshDrawCommands[]; };
layout(set = 0, binding = 3) readonly buffer  b3 { material MeshMaterials[]; };

layout(location = 0) out vert_out Out;
layout(location = 4) out uint     MatIdx;
layout(location = 5) out mat3     TBN;

void main()
{
	uint VertexIndex   = gl_VertexIndex;
	uint InstanceIndex = gl_InstanceIndex;
	MatIdx = MeshDrawCommands[InstanceIndex].MatIdx;

	Out.Coord = In[VertexIndex].Pos * MeshDrawCommands[InstanceIndex].Scale + MeshDrawCommands[InstanceIndex].Translate;

	uint NormalX = (In[VertexIndex].Normal >> 24) & 0xff;
	uint NormalY = (In[VertexIndex].Normal >> 16) & 0xff;
	uint NormalZ = (In[VertexIndex].Normal >>  8) & 0xff;
	vec3 Normal  = normalize(vec3(NormalX, NormalY, NormalZ) / 127.0 - 1.0);
	vec3 Tang    = normalize(vec3(In[VertexIndex].Tangent  ));
	vec3 Bitang  = normalize(vec3(In[VertexIndex].Bitangent));
	TBN          = mat3(Tang, Bitang, Normal);

	Out.Norm      = vec4(Normal, 0.0);
	Out.Col		  = MeshMaterials[MatIdx].LightEmmit;
	Out.TextCoord = In[VertexIndex].TexPos;

	gl_Position   = WorldUpdate.Proj * WorldUpdate.View * Out.Coord;
}

