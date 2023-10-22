#version 450

#define SAMPLES_COUNT 64
#define light_type_none 0
#define light_type_directional 1
#define light_type_point 2
#define light_type_spot 3

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;


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

struct light_source
{
	vec4 Pos;
	vec4 Dir;
	vec4 Col;
	uint Type;
};

layout(binding = 0)   readonly uniform block1 { global_world_data WorldUpdate; };
layout(binding = 1)   uniform block2 { light_source LightSources[1024]; };
layout(binding = 2)   buffer  block3 { vec2 PoissonDisk[SAMPLES_COUNT]; };
layout(binding = 3)   uniform sampler3D RandomAnglesTexture;
layout(binding = 4)   uniform sampler2D VertexPosBuffer;
layout(binding = 5)   uniform sampler2D VertexNormalBuffer;
layout(binding = 6)   uniform sampler2D FragmentNormalBuffer;
layout(binding = 7)   uniform sampler2D DiffuseBuffer;
layout(binding = 8)   uniform sampler2D SpecularBuffer;
layout(binding = 9)   uniform sampler2D AmbientOcclusionBuffer;
layout(binding = 10)  uniform writeonly image2D ColorTarget;
layout(binding = 11)  uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
layout(push_constant) uniform pushConstant { float CascadeSplits[DEPTH_CASCADES_COUNT + 1]; };

float GetRandomValue(vec2 Seed)
{
    float  RandomValue = fract(sin(dot(Seed, vec2(12.9898, 78.233))) * 43758.5453);
    return RandomValue;
}

float GetSearchWidth(float ReceiverDepth, float Near)
{
    return WorldUpdate.GlobalLightSize * (ReceiverDepth - Near) / WorldUpdate.CameraPos.z;
}

vec3 WorldPosFromDepth(vec2 TextCoord, float Depth) {
	TextCoord.y = 1 - TextCoord.y;
    vec4 ClipSpacePosition = vec4(TextCoord * 2.0 - 1.0, Depth, 1.0);
    vec4 ViewSpacePosition = inverse(WorldUpdate.Proj * WorldUpdate.DebugView) * ClipSpacePosition;
    ViewSpacePosition /= ViewSpacePosition.w;

    return ViewSpacePosition.xyz;
}

vec3 ViewPosFromDepth(vec2 TextCoord, float Depth) {
	TextCoord.y = 1 - TextCoord.y;
    vec4 ClipSpacePosition = vec4(TextCoord * 2.0 - 1.0, Depth, 1.0);
    vec4 ViewSpacePosition = inverse(WorldUpdate.Proj) * ClipSpacePosition;
    ViewSpacePosition /= ViewSpacePosition.w;

    return ViewSpacePosition.xyz;
}
 
float GetBlockerDistance(sampler2D ShadowSampler, vec2 ShadowCoord, vec2 Rotation, float ReceiverDepth, float Bias, float Near)
{
    float SumBlockerDistances = 0.0f;
    int   NumBlockerDistances = 0;
 
	vec2 TextureSize = 1.0f / textureSize(ShadowSampler, 0);
    int sw = int(GetSearchWidth(ReceiverDepth, Near));
    for (int SampleIdx = 0; SampleIdx < SAMPLES_COUNT; ++SampleIdx)
    {
		vec2 Offset = vec2(Rotation.x * PoissonDisk[SampleIdx].x - Rotation.y * PoissonDisk[SampleIdx].y,
						   Rotation.y * PoissonDisk[SampleIdx].x + Rotation.x * PoissonDisk[SampleIdx].y);
 
		float Depth = texture(ShadowSampler, ShadowCoord + Offset*TextureSize*sw).r;
        if (Depth < ReceiverDepth + Bias)
        {
            ++NumBlockerDistances;
            SumBlockerDistances += Depth;
        }
    }
 
    if (NumBlockerDistances > 0)
    {
        return SumBlockerDistances / NumBlockerDistances;
    }
    else
    {
        return -1;
    }
}

float GetPCFKernelSize(sampler2D ShadowSampler, vec2 ShadowCoord, vec2 Rotation, float ReceiverDepth, float Bias, float Near)
{
    float BlockerDistance = GetBlockerDistance(ShadowSampler, ShadowCoord, Rotation, ReceiverDepth, Bias, Near);

    if (BlockerDistance == -1)
    {
        return 1;
    }
 
    float  PenumbraWidth = (ReceiverDepth - BlockerDistance) / BlockerDistance;
    return PenumbraWidth * WorldUpdate.GlobalLightSize * Near / ReceiverDepth;
}

