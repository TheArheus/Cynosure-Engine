#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_EXT_shader_explicit_arithmetic_types:require
#extension GL_EXT_shader_explicit_arithmetic_types_int16:require
#extension GL_EXT_shader_explicit_arithmetic_types_int8:require

struct vert_in
{
	vec4 Pos;
	vec4 Col;
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
	uint  ColorSourceCount;
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	bool  DebugColors;
};

layout(binding = 0) readonly buffer block0
{
	vert_in In[];
};

layout(binding = 1) readonly uniform block1
{
	global_world_data WorldUpdate;
};

layout(location = 0) out vec4 OutCol;

void main()
{
	gl_Position = WorldUpdate.Proj * WorldUpdate.View * In[gl_VertexIndex].Pos;
	OutCol = In[gl_VertexIndex].Col;
}

