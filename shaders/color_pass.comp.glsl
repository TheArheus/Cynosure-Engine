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
	uint  DirectionalLightSourceCount;
	uint  PointLightSourceCount;
	uint  SpotLightSourceCount;
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

layout(set = 0, binding = 0) readonly uniform block1 { global_world_data WorldUpdate; };
layout(set = 0, binding = 1) uniform block2 { light_source LightSources[1024]; };
layout(set = 0, binding = 2) buffer  block3 { vec2 PoissonDisk[SAMPLES_COUNT]; };
layout(set = 0, binding = 3) uniform sampler3D RandomAnglesTexture;
layout(set = 0, binding = 4) uniform sampler2D GBuffer[GBUFFER_COUNT];
layout(set = 0, binding = 5) uniform sampler2D AmbientOcclusionBuffer;
layout(set = 0, binding = 6) uniform writeonly image2D ColorTarget;
layout(set = 0, binding = 7) uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
layout(set = 1, binding = 0) uniform sampler2D ShadowMaps[LIGHT_SOURCES_MAX_COUNT];
layout(set = 1, binding = 1) uniform samplerCube PointShadowMaps[LIGHT_SOURCES_MAX_COUNT];
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

vec4 GetQuat(vec3 V)
{
	return vec4(V, 0);
}
vec4 GetQuat(vec4 V)
{
	return vec4(V);
}
vec4 QuatInv(vec3 V)
{
	return vec4(V, 0);
}
vec4 QuatInv(vec4 V)
{
	return vec4(V);
}
vec4 QuatMul(vec4 lhs, vec4 rhs)
{
	return vec4(lhs.xyz * rhs.w + rhs.xyz * lhs.w + cross(lhs.xyz, rhs.xyz), dot(-lhs.xyz, rhs.xyz) + lhs.w * rhs.w);
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
 
	Result /= (SAMPLES_COUNT);
	return Result;
}

float GetPointLightShadow(samplerCube ShadowSampler, vec3 Position, vec3 LightPos, vec3 Normal, vec2 TextCoord)
{
    vec3  Direction = normalize(Position - LightPos) * vec3(1, -1, 1);
    float ShadowDepth = texture(ShadowSampler, Direction).r;
    float ObjectDepth = length(Position - LightPos) / WorldUpdate.FarZ;
    
	float  Bias = -0.05;
    return (ObjectDepth + Bias) > ShadowDepth ? 1.0 : 0.0;
}

vec3 DoDiffuse(vec3 LightCol, vec3 ToLightDir, vec3 Normal)
{
	float NdotL = max(dot(ToLightDir, Normal), 0);
	return LightCol * NdotL;
}

vec3 DoSpecular(vec3 LightCol, vec3 ToLightDir, vec3 CameraDir, vec3 Normal, float Specular)
{
	vec3  Refl = normalize(reflect(-ToLightDir, Normal));
	float BlinnTerm = max(dot(Refl, CameraDir), 0);
	return LightCol * pow(BlinnTerm, Specular);
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
	ToLightDir = normalize(ToLightDir);

	vec3 NewDiffuse   = DoDiffuse(LightCol, ToLightDir, Normal) * Intensity;
	vec3 NewSpecular  = DoSpecular(LightCol, ToLightDir, CameraDir, Normal, Specular) * Intensity;

	DiffuseResult  += NewDiffuse.rgb;
	SpecularResult += NewSpecular.rgb;
}

void PointLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  ToLightDir = LightPos - Coord;
	float Distance = length(ToLightDir);
	ToLightDir = normalize(ToLightDir);

	float Attenuation = DoAttenuation(Radius, Distance);
	vec3  NewDiffuse  = DoDiffuse(LightCol, ToLightDir, Normal) * Attenuation * Intensity;
	vec3  NewSpecular = DoSpecular(LightCol, ToLightDir, CameraDir, Normal, Specular) * Attenuation * Intensity;

	DiffuseResult  += NewDiffuse;
	SpecularResult += NewSpecular;
}

void SpotLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightView, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  LightPosDir = LightPos - Coord;
	float Distance = length(LightPosDir);

	float Attenuation   = DoAttenuation(Radius, Distance);
	float SpotIntensity = DoSpotCone(Radius, LightPosDir, LightView.xyz);
	vec3  NewDiffuse    = DoDiffuse(LightCol, LightPosDir, Normal) * Attenuation * SpotIntensity * Intensity;
	vec3  NewSpecular   = DoSpecular(LightCol, LightPosDir, CameraDir, Normal, Specular) * Attenuation * SpotIntensity * Intensity;

	DiffuseResult  += NewDiffuse;
	SpecularResult += NewSpecular;
}

