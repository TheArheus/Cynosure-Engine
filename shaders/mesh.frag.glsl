#version 450

#extension GL_EXT_scalar_block_layout: require

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

struct vert_in
{
	vec4 Coord;
	vec4 Norm;
	vec2 TextCoord;
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
layout(set = 1, binding = 0) uniform sampler2D DiffuseSamplers[256];
layout(set = 2, binding = 0) uniform sampler2D NormalSamplers[256];
layout(set = 3, binding = 0) uniform sampler2D SpecularSamplers[256];
layout(set = 4, binding = 0) uniform sampler2D HeightSamplers[256];

layout(location = 0) in vert_in   In;
layout(location = 3) in flat uint MatIdx;
layout(location = 4) in mat3      TBN;

layout(location = 0) out vec4 OutputVertexNormal;
layout(location = 1) out vec4 OutputFragmentNormal;
layout(location = 2) out vec4 OutputDiffuse;
layout(location = 3) out vec4 OutputEmmit;
layout(location = 4) out vec2 OutputSpecularEmmit;


void main()
{
	vec4 ColEmmit = MeshMaterials[MatIdx].LightEmmit;
	OutputEmmit			= vec4(ColEmmit.rgb, 1.0);

	uint TextureIdx     = MeshMaterials[MatIdx].TextureIdx;
	uint NormalMapIdx   = MeshMaterials[MatIdx].NormalMapIdx;
	uint SpecularMapIdx = MeshMaterials[MatIdx].SpecularMapIdx;
	uint HeightMapIdx   = MeshMaterials[MatIdx].HeightMapIdx;

	vec2 TextCoord = In.TextCoord;
	vec3 ViewDir = (WorldUpdate.CameraPos - In.Coord).xyz;
	if((length(ViewDir) < 4) && MeshMaterials[MatIdx].HasHeightMap)
	{
		ViewDir = normalize(transpose(TBN) * ViewDir);
		float HeightScale = 0.04;

		vec3 Guide = vec3(0, 1, 0);
		if(abs(dot(ViewDir, Guide)) == 1.0)
			 Guide = vec3(0, 0, 1);

		const float MinLayers   = 32 ;
		const float MaxLayers   = 128;
		float LayersCount       = mix(MaxLayers, MinLayers, max(dot(Guide, ViewDir), 0.0));
		float LayersDepth       = 1.0 / LayersCount;
		float CurrentLayerDepth = 0.0;

		vec2  DeltaTextCoord   = ViewDir.xy * HeightScale * LayersDepth;
		vec2  CurrentTextCoord = In.TextCoord;
		float CurrentDepth     = 1.0 - texture(HeightSamplers[HeightMapIdx], CurrentTextCoord).r;
		while(CurrentLayerDepth < CurrentDepth)
		{
			CurrentTextCoord  -= DeltaTextCoord;
			CurrentDepth       = 1.0 - texture(HeightSamplers[HeightMapIdx], CurrentTextCoord).r;
			CurrentLayerDepth += LayersDepth;
		}

		vec2 PrevTextCoord = CurrentTextCoord + DeltaTextCoord;

		float AfterDepth  = CurrentDepth - CurrentLayerDepth;
		float BeforeDepth = 1.0 - texture(HeightSamplers[HeightMapIdx], PrevTextCoord).x - CurrentLayerDepth + LayersDepth;
		float Weight	  = AfterDepth / (AfterDepth - BeforeDepth);
		TextCoord         = mix(PrevTextCoord, CurrentTextCoord, Weight);

		if(TextCoord.x < 0.0 || TextCoord.y < 0.0 || TextCoord.x > 1.0 || TextCoord.y > 1.0) discard; // clamp?
	}

	OutputVertexNormal   = In.Norm * 0.5 + 0.5;
	OutputFragmentNormal = OutputVertexNormal;
	if(MeshMaterials[MatIdx].HasNormalMap)
		OutputFragmentNormal = vec4((TBN * texture(NormalSamplers[NormalMapIdx], TextCoord).rgb) * 0.5 + 0.5, 0);
#if DEBUG_COLOR_BLEND
	OutputDiffuse		 = vec4(vec3(0.1) * In.Col.rgb, 1.0);
#else
	OutputDiffuse		 = MeshMaterials[MatIdx].LightDiffuse;
	if(MeshMaterials[MatIdx].HasTexture)
		OutputDiffuse    = vec4(texture(DiffuseSamplers[TextureIdx], TextCoord).rgb, 1);
#endif
	OutputSpecularEmmit	 = vec2(0.0, ColEmmit.w);
	if(MeshMaterials[MatIdx].HasSpecularMap)
		OutputSpecularEmmit = vec2(texture(SpecularSamplers[SpecularMapIdx], TextCoord).r, ColEmmit.w);
}
