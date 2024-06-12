#version 450

#extension GL_EXT_scalar_block_layout: require

#define light_type_none 0
#define light_type_directional 1
#define light_type_point 2
#define light_type_spot 3

const float Pi = 3.14159265358979;

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
	vec4  SceneScale;
	vec4  SceneCenter;
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

struct light_source
{
	vec4 Pos;
	vec4 Dir;
	vec4 Col;
	uint Type;
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

layout(set = 0, binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };
layout(set = 0, binding = 3) readonly buffer b3 { material MeshMaterials[]; };
layout(set = 0, binding = 5) readonly buffer b5 { light_source LightSources[LIGHT_SOURCES_MAX_COUNT]; };
layout(set = 0, binding = 6) uniform writeonly image3D VoxelTexture;
layout(set = 1, binding = 0) uniform sampler2D DiffuseSamplers[256];
layout(set = 2, binding = 0) uniform sampler2D NormalSamplers[256];
layout(set = 3, binding = 0) uniform sampler2D SpecularSamplers[256];
layout(set = 4, binding = 0) uniform sampler2D HeightSamplers[256];

layout(location = 0) in vec4 CoordWS;
layout(location = 1) in vec4 Norm;
layout(location = 2) in vec2 TextCoord;
layout(location = 3) in flat uint MatIdx;


vec3 DoDiffuse(vec3 LightCol, vec3 ToLightDir, vec3 Normal)
{
	float  NdotL = max(dot(ToLightDir, Normal), 0);
	return LightCol * NdotL;
}

float DoAttenuation(float Radius, float Distance)
{
	return 1.0 - smoothstep(Radius * 0.75, Radius, Distance);
}

float DoSpotCone(float Radius, vec3 ToLightDir, vec3 LightView)
{
	float MinCos = cos(radians(Radius));
	float MaxCos = (MinCos + 1.0f) / 2.0f;
	float CosA   = dot(-ToLightDir, LightView);
	float SpotIntensity = smoothstep(MinCos, MaxCos, CosA);
	return SpotIntensity;
}

void DirectionalLight(inout vec3 DiffuseResult, vec3 Normal, vec3 CameraDir, vec3 LightDir, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  ToLightDir = LightDir;
	float Distance   = length(ToLightDir);
	ToLightDir = normalize(ToLightDir);

	vec3 NewDiffuse  = DoDiffuse(LightCol, ToLightDir, Normal) * Intensity;

	DiffuseResult  += NewDiffuse.rgb;
}

void PointLight(inout vec3 DiffuseResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  ToLightDir = LightPos - Coord;
	float Distance   = length(ToLightDir);
	ToLightDir = normalize(ToLightDir);

	float Attenuation = DoAttenuation(Radius, Distance);
	vec3  NewDiffuse  = DoDiffuse(LightCol, ToLightDir, Normal) * Attenuation * Intensity;

	DiffuseResult  += NewDiffuse;
}

void SpotLight(inout vec3 DiffuseResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightView, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  LightPosDir = LightPos - Coord;
	float Distance    = length(LightPosDir);

	float Attenuation   = DoAttenuation(Radius, Distance);
	float SpotIntensity = DoSpotCone(Radius, LightPosDir, LightView.xyz);
	vec3  NewDiffuse    = DoDiffuse(LightCol, LightPosDir, Normal) * Attenuation * SpotIntensity * Intensity;

	DiffuseResult  += NewDiffuse;
}

void GetPBRColor(inout vec3 DiffuseColor, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightDir, vec3 LightCol, float Intensity, vec3 Diffuse, float Roughness, float Metalness)
{
	vec3  Half        = normalize(LightDir + CameraDir);
	float Distance    = length(LightDir);
	float Attenuation = 1.0 / ((1 + 0.22 * Distance / Radius) * (1 + 0.2 * Distance * Distance / (Radius * Radius)));

	float NdotL = max(dot(Normal, LightDir), 0.0);

	vec3 RadianceAmmount = LightCol * Attenuation * NdotL;
	DiffuseColor += RadianceAmmount;
}

bool IsInsideUnitCube(vec3 V)
{
	return abs(V.x) < 1.0 && abs(V.y) < 1.0 && abs(V.z) < 1.0;
}

bool IsSaturated(vec3 V)
{
	vec3   A = clamp(V, 0.0, 1.0);
	return V == A;
}

void main()
{
	vec3 VoxelSize = imageSize(VoxelTexture);
	vec3 VoxelPos = (CoordWS.xyz - WorldUpdate.SceneCenter.xyz) * WorldUpdate.SceneScale.xyz;

	if(!IsInsideUnitCube(VoxelPos)) return;

	vec4 ColDiffuse = MeshMaterials[MatIdx].LightDiffuse;
	vec4 ColEmmit   = MeshMaterials[MatIdx].LightEmmit;

	uint TextureIdx     = MeshMaterials[MatIdx].TextureIdx;
	uint NormalMapIdx   = MeshMaterials[MatIdx].NormalMapIdx;
	uint SpecularMapIdx = MeshMaterials[MatIdx].SpecularMapIdx;

	vec3 Normal = Norm.xyz;

	vec3 Diffuse = MeshMaterials[MatIdx].LightDiffuse.rgb;
	if(MeshMaterials[MatIdx].HasTexture)
		 Diffuse = texture(DiffuseSamplers[TextureIdx], TextCoord).rgb;

	vec3 ViewDirWS = normalize(WorldUpdate.CameraPos.xyz - CoordWS.xyz);

	vec3 LightDiffuse = vec3(0);
	uint TotalLightSourceCount = WorldUpdate.PointLightSourceCount + 
								 WorldUpdate.SpotLightSourceCount;

	float Metallic  = 1.0;
	float Roughness = 0.8;
	for(uint LightIdx = 0;
		LightIdx < TotalLightSourceCount;
		++LightIdx)
	{
		vec3 LightSourcePosWS = LightSources[LightIdx].Pos.xyz;
		vec3 LightSourceDirWS = LightSources[LightIdx].Dir.xyz;
		vec3 LightSourceCol   = LightSources[LightIdx].Col.xyz;
		float Intensity       = LightSources[LightIdx].Col.w;
		float Radius          = LightSources[LightIdx].Pos.w;

		if(LightSources[LightIdx].Type == light_type_point)
		{
			vec3 LightDirWS = LightSourcePosWS - CoordWS.xyz;
			GetPBRColor(LightDiffuse, CoordWS.xyz, Normal, ViewDirWS, LightSourcePosWS, Radius, LightDirWS, LightSourceCol, Intensity, Diffuse, Roughness, Metallic);
		}
		else if(LightSources[LightIdx].Type == light_type_spot)
		{
			GetPBRColor(LightDiffuse, CoordWS.xyz, Normal, ViewDirWS, LightSourcePosWS, Radius, LightSourceDirWS, LightSourceCol, Intensity, Diffuse, Roughness, Metallic);
		}
	}

	VoxelPos = VoxelPos * 0.5 + 0.5;
	VoxelPos = VoxelPos * VoxelSize;
	imageStore(VoxelTexture, ivec3(VoxelPos), vec4(LightDiffuse, 1));
}
