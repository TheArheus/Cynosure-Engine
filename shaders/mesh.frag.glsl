#version 450

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

struct vert_in
{
	vec4 Coord;
	vec4 Norm;
	vec4 Col;
	vec2 TextCoord;
};

layout(binding = 1) readonly uniform block1 { global_world_data WorldUpdate; };
layout(binding = 3) uniform sampler2D DiffuseSampler;
layout(binding = 4) uniform sampler2D NormalSampler;
layout(binding = 5) uniform sampler2D HeightSampler;

layout(location = 0) in vert_in In;
layout(location = 4) in mat3    TBN;

layout(location = 0) out vec4  OutputPosition;
layout(location = 1) out vec4  OutputVertexNormal;
layout(location = 2) out vec4  OutputFragmentNormal;
layout(location = 3) out vec4  OutputDiffuse;
layout(location = 4) out float OutputSpecular;


void main()
{
	vec3  ViewDir = normalize(TBN * (WorldUpdate.CameraPos - In.Coord).xyz);
	float HeightScale = 0.01;

	const float MinLayers   = 8 ;
	const float MaxLayers   = 64;
	float LayersCount       = mix(MaxLayers, MinLayers, abs(dot(vec3(0, 0, 1), ViewDir)));
	float LayersDepth       = 1.0 / LayersCount;
	float CurrentLayerDepth = 0.0;

	vec2  DeltaTextCoord   = (ViewDir.xy * HeightScale) / (LayersCount);
	vec2  CurrentTextCoord = In.TextCoord;
	float CurrentDepth     = 1.0 - texture(HeightSampler, CurrentTextCoord).x;
	while(CurrentLayerDepth < CurrentDepth)
	{
		CurrentTextCoord  -= DeltaTextCoord;
		CurrentDepth       = 1.0 - texture(HeightSampler, CurrentTextCoord).x;
		CurrentLayerDepth += LayersDepth;
	}

	vec2 PrevTextCoord = CurrentTextCoord + DeltaTextCoord;

	float AfterDepth  = CurrentDepth - CurrentLayerDepth;
	float BeforeDepth = 1.0 - texture(HeightSampler, CurrentTextCoord).x - CurrentLayerDepth + LayersDepth;
	vec2  TextCoord   = mix(PrevTextCoord, CurrentTextCoord, AfterDepth / (AfterDepth - BeforeDepth));

	if(TextCoord.x < 0.0 || TextCoord.y < 0.0 || TextCoord.x > 1.0 || TextCoord.y > 1.0) discard;

	OutputPosition       = In.Coord;
	OutputVertexNormal   = In.Norm;
	OutputFragmentNormal = vec4(TBN * (texture(NormalSampler, TextCoord).xyz * 2.0 - 1.0), 0);

#if DEBUG_COLOR_BLEND
	OutputDiffuse  = vec4(vec3(0.1), 1.0);
#else
	OutputDiffuse  = In.Col * vec4(texture(DiffuseSampler, TextCoord).xyz, 1);
#endif

	OutputSpecular = 2.0f;
}
