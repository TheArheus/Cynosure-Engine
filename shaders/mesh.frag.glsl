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
layout(binding = 5) uniform sampler2D SpecularSampler;
layout(binding = 6) uniform sampler2D HeightSampler;

layout(location = 0) in vert_in In;
layout(location = 4) in mat3    TBN;

layout(location = 0) out vec4  OutputVertexPosition;
layout(location = 1) out vec4  OutputVertexNormal;
layout(location = 2) out vec4  OutputFragmentNormal;
layout(location = 3) out vec4  OutputDiffuse;
layout(location = 4) out float OutputSpecular;


// TODO: Parallax Mapping only for a close objects
void main()
{
	OutputVertexPosition = In.Coord;
	OutputVertexNormal   = In.Norm * 0.5 + 0.5;

	vec2 TextCoord = In.TextCoord;
	vec3 ViewDir = (WorldUpdate.CameraPos - In.Coord).xyz;
	if(length(ViewDir) < 6)
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
		float CurrentDepth     = 1.0 - texture(HeightSampler, CurrentTextCoord).r;
		while(CurrentLayerDepth < CurrentDepth)
		{
			CurrentTextCoord  -= DeltaTextCoord;
			CurrentDepth       = 1.0 - texture(HeightSampler, CurrentTextCoord).r;
			CurrentLayerDepth += LayersDepth;
		}

		vec2 PrevTextCoord = CurrentTextCoord + DeltaTextCoord;

		float AfterDepth  = CurrentDepth - CurrentLayerDepth;
		float BeforeDepth = 1.0 - texture(HeightSampler, PrevTextCoord).x - CurrentLayerDepth + LayersDepth;
		float Weight	  = AfterDepth / (AfterDepth - BeforeDepth);
		TextCoord   = mix(PrevTextCoord, CurrentTextCoord, Weight);

		if(TextCoord.x < 0.0 || TextCoord.y < 0.0 || TextCoord.x > 1.0 || TextCoord.y > 1.0) discard;
	}

	OutputFragmentNormal = vec4((TBN * texture(NormalSampler, TextCoord).rgb) * 0.5 + 0.5, 0);
#if DEBUG_COLOR_BLEND
	OutputDiffuse		 = vec4(vec3(0.1), 1.0);
#else
	OutputDiffuse		 = In.Col * vec4(texture(DiffuseSampler, TextCoord).rgb, 1);
#endif
	OutputSpecular		 = texture(SpecularSampler, TextCoord).r;
}
