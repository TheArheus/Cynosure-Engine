#version 450

#extension GL_EXT_scalar_block_layout:  require
#extension GL_EXT_nonuniform_qualifier: require

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

struct vert_in
{
	vec4 Coord;
	vec4 Norm;
	vec4 Col;
	vec2 TextCoord;
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

layout(set = 0, binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };
layout(set = 0, binding = 3) readonly buffer b3 { material MeshMaterials[]; };
layout(set = 1, binding = 0) uniform sampler2D DiffuseSamplers[];
layout(set = 1, binding = 1) uniform sampler2D NormalSamplers[];
layout(set = 1, binding = 2) uniform sampler2D SpecularSamplers[];
layout(set = 1, binding = 3) uniform sampler2D HeightSamplers[];

layout(location = 0) in vert_in   In;
layout(location = 4) in flat uint MatIdx;
layout(location = 5) in mat3      TBN;

layout(location = 0) out vec4  OutputVertexPosition;
layout(location = 1) out vec4  OutputVertexNormal;
layout(location = 2) out vec4  OutputFragmentNormal;
layout(location = 3) out vec4  OutputDiffuse;
layout(location = 4) out float OutputSpecular;


void main()
{
	OutputVertexPosition = In.Coord;
	OutputVertexNormal   = In.Norm * 0.5 + 0.5;

	uint TextureIdx     = MeshMaterials[nonuniformEXT(MatIdx)].TextureIdx;
	uint NormalMapIdx   = MeshMaterials[nonuniformEXT(MatIdx)].NormalMapIdx;
	uint SpecularMapIdx = MeshMaterials[nonuniformEXT(MatIdx)].SpecularMapIdx;
	uint HeightMapIdx   = MeshMaterials[nonuniformEXT(MatIdx)].HeightMapIdx;

	vec2 TextCoord = In.TextCoord;
	vec3 ViewDir = (WorldUpdate.CameraPos - In.Coord).xyz;
	if((length(ViewDir) < 6) && MeshMaterials[nonuniformEXT(MatIdx)].HasHeightMap)
	{
		ViewDir = normalize(transpose(TBN) * ViewDir);
		float HeightScale = 0.04;

		const float MinLayers   = 64 ;
		const float MaxLayers   = 256;
		float LayersCount       = mix(MaxLayers, MinLayers, max(dot(vec3(0, 0, 1), ViewDir), 0.0));
		float LayersDepth       = 1.0 / LayersCount;
		float CurrentLayerDepth = 0.0;

		vec2  DeltaTextCoord   = ViewDir.xy * HeightScale * LayersDepth;
		vec2  CurrentTextCoord = In.TextCoord;
		float CurrentDepth     = 1.0 - texture(HeightSamplers[nonuniformEXT(HeightMapIdx)], CurrentTextCoord).r;
		while(CurrentLayerDepth < CurrentDepth)
		{
			CurrentTextCoord  -= DeltaTextCoord;
			CurrentDepth       = 1.0 - texture(HeightSamplers[nonuniformEXT(HeightMapIdx)], CurrentTextCoord).r;
			CurrentLayerDepth += LayersDepth;
		}

		vec2 PrevTextCoord = CurrentTextCoord + DeltaTextCoord;

		float AfterDepth  = CurrentDepth - CurrentLayerDepth;
		float BeforeDepth = 1.0 - texture(HeightSamplers[nonuniformEXT(HeightMapIdx)], PrevTextCoord).x - CurrentLayerDepth + LayersDepth;
		float Weight	  = AfterDepth / (AfterDepth - BeforeDepth);
		TextCoord         = mix(PrevTextCoord, CurrentTextCoord, Weight);

		if(TextCoord.x < 0.0 || TextCoord.y < 0.0 || TextCoord.x > 1.0 || TextCoord.y > 1.0) discard;
	}

	OutputFragmentNormal = OutputVertexNormal;
	if(MeshMaterials[nonuniformEXT(MatIdx)].HasNormalMap)
		OutputFragmentNormal = vec4((TBN * texture(NormalSamplers[nonuniformEXT(NormalMapIdx)], TextCoord).rgb) * 0.5 + 0.5, 0);
#if DEBUG_COLOR_BLEND
	OutputDiffuse		 = vec4(vec3(0.1) * In.Col.rgb, 1.0);
#else
	OutputDiffuse		 = In.Col;
	if(MeshMaterials[nonuniformEXT(MatIdx)].HasTexture)
		OutputDiffuse *= vec4(texture(DiffuseSamplers[nonuniformEXT(TextureIdx)], TextCoord).rgb, 1);
#endif
	OutputSpecular		 = 1.0;
	if(MeshMaterials[nonuniformEXT(MatIdx)].HasSpecularMap)
		OutputSpecular *= texture(SpecularSamplers[nonuniformEXT(SpecularMapIdx)], TextCoord).r;
}