float GetShadow(sampler2D ShadowSampler, vec4 PositionInLightSpace, vec2 Rotation, float Near, vec3 Normal, vec3 CurrLightPos)
{
	vec3  LightPos = CurrLightPos * 5000000;
	float Bias = dot(LightPos, Normal) > 0 ? -0.005 : 0.2;

	vec2   TextureSize = 1.0f / textureSize(ShadowSampler, 0);
	vec3   ProjPos = PositionInLightSpace.xyz / PositionInLightSpace.w;
	float  ObjectDepth = ProjPos.z;
	vec2   ShadowCoord = ProjPos.xy * vec2(0.5, -0.5) + 0.5;

	float Result = 0.0;

	if(Near < 1) Near = 1;
	float KernelSize   = GetPCFKernelSize(ShadowSampler, ShadowCoord, Rotation, ObjectDepth, Bias, Near);
	for(uint SampleIdx = 0; SampleIdx < SAMPLES_COUNT; SampleIdx++)
	{
		vec2 Offset = vec2(Rotation.x * PoissonDisk[SampleIdx].x - Rotation.y * PoissonDisk[SampleIdx].y,
						   Rotation.y * PoissonDisk[SampleIdx].x + Rotation.x * PoissonDisk[SampleIdx].y);

		float ShadowDepth = texture(ShadowSampler, ShadowCoord + Offset*TextureSize*KernelSize).r;
		Result += (ObjectDepth + Bias) > ShadowDepth ? 1.0 : 0.0;
	}
	float l = clamp(smoothstep(0.0f, 0.2f, dot(LightPos, Normal)), 0.0f, 1.0f);
	float t = smoothstep(GetRandomValue(gl_GlobalInvocationID.xy / vec2(WorldUpdate.ScreenWidth, WorldUpdate.ScreenHeight)) * 0.5f, 1.0f, l);
 
	//Result /= (SAMPLES_COUNT * t);
	Result /= (SAMPLES_COUNT);
	return Result;
}

vec3 DoDiffuse(vec3 LightCol, vec3 ToLightDir, vec3 Normal)
{
	float NdotL = max(dot(ToLightDir, Normal), 0);
	return LightCol * NdotL;
}

vec3 DoSpecular(vec3 LightCol, vec3 ToLightDir, vec3 CameraDir, vec3 Normal, float Specular)
{
	vec3  HalfA = normalize(ToLightDir + CameraDir);
	float BlinnTerm = max(dot(Normal, HalfA), 0);
	BlinnTerm = pow(BlinnTerm, Specular);

	return LightCol * BlinnTerm;
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

void DirectionalLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Normal, vec3 CameraDir, vec3 LightDir, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  ToLightDir = LightDir;
	float Distance = length(ToLightDir);
	ToLightDir /= Distance;

	vec3 NewDiffuse   = DoDiffuse(LightCol, ToLightDir, Normal) * Intensity;
	vec3 NewSpecular  = DoSpecular(LightCol, ToLightDir, CameraDir, Normal, Specular) * Intensity;

	DiffuseResult  += NewDiffuse.rgb;
	SpecularResult += NewSpecular.rgb;
}

void PointLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  ToLightDir = LightPos - Coord;
	float Distance = length(ToLightDir);
	ToLightDir /= Distance;

	float Attenuation = DoAttenuation(Radius, Distance);
	vec3 NewDiffuse   = DoDiffuse(LightCol, ToLightDir, Normal) * Attenuation * Intensity;
	vec3 NewSpecular  = DoSpecular(LightCol, ToLightDir, CameraDir, Normal, Specular) * Attenuation * Intensity;

	DiffuseResult  += NewDiffuse;
	SpecularResult += NewSpecular;
}

void SpotLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightView, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  LightPosDir = LightPos - Coord;
	float Distance = dot(LightPosDir, LightPosDir);

	float Attenuation   = DoAttenuation(Radius, Distance);
	float SpotIntensity = DoSpotCone(Radius, LightPosDir, LightView.xyz);
	vec3  NewDiffuse    = DoDiffuse(LightCol, LightPosDir, Normal) * Attenuation * SpotIntensity * Intensity;
	vec3  NewSpecular   = DoSpecular(LightCol, LightPosDir, CameraDir, Normal, Specular) * Attenuation * SpotIntensity * Intensity;

	DiffuseResult  += NewDiffuse;
	SpecularResult += NewSpecular;
}

