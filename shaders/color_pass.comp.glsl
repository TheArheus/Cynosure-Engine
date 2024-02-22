#version 450

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_nonuniform_qualifier: require

#define SAMPLES_COUNT 16
#define light_type_none 0
#define light_type_directional 1
#define light_type_point 2
#define light_type_spot 3

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;


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

struct light_source
{
	vec4 Pos;
	vec4 Dir;
	vec4 Col;
	uint Type;
};

layout(set = 0, binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };
layout(set = 0, binding = 1) readonly buffer b1 { light_source LightSources[LIGHT_SOURCES_MAX_COUNT]; };
layout(set = 0, binding = 2) readonly buffer b2 { vec2 PoissonDisk[SAMPLES_COUNT]; };
layout(set = 0, binding = 3) uniform sampler3D RandomAnglesTexture;
layout(set = 0, binding = 4) uniform sampler2D GBuffer[GBUFFER_COUNT];
layout(set = 0, binding = 5) uniform writeonly image2D ColorTarget;

layout(set = 1, binding = 0) uniform sampler2D AmbientOcclusionBuffer;
layout(set = 2, binding = 0) uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
layout(set = 3, binding = 0) uniform sampler2D ShadowMaps[LIGHT_SOURCES_MAX_COUNT];
layout(set = 4, binding = 0) uniform samplerCube PointShadowMaps[LIGHT_SOURCES_MAX_COUNT];