void main()
{
	vec2 TextureDims = textureSize(GBuffer[0], 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y) {
        return;
    }

	vec4  CoordWS  = texelFetch(GBuffer[0], ivec2(TextCoord), 0);
	if((CoordWS.x == 0) && (CoordWS.y == 0) && (CoordWS.z == 0))
	{
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0), 1));
		return;
	}
	vec3  VertexNormalWS   = normalize(texelFetch(GBuffer[1], ivec2(TextCoord), 0).xyz);
	vec3  VertexNormalVS   = normalize((transpose(inverse(WorldUpdate.DebugView)) * vec4(VertexNormalWS, 1)).xyz);
	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[2], ivec2(TextCoord), 0).xyz);
	vec3  FragmentNormalVS = normalize((transpose(inverse(WorldUpdate.DebugView)) * vec4(FragmentNormalWS, 1)).xyz);
	vec4  Diffuse  = texelFetch(GBuffer[3], ivec2(TextCoord), 0);
	float Specular = texelFetch(GBuffer[4], ivec2(TextCoord), 0).x;
	float AmbientOcclusion = texelFetch(AmbientOcclusionBuffer, ivec2(TextCoord), 0).r;

	vec3  CoordVS  = (WorldUpdate.DebugView * CoordWS).xyz;
	vec3  RotationAngles = texture(RandomAnglesTexture, CoordVS / 32).xyz;
	vec2  Rotation2D = vec2(cos(RotationAngles.x), sin(RotationAngles.y));
	vec3  Rotation3D = vec3(cos(RotationAngles.x)*sin(RotationAngles.y), sin(RotationAngles.x)*sin(RotationAngles.y), cos(RotationAngles.y));

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
	uint TotalLightSourceCount = WorldUpdate.DirectionalLightSourceCount + 
								 WorldUpdate.PointLightSourceCount + 
								 WorldUpdate.SpotLightSourceCount;
	float LightShadow = 0.0;
	uint  DirLightShadowMapIdx = 0;
	uint  PointLightShadowMapIdx = 0;
	uint  SpotLightShadowMapIdx = 0;
	for(uint LightIdx = 0;
		LightIdx < TotalLightSourceCount;
		++LightIdx)
	{
		vec3 LightSourcePosWS = LightSources[LightIdx].Pos.xyz;
		vec3 LightSourcePosVS = (WorldUpdate.DebugView * vec4(LightSourcePosWS, 1)).xyz;
		vec3 LightSourceDirVS = (WorldUpdate.DebugView * vec4(LightSources[LightIdx].Dir.xyz, 1)).xyz;
		vec3 LightSourceCol   = LightSources[LightIdx].Col.xyz;
		float Intensity       = LightSources[LightIdx].Col.w;
		float Radius          = LightSources[LightIdx].Pos.w;

		if(LightSources[LightIdx].Type == light_type_directional)
		{
			DirectionalLight(LightDiffuse, LightSpecular, FragmentNormalVS, ViewDirVS, LightSourceDirVS, LightSourceCol, Intensity, Diffuse.xyz, Specular);
			DirLightShadowMapIdx++;
		}
		else if(LightSources[LightIdx].Type == light_type_point)
		{
			PointLight(LightDiffuse, LightSpecular, CoordVS.xyz, FragmentNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightSourceCol, Intensity, Diffuse.xyz, Specular);
			LightShadow += GetPointLightShadow(PointShadowMaps[PointLightShadowMapIdx], vec3(CoordWS), LightSourcePosWS, FragmentNormalWS, TextCoord/TextureDims);
			PointLightShadowMapIdx++;
		}
		else if(LightSources[LightIdx].Type == light_type_spot)
		{
			SpotLight(LightDiffuse, LightSpecular, CoordVS.xyz, FragmentNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightSourceDirVS, LightSourceCol, Intensity, Diffuse.xyz, Specular);
			SpotLightShadowMapIdx++;
		}
	}
	LightShadow /= float(TotalLightSourceCount);

	uint  Layer = 0;
	float GlobalShadow = 0.0;
	vec3  CascadeCol;
	for(uint CascadeIdx = 1;
		CascadeIdx <= DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		if(abs(CoordVS.z) < CascadeSplits[CascadeIdx])
		{
			Layer  = CascadeIdx - 1;
			GlobalShadow = GetShadow(ShadowMap[Layer], ShadowPos[Layer], Rotation2D, CascadeSplits[CascadeIdx - 1], VertexNormalWS, GlobalLightPos.xyz);
			CascadeCol = CascadeColors[Layer];

			float Fade = clamp((1.0 - (CoordVS.z * CoordVS.z) / (CascadeSplits[CascadeIdx] * CascadeSplits[CascadeIdx])) / 0.2, 0.0, 1.0);
			if(Fade > 0.0 && Fade < 1.0)
			{
				float NextShadow = GetShadow(ShadowMap[Layer + 1], ShadowPos[Layer + 1], Rotation2D, CascadeSplits[CascadeIdx], VertexNormalWS, GlobalLightPos.xyz);
				vec3  NextCascadeCol = CascadeColors[Layer + 1];
				GlobalShadow = mix(NextShadow, GlobalShadow, Fade);
				CascadeCol = mix(NextCascadeCol, CascadeCol, Fade);
			}

			break;
		}
	}

	vec3 ShadowCol = vec3(0.0001);

	float Shadow = (GlobalShadow + LightShadow) / 2.0;
#if DEBUG_COLOR_BLEND
	imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(Diffuse.xyz, 1), vec4(1.0 / 2.0)));
#else
	if(WorldUpdate.DebugColors)
	{
		vec4 FinalLight = vec4(Diffuse.xyz*0.02 + (1.0 - Shadow), 1.0) * vec4(CascadeCol, 1.0);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(FinalLight, vec4(1.0 / 2.0)));
	}
	else
	{
		vec3 FinalLight = (Diffuse.xyz*0.2 + LightDiffuse + LightSpecular) * AmbientOcclusion;
		FinalLight += mix(ShadowCol, FinalLight, 1.0 - Shadow);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(FinalLight, 1), vec4(1.0 / 2.0)));
		//imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(LightShadow), 1));
	}
#endif
}