void main()
{
	vec2 TextureDims = textureSize(VertexPosBuffer, 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y) {
        return;
    }

	vec4  CoordWS  = texelFetch(VertexPosBuffer, ivec2(TextCoord), 0);
	vec3  VertexNormal     = texelFetch(VertexNormalBuffer, ivec2(TextCoord), 0).xyz;
	vec3  FragmentNormalWS = normalize(texelFetch(FragmentNormalBuffer, ivec2(TextCoord), 0).xyz);
	vec3  FragmentNormalVS = normalize((transpose(inverse(WorldUpdate.DebugView)) * vec4(FragmentNormalWS, 1)).xyz);
	vec4  Diffuse  = texelFetch(DiffuseBuffer, ivec2(TextCoord), 0);
	float Specular = texelFetch(SpecularBuffer, ivec2(TextCoord), 0).x;
	float AmbientOcclusion = texelFetch(AmbientOcclusionBuffer, ivec2(TextCoord), 0).r;

	vec3  CoordVS  = (WorldUpdate.DebugView * CoordWS).xyz;
	vec2  Rotation = texture(RandomAnglesTexture, CoordVS / 32).xy;

	vec4 ShadowPos[4];
	for(uint CascadeIdx = 0;
		CascadeIdx < DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		ShadowPos[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * CoordWS;
	}

	vec3 CascadeColors[] = 
	{
		vec3(1, 0, 0),
		vec3(0, 1, 0),
		vec3(0, 0, 1),
		vec3(1, 1, 0),
		vec3(1, 0, 1),
		vec3(0, 1, 1),
		vec3(1, 1, 1),
	};

	vec4 GlobalLightPos = WorldUpdate.GlobalLightPos;
	vec4 GlobalLightDir = normalize(-WorldUpdate.GlobalLightPos);

	vec3 ViewDirVS = -normalize(CoordVS);

	vec3 LightDiffuse   = vec3(0);
	vec3 LightSpecular  = vec3(0);
	for(uint LightIdx = 0;
		LightIdx < WorldUpdate.ColorSourceCount;
		++LightIdx)
	{
		vec3 LightSourcePosVS = (WorldUpdate.DebugView * vec4(LightSources[LightIdx].Pos.xyz, 1)).xyz;
		vec3 LightSourceDirVS = (WorldUpdate.DebugView * vec4(LightSources[LightIdx].Dir.xyz, 1)).xyz;
		vec3 LightSourceCol   = LightSources[LightIdx].Col.xyz;
		float Intensity       = LightSources[LightIdx].Col.w;
		float Radius          = LightSources[LightIdx].Pos.w;

		if(LightSources[LightIdx].Type == light_type_directional)
			DirectionalLight(LightDiffuse, LightSpecular, FragmentNormalVS, ViewDirVS, LightSourceDirVS, LightSourceCol, Intensity, Diffuse.xyz, Specular);
		else if(LightSources[LightIdx].Type == light_type_point)
			PointLight(LightDiffuse, LightSpecular, CoordVS.xyz, FragmentNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightSourceCol, Intensity, Diffuse.xyz, Specular);
		else if(LightSources[LightIdx].Type == light_type_spot)
			SpotLight(LightDiffuse, LightSpecular, CoordVS.xyz, FragmentNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightSourceDirVS, LightSourceCol, Intensity, Diffuse.xyz, Specular);
	}

	uint  Layer = 0;
	float Shadow = 0.0;
	vec3  CascadeCol;
	for(uint CascadeIdx = 1;
		CascadeIdx <= DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		if(abs(CoordVS.z) < CascadeSplits[CascadeIdx])
		{
			Layer  = CascadeIdx - 1;
			Shadow = GetShadow(ShadowMap[Layer], ShadowPos[Layer], Rotation, CascadeSplits[CascadeIdx - 1], VertexNormal, GlobalLightPos.xyz);
			CascadeCol = CascadeColors[Layer];

			float Fade = clamp((1.0 - (CoordVS.z * CoordVS.z) / (CascadeSplits[CascadeIdx] * CascadeSplits[CascadeIdx])) / 0.2, 0.0, 1.0);
			if(Fade > 0.0 && Fade < 1.0)
			{
				float NextShadow = GetShadow(ShadowMap[Layer + 1], ShadowPos[Layer + 1], Rotation, CascadeSplits[CascadeIdx], VertexNormal, GlobalLightPos.xyz);
				vec3  NextCascadeCol = CascadeColors[Layer + 1];
				Shadow = mix(NextShadow, Shadow, Fade);
				CascadeCol = mix(NextCascadeCol, CascadeCol, Fade);
			}

			break;
		}
	}

	vec3 ShadowCol = vec3(0.0001);

#if DEBUG_COLOR_BLEND
	imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(Diffuse.xyz, 1), vec4(1.0 / 2.0)));
#else
	if(WorldUpdate.DebugColors)
	{
		vec4 FinalLight = vec4(Diffuse.xyz*0.2 + (1.0 - Shadow), 1.0) * vec4(CascadeCol, 1.0);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(FinalLight, vec4(1.0 / 2.0)));
	}
	else
	{
		vec3 FinalLight = (Diffuse.xyz*0.2 + LightDiffuse + LightSpecular)*AmbientOcclusion;
		FinalLight += mix(ShadowCol, FinalLight, 1.0 - Shadow);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(FinalLight, 1), vec4(1.0 / 2.0)));
	}
#endif
}