float GetRandomValue(vec2 Seed)
{
    float  RandomValue = fract(sin(dot(Seed, vec2(12.9898, 78.233))) * 43758.5453);
    return RandomValue;
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

vec4 QuatMul(vec4 lhs, vec4 rhs)
{
	return vec4(lhs.xyz * rhs.w + rhs.xyz * lhs.w + cross(lhs.xyz, rhs.xyz), dot(-lhs.xyz, rhs.xyz) + lhs.w * rhs.w);
}

float GetSearchWidth(float LightSize, float ReceiverDepth, float Near)
{
    return LightSize * (ReceiverDepth - Near) / ReceiverDepth;
}
 
float GetBlockerDistance(sampler2D ShadowSampler, vec2 ShadowCoord, vec2 Rotation, float ReceiverDepth, float Bias, float Near, float LightSize)
{
    float SumBlockerDistances = 0.0f;
    int   NumBlockerDistances = 0;
 
    float SearchWidth  = GetSearchWidth(LightSize, ReceiverDepth, Near);
    for (int SampleIdx = 0; SampleIdx < SAMPLES_COUNT; ++SampleIdx)
    {
		vec2 Offset = vec2(Rotation.x * PoissonDisk[SampleIdx].x - Rotation.y * PoissonDisk[SampleIdx].y,
						   Rotation.y * PoissonDisk[SampleIdx].x + Rotation.x * PoissonDisk[SampleIdx].y);
 
		float Depth = texture(ShadowSampler, ShadowCoord + Offset*SearchWidth).r;
        if ((Depth + Bias) > ReceiverDepth)
        {
            NumBlockerDistances += 1;
            SumBlockerDistances += Depth;
        }
    }
 
    if (NumBlockerDistances > 0)
    {
        return SumBlockerDistances / NumBlockerDistances;
    }
	return -1;
}

float GetPenumbraSize(sampler2D ShadowSampler, vec2 ShadowCoord, vec2 Rotation, float ReceiverDepth, float Bias, float Near, float LightSize)
{
    float BlockerDistance = GetBlockerDistance(ShadowSampler, ShadowCoord, Rotation, ReceiverDepth, Bias, Near, LightSize);

    if (BlockerDistance <= -1.0)
    {
        return 1.0;
    }
 
    float  PenumbraWidth = (ReceiverDepth - BlockerDistance) / BlockerDistance;
    return PenumbraWidth * LightSize * Near / ReceiverDepth;
}

float GetShadow(sampler2D ShadowSampler, vec4 PositionInLightSpace, vec2 Rotation, float Near, vec3 Normal, vec3 CurrLightPos, float LightSize)
{
	vec3  LightPos = CurrLightPos * 5000000;
	float Bias = dot(LightPos, Normal) > 0 ? -0.005 : 0.2;

	vec3  ProjPos = PositionInLightSpace.xyz / PositionInLightSpace.w;
	float ObjectDepth = ProjPos.z;
	vec2  ShadowCoord = ProjPos.xy * vec2(0.5, -0.5) + 0.5;

	float Result = 0.0;

	float SearchSize   = GetPenumbraSize(ShadowSampler, ShadowCoord, Rotation, ObjectDepth, Bias, Near, LightSize);
	if(SearchSize == 1.0) SearchSize = 3.402823466e-38;
	for(uint SampleIdx = 0; SampleIdx < SAMPLES_COUNT; SampleIdx++)
	{
		vec2 Offset = vec2(Rotation.x * PoissonDisk[SampleIdx].x - Rotation.y * PoissonDisk[SampleIdx].y,
						   Rotation.y * PoissonDisk[SampleIdx].x + Rotation.x * PoissonDisk[SampleIdx].y);

		float ShadowDepth = texture(ShadowSampler, ShadowCoord + Offset*SearchSize).r;
		Result += (ObjectDepth + Bias) > ShadowDepth ? 1.0 : 0.0;
	}
 
	Result /= (SAMPLES_COUNT);
	return Result;
}
 
float GetPointBlockerDistance(samplerCube ShadowSampler, vec3 Direction, vec2 Rotation, float ReceiverDepth, float Bias, float Near, float LightSize)
{
    float SumBlockerDistances = 0.0f;
    int   NumBlockerDistances = 0;

    float SearchWidth  = GetSearchWidth(LightSize, ReceiverDepth, Near);
	for(uint SampleIdx = 0; SampleIdx < SAMPLES_COUNT; SampleIdx++)
	{
		vec2 Offset = vec2(Rotation.x * PoissonDisk[SampleIdx].x - Rotation.y * PoissonDisk[SampleIdx].y,
						   Rotation.y * PoissonDisk[SampleIdx].x + Rotation.x * PoissonDisk[SampleIdx].y);

		float Depth = texture(ShadowSampler, normalize(Direction + vec3(Offset.x*SearchWidth, Offset.y*SearchWidth, 1))).r;
		if((Depth + Bias) > ReceiverDepth)
		{
            NumBlockerDistances += 1;
            SumBlockerDistances += Depth;
		}
	}
 
    if (NumBlockerDistances > 0)
    {
        return SumBlockerDistances / NumBlockerDistances;
    }
	return -1;
}

float GetPointPenumbraSize(samplerCube ShadowSampler, vec3 Direction, vec2 Rotation, float ReceiverDepth, float Bias, float Near, float LightSize)
{
    float BlockerDistance = GetPointBlockerDistance(ShadowSampler, Direction, Rotation, ReceiverDepth, Bias, Near, LightSize);

    if (BlockerDistance <= -1.0)
    {
        return 1.0;
    }
 
    float  PenumbraWidth = (ReceiverDepth - BlockerDistance) / BlockerDistance;
    return PenumbraWidth * LightSize * Near / ReceiverDepth;
}

float GetPointLightShadow(samplerCube ShadowSampler, vec3 Position, vec3 LightPos, vec3 Normal, vec2 TextCoord, vec2 Rotation, float Near, float LightSize)
{
    vec3  Direction = normalize(Position - LightPos) * vec3(1, -1, 1);
	float ObjectDepth = length(Position - LightPos) / WorldUpdate.FarZ;
	float Bias = -0.01;
	float Result = 0.0;

	float SearchSize   = GetPointPenumbraSize(ShadowSampler, Direction, Rotation, ObjectDepth, Bias, Near, LightSize);
	if(SearchSize == 1.0) SearchSize = 3.402823466e-38;
	for(uint SampleIdx = 0; SampleIdx < SAMPLES_COUNT; SampleIdx++)
	{
		vec2 Offset = vec2(Rotation.x * PoissonDisk[SampleIdx].x - Rotation.y * PoissonDisk[SampleIdx].y,
						   Rotation.y * PoissonDisk[SampleIdx].x + Rotation.x * PoissonDisk[SampleIdx].y);

		float ShadowDepth = texture(ShadowSampler, normalize(Direction + vec3(Offset.x*SearchSize, Offset.y*SearchSize, 1))).r;
		Result += (ObjectDepth + Bias) > ShadowDepth ? 1.0 : 0.0;
	}
 
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
#if 0
	vec3  Refl  = normalize(reflect(-ToLightDir, Normal));
	float BlinnTerm = max(dot(Refl, CameraDir), 0);
	return LightCol * pow(BlinnTerm, Specular) * float(dot(ToLightDir, Normal) > 0);
#else
	vec3 Half = normalize(ToLightDir + CameraDir);
	return LightCol * pow(max(dot(Half, Normal), 0), Specular) * float(dot(ToLightDir, Normal) > 0);
#endif
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

	vec3 NewDiffuse  = DoDiffuse(LightCol, ToLightDir, Normal) * Intensity;
	vec3 NewSpecular = DoSpecular(LightCol, ToLightDir, CameraDir, Normal, Specular) * Intensity;

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
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	vec4 CoordWS  = texelFetch(GBuffer[0], ivec2(TextCoord), 0);
	vec3 CoordVS  = (WorldUpdate.DebugView * CoordWS).xyz;
	if((CoordWS.x == 0) && (CoordWS.y == 0) && (CoordWS.z == 0))
	{
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0), 1));
		return;
	}

	float Specular         = texelFetch(GBuffer[4], ivec2(gl_GlobalInvocationID.xy), 0).x;
	vec3  VertexNormalWS   = normalize(texelFetch(GBuffer[1], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  VertexNormalVS   = normalize(inverse(transpose(mat3(WorldUpdate.DebugView))) * VertexNormalWS).xyz;
	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[2], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  FragmentNormalVS = normalize(inverse(transpose(mat3(WorldUpdate.DebugView))) * FragmentNormalWS).xyz;

	vec2 ReflTextCoord = TextCoord;
	{
		float TotalDist = 0;
        float FltMin = 0.001;

		vec2 TexelViewDirUV = (TextCoord / TextureDims - 0.5) * 2.0;
		vec4 TexelViewDirDP = inverse(WorldUpdate.DebugView) * inverse(WorldUpdate.Proj) * vec4(vec3(TexelViewDirUV, 1), 1);
		vec3 TexelViewDirVP = TexelViewDirDP.xyz / TexelViewDirDP.w;

        vec3 RayO  = CoordWS.xyz;
        vec3 RayD  = reflect(TexelViewDirVP - CoordWS.xyz, FragmentNormalVS);
        for(uint i = 0; i < 32; i++)
        {
            vec3 CurrentRayVS = RayO + TotalDist * RayD;
            vec4 CurrentRaySP = WorldUpdate.Proj * WorldUpdate.DebugView * vec4(CurrentRayVS, 1);
            vec2 CurrentRayUV = vec2(CurrentRaySP.xyz / CurrentRaySP.w) * 0.5 + 0.5;

            vec4 SampledPosVS = texture(GBuffer[0], CurrentRayUV);
			//SampledPosVS = WorldUpdate.DebugView * SampledPosVS;

            float Dist = distance(SampledPosVS.xyz, CurrentRayVS);
            TotalDist += Dist;
            if(Dist <= FltMin || TotalDist > WorldUpdate.FarZ) 
            { 
				ReflTextCoord = CurrentRayUV * TextureDims;
				break;
            }
        }
	}

	vec4  Diffuse  = texelFetch(GBuffer[3], ivec2(ReflTextCoord), 0);
	      Specular = texelFetch(GBuffer[4], ivec2(ReflTextCoord), 0).x;
	float AmbientOcclusion = texelFetch(AmbientOcclusionBuffer, ivec2(ReflTextCoord), 0).r;

	vec3  RotationAngles = texture(RandomAnglesTexture, CoordVS / 32).xyz;
	vec2  Rotation2D = vec2(cos(RotationAngles.x), sin(RotationAngles.y));
	vec3  Rotation3D = vec3(cos(RotationAngles.x)*sin(RotationAngles.y), sin(RotationAngles.x)*sin(RotationAngles.y), cos(RotationAngles.y));

	vec4 ShadowPos[4];
	vec4 CameraPosLS[4];
	for(uint CascadeIdx = 0;
		CascadeIdx < DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		ShadowPos[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * CoordWS;
		CameraPosLS[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * WorldUpdate.CameraPos;
	}

	vec3 GlobalLightPosWS = WorldUpdate.GlobalLightPos.xyz;
	vec3 GlobalLightDirWS = normalize(-WorldUpdate.GlobalLightPos.xyz);
	vec3 GlobalLightPosVS = (WorldUpdate.DebugView * vec4(GlobalLightPosWS, 1)).xyz;
	vec3 GlobalLightDirVS = (WorldUpdate.DebugView * vec4(GlobalLightDirWS, 1)).xyz;

	vec3 ViewDirVS = -normalize(CoordVS);

	vec3 LightDiffuse   = vec3(0);
	vec3 LightSpecular  = vec3(0);
	uint TotalLightSourceCount = WorldUpdate.PointLightSourceCount + 
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

		if(LightSources[LightIdx].Type == light_type_point)
		{
			PointLight(LightDiffuse, LightSpecular, CoordVS.xyz, VertexNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightSourceCol, Intensity, Diffuse.xyz, Specular);
			if(WorldUpdate.LightSourceShadowsEnabled)
			{
				LightShadow += GetPointLightShadow(PointShadowMaps[PointLightShadowMapIdx], vec3(CoordWS), LightSourcePosWS, FragmentNormalWS, TextCoord/TextureDims, Rotation2D, WorldUpdate.CascadeSplits[0], 0.1);
			}
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
		if(abs(CoordVS.z) < WorldUpdate.CascadeSplits[CascadeIdx])
		{
			Layer  = CascadeIdx - 1;
			GlobalShadow = GetShadow(ShadowMap[Layer], ShadowPos[Layer], Rotation2D, WorldUpdate.CascadeSplits[CascadeIdx - 1], VertexNormalWS, GlobalLightPosWS, WorldUpdate.GlobalLightSize);
			CascadeCol = CascadeColors[Layer];

			float Fade = clamp((1.0 - (CoordVS.z * CoordVS.z) / (WorldUpdate.CascadeSplits[CascadeIdx] * WorldUpdate.CascadeSplits[CascadeIdx])) / 0.2, 0.0, 1.0);
			if(Fade > 0.0 && Fade < 1.0)
			{
				float NextShadow = GetShadow(ShadowMap[Layer + 1], ShadowPos[Layer + 1], Rotation2D, WorldUpdate.CascadeSplits[CascadeIdx], VertexNormalWS, GlobalLightPosWS, WorldUpdate.GlobalLightSize);
				vec3  NextCascadeCol = CascadeColors[Layer + 1];
				GlobalShadow = mix(NextShadow, GlobalShadow, Fade);
				CascadeCol = mix(NextCascadeCol, CascadeCol, Fade);
			}

			break;
		}
	}

	float GlobalLightIntensity = 0.2; //max(dot(vec3(0, 1, 0), normalize(GlobalLightPosWS)), 0.00001);

	float Shadow    = (GlobalShadow + LightShadow) / 2.0;
#if DEBUG_COLOR_BLEND
	imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(Diffuse, 1), vec4(1.0 / 2.0)));
#else
	if(WorldUpdate.DebugColors)
	{
		vec3 ShadowCol  = vec3(0.2) * (1.0 - Shadow);
		vec3 FinalLight = (Diffuse.xyz + ShadowCol) * CascadeCol;
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(pow(FinalLight, vec3(1.0 / 2.0)), 1.0));
	}
	else
	{
		vec3 FinalLight = (Diffuse.xyz*GlobalLightIntensity + LightDiffuse + LightSpecular) * AmbientOcclusion;
		FinalLight = mix(vec3(0.002), FinalLight, 1.0 - Shadow);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(pow(FinalLight, vec3(1.0 / 2.0)), 1));
	}
#endif
}
