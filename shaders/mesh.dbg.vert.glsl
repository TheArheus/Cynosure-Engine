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
	uint  DebugColors;
	uint  LightSourceShadowsEnabled;
};

struct material
{
	vec4  LightEmmit;
	float Specular;
	uint  TextureIdx;
	uint  NormalMapIdx;
	uint  LightType;
};

struct mesh_draw_command_data
{
	material Mat;
	vec4 Translate;
	vec4 Scale;
	uint MeshIndex;
};

layout(binding = 0) readonly buffer block0
{
	vert_in In[];
};

layout(binding = 1) readonly uniform block1
{
	global_world_data WorldUpdate;
};

layout(binding = 2) readonly buffer block2
{
	mesh_draw_command_data MeshData[];
};

layout(location = 0) out vec4 OutCol;

void main()
{
	vec4 Position = In[gl_VertexIndex].Pos * MeshData[gl_InstanceIndex].Scale + MeshData[gl_InstanceIndex].Translate;
	gl_Position = WorldUpdate.Proj * WorldUpdate.View * Position;
	OutCol = MeshData[gl_InstanceIndex].Mat.LightEmmit;
}

