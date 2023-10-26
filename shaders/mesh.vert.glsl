#version 450

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
	uint  ColorSourceCount;
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	bool  DebugColors;
};

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

struct vert_out
{
	vec4 Coord;
	vec4 Norm;
	vec4 Col;
	vec2 TextCoord;
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

layout(binding = 0) readonly buffer  block0 {vert_in In[];};
layout(binding = 1) readonly uniform block1 {global_world_data WorldUpdate;};
layout(binding = 2) readonly buffer  block2 {mesh_draw_command MeshDrawCommands[];};

layout(location = 0) out vert_out Out;
layout(location = 4) out mat3     TBN;

void main()
{
	uint VertexIndex   = gl_VertexIndex;
	uint InstanceIndex = gl_InstanceIndex;

	Out.Coord = In[VertexIndex].Pos * MeshDrawCommands[InstanceIndex].Scale + MeshDrawCommands[InstanceIndex].Translate;

	uint NormalX = (In[VertexIndex].Normal >> 24) & 0xff;
	uint NormalY = (In[VertexIndex].Normal >> 16) & 0xff;
	uint NormalZ = (In[VertexIndex].Normal >>  8) & 0xff;
	vec3 Normal  = normalize(vec3(NormalX, NormalY, NormalZ) / 127.0 - 1.0);
	vec3 Tang    = normalize(vec3(In[VertexIndex].Tangent  ));
	vec3 Bitang  = normalize(vec3(In[VertexIndex].Bitangent));
    if (dot(cross(Normal, Tang), Bitang) < 0.0)
	{
		Tang = Tang * -1.0;
	}
	TBN           = mat3(Tang, -Bitang, Normal);

	Out.Norm      = vec4(Normal, 0.0);
	Out.Col		  = MeshDrawCommands[InstanceIndex].Mat.LightEmmit;
	Out.TextCoord = In[VertexIndex].TexPos;

	gl_Position   = WorldUpdate.Proj * WorldUpdate.View * Out.Coord;
}

