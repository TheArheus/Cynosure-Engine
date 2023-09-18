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
	vec2 TexPos;
	uint Normal;
};

struct vert_out
{
	mat4 Proj;

	vec4 Pos;
	vec4 Coord;
	vec4 Norm;
	vec4 Col;

	vec4 ShadowPos[DEPTH_CASCADES_COUNT];
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

struct mesh_draw_command
{
	material Mat;
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
};

layout(binding = 0) readonly buffer  block0 {vert_in In[];};
layout(binding = 1) readonly uniform block1 {global_world_data WorldUpdate;};
layout(binding = 2) readonly buffer  block2 {mesh_draw_command MeshDrawCommands[];};
layout(binding = 5) uniform sampler3D RandomAnglesTexture;
layout(push_constant) uniform pushConstant { float CascadeSplits[DEPTH_CASCADES_COUNT + 1]; };

layout(location = 0) out vec4 GlobalLightPos;
layout(location = 1) out vec4 GlobalLightDir;
layout(location = 2) flat out vec2 Rotation;
layout(location = 3) out vert_out Out;

void main()
{
	uint VertexIndex   = gl_VertexIndex;
	uint InstanceIndex = gl_InstanceIndex;
	uint NormalX = (In[VertexIndex].Normal >> 24) & 0xff;
	uint NormalY = (In[VertexIndex].Normal >> 16) & 0xff;
	uint NormalZ = (In[VertexIndex].Normal >>  8) & 0xff;
	vec3 Normal = vec3(NormalX, NormalY, NormalZ) / 127.0 - 1.0;

	mat4 Bias = mat4
	(0.5,  0.0, 0.0, 0.0,
	 0.0, -0.5, 0.0, 0.0,
	 0.0,  0.0, 1.0, 0.0,
	 0.5,  0.5, 0.0, 1.0);

	Out.Proj    = WorldUpdate.Proj;
	Out.Coord   = In[VertexIndex].Pos * MeshDrawCommands[InstanceIndex].Scale + MeshDrawCommands[InstanceIndex].Translate;
	gl_Position = WorldUpdate.Proj * WorldUpdate.View * Out.Coord;

	Out.Norm = normalize(vec4(Normal, 0.0));

	for(uint CascadeIdx = 0;
		CascadeIdx < DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		Out.ShadowPos[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * Out.Coord;
	}

	Out.Pos = WorldUpdate.DebugView * Out.Coord;
	Out.Col = MeshDrawCommands[InstanceIndex].Mat.LightEmmit;

	ivec3 AnglePos = ivec3(Out.Pos.xyz + 31) / 32;
	Rotation = texture(RandomAnglesTexture, AnglePos).xy;

	GlobalLightPos = WorldUpdate.GlobalLightPos;
	GlobalLightDir = normalize(-WorldUpdate.GlobalLightPos);
}

