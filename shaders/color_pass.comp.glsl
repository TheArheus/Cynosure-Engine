#version 450

#extension GL_EXT_scalar_block_layout: require

#define SAMPLES_COUNT 16
#define light_type_none 0
#define light_type_directional 1
#define light_type_point 2
#define light_type_spot 3

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

const float GScat    = 0.7;
const float RayStep  = 0.1;
const uint  MaxSteps = 128;

const float Pi = 3.14159265358979;

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
layout(set = 0, binding = 3) readonly buffer b3 { vec4 HemisphereSamples[SAMPLES_COUNT]; };
layout(set = 0, binding = 4) uniform sampler2D PrevColorTarget;
layout(set = 0, binding = 5) uniform sampler2D DepthTarget;
layout(set = 0, binding = 6) uniform sampler3D RandomAnglesTexture;
layout(set = 0, binding = 7) uniform sampler2D GBuffer[GBUFFER_COUNT];
layout(set = 0, binding = 8) uniform writeonly image2D ColorTarget;
layout(set = 0, binding = 9) uniform writeonly image2D BrightTarget;

layout(set = 1, binding = 0) uniform sampler2D AmbientOcclusionBuffer;
layout(set = 2, binding = 0) uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
layout(set = 3, binding = 0) uniform sampler2D ShadowMaps[LIGHT_SOURCES_MAX_COUNT];
layout(set = 4, binding = 0) uniform samplerCube PointShadowMaps[LIGHT_SOURCES_MAX_COUNT];

float GetRandomValue(vec2 Seed)
{
    float  RandomValue = fract(sin(dot(Seed, vec2(12.9898, 78.233))) * 43758.5453);
    return RandomValue;
}

vec3 WorldPosFromDepth(vec2 TextCoord, float Depth) 
{
	TextCoord.y = 1 - TextCoord.y;
    vec4 ClipSpacePosition = vec4(TextCoord * 2.0 - 1.0, Depth, 1.0);
    vec4 ViewSpacePosition = inverse(WorldUpdate.Proj * WorldUpdate.DebugView) * ClipSpacePosition;
    ViewSpacePosition /= ViewSpacePosition.w;

    return ViewSpacePosition.xyz;
}

vec3 ViewPosFromDepth(vec2 TextCoord, float Depth) 
{
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
	float  NdotL = max(dot(ToLightDir, Normal), 0);
	return LightCol * NdotL;
}

vec3 DoSpecular(vec3 LightCol, vec3 ToLightDir, vec3 CameraDir, vec3 Normal, float Specular)
{
	vec3 Half = normalize(ToLightDir + CameraDir);
	return LightCol * pow(max(dot(Half, Normal), 0), Specular) * max(dot(ToLightDir, Normal), 0.0);
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
	float Distance   = length(ToLightDir);
	ToLightDir = normalize(ToLightDir);

	vec3 NewDiffuse  = DoDiffuse(LightCol, ToLightDir, Normal) * Intensity;
	vec3 NewSpecular = DoSpecular(LightCol, ToLightDir, CameraDir, Normal, Specular) * Intensity;

	DiffuseResult  += NewDiffuse.rgb;
	SpecularResult += NewSpecular.rgb;
}

void PointLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightCol, float Intensity, vec3 Diffuse, float Specular)
{
	vec3  ToLightDir = LightPos - Coord;
	float Distance   = length(ToLightDir);
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
	float Distance    = length(LightPosDir);

	float Attenuation   = DoAttenuation(Radius, Distance);
	float SpotIntensity = DoSpotCone(Radius, LightPosDir, LightView.xyz);
	vec3  NewDiffuse    = DoDiffuse(LightCol, LightPosDir, Normal) * Attenuation * SpotIntensity * Intensity;
	vec3  NewSpecular   = DoSpecular(LightCol, LightPosDir, CameraDir, Normal, Specular) * Attenuation * SpotIntensity * Intensity;

	DiffuseResult  += NewDiffuse;
	SpecularResult += NewSpecular;
}

float MieScattering(float Angle)
{
	return (1.0 - GScat * GScat) / (4.0 * Pi * pow(1.0 + GScat * GScat - 2.0 * GScat * Angle, 1.5));
}

float NormalDistributionGGX(vec3 Normal, vec3 Half, float Roughness)
{
	float a2    = Roughness * Roughness;
	float NdotH = max(dot(Normal, Half), 0.0);

	float Denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);

	return a2 / (Pi * Denom * Denom);
}

float GeometrySchlickGGX(float NdotV, float Roughness)
{
	float  r = Roughness + 1.0;
	float  k = r * r / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 Normal, vec3 ViewDir, vec3 LightDir, float Roughness)
{
	float NdotV = max(dot(Normal, ViewDir ), 0.0);
	float NdotL = max(dot(Normal, LightDir), 0.0);
	float  GGX1 = GeometrySchlickGGX(NdotV, Roughness);
	float  GGX2 = GeometrySchlickGGX(NdotL, Roughness);
	return GGX1 * GGX2;
}

vec3 FresnelSchlick(float CosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - CosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float CosTheta, vec3 F0, float Roughness)
{
	return F0 + (max(vec3(1.0 - Roughness), F0) - F0) * pow(clamp(1.0 - CosTheta, 0.0, 1.0), 5.0);
}

void GetPBRColor(inout vec3 DiffuseColor, inout vec3 SpecularColor, vec3 Coord, vec3 Normal, vec3 CameraDir, vec3 LightPos, float Radius, vec3 LightDir, vec3 LightCol, float Intensity, vec3 Diffuse, float Roughness, float Metalness)
{
	vec3  Half        = normalize(LightDir + CameraDir);
	float Distance    = length(LightDir);
	float Attenuation = DoAttenuation(Radius, Distance);
	//float Attenuation = 1.0 / ((1 + 0.22 * Distance / Radius) * (1 + 0.2 * Distance * Distance / (Radius * Radius)));

	float NdotL = max(dot(Normal, LightDir ), 0.0);
	float NdotV = max(dot(Normal, CameraDir), 0.0);

	vec3  Radiance = LightCol * Attenuation;

	vec3 F0 = mix(vec3(0.04), Diffuse, Metalness);
	vec3  F = FresnelSchlick(NdotV, F0);
	float G = GeometrySmith(Normal, CameraDir, LightDir, Roughness);
	float D = NormalDistributionGGX(Normal, Half, Roughness);

	vec3 kDiff = (1.0 - F);
		 kDiff = kDiff * (1.0 - Metalness);

	vec3 DiffusePart  = kDiff * Diffuse / Pi;
	vec3 SpecularPart = (D * F * G) / (4 * NdotL * NdotV + 0.00001);

	vec3 RadianceAmmount = Radiance * NdotL;
	DiffuseColor  += DiffusePart  * RadianceAmmount;
	SpecularColor += SpecularPart * RadianceAmmount;
}

vec3 VecHash(vec3 a)
{
	a  = fract(a * vec3(0.8));
	a += dot(a, a.yxz + 19.19);
	return fract((a.xxy + a.yxx)*a.zyx);
}

vec2 RayBinarySearch(inout vec3 RayP, inout vec3 RayD, float Dist)
{
	const uint StepsCount = 16;
	vec4  ProjectedCoord  = vec4(0.0);

	for(uint StepIdx = 0; StepIdx < StepsCount; StepIdx++)
	{
		vec4 ProjectedRayCoord = WorldUpdate.Proj * vec4(RayP, 1.0);
		ProjectedRayCoord   /= ProjectedRayCoord.w;
		ProjectedRayCoord.xy = ProjectedRayCoord.xy * vec2(0.5, -0.5) + 0.5;

		if(ProjectedRayCoord.x < 0.0 || ProjectedRayCoord.y < 0.0 || ProjectedRayCoord.x > 1.0 || ProjectedRayCoord.y > 1.0)
			return vec2(-1);

		vec3 SampledPosVS = ViewPosFromDepth(ProjectedRayCoord.xy, textureLod(DepthTarget, ProjectedRayCoord.xy, 2).x);

		float Delta = RayP.z - SampledPosVS.z;

		RayD *= 0.5;
		if(Delta > 0.0)
			RayP += RayD;
		else
			RayP -= RayD;
	}

	ProjectedCoord = WorldUpdate.Proj * vec4(RayP, 1.0);
	ProjectedCoord = ProjectedCoord / ProjectedCoord.w;
	return ProjectedCoord.xy * vec2(0.5, -0.5) + 0.5;
}

vec2 ReflectedRayCast(vec3 Coord, vec3 ReflDir)
{
	vec3  RayP = Coord;
	vec3  RayD = ReflDir*RayStep;

	for(uint StepIdx = 0; StepIdx < MaxSteps; ++StepIdx)
	{
		RayP += RayD;
		
		vec4 ProjectedRayCoord = WorldUpdate.Proj * vec4(RayP, 1.0);
		ProjectedRayCoord   /= ProjectedRayCoord.w;
		ProjectedRayCoord.xy = ProjectedRayCoord.xy * vec2(0.5, -0.5) + 0.5;

		if(ProjectedRayCoord.x < 0.0 || ProjectedRayCoord.x > 1.0 || ProjectedRayCoord.y < 0.0 || ProjectedRayCoord.y > 1.0)
			return vec2(-1);

		vec3 SampledPosVS = ViewPosFromDepth(ProjectedRayCoord.xy, textureLod(DepthTarget, ProjectedRayCoord.xy, 2).x);

		if(SampledPosVS.z >= WorldUpdate.FarZ)
			break;

		float Delta = RayP.z - SampledPosVS.z;

		if(((RayD.z - Delta) < 1.2) && (Delta < 0.0))
		{
			return RayBinarySearch(RayP, RayD, RayStep);
		}
	}

	return vec2(-1);
}


void main()
{
	vec2 TextureDims = textureSize(GBuffer[0], 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	vec3  GlobalLightPosWS = WorldUpdate.GlobalLightPos.xyz;
	vec3  GlobalLightDirWS = normalize(-WorldUpdate.GlobalLightPos.xyz);
	vec3  GlobalLightPosVS = (WorldUpdate.DebugView * vec4(GlobalLightPosWS, 1)).xyz;
	vec3  GlobalLightDirVS = normalize((WorldUpdate.DebugView * vec4(GlobalLightDirWS, 1)).xyz);

	float CurrDepth = texelFetch(DepthTarget, ivec2(TextCoord), 0).r;
	vec3  CoordWS = WorldPosFromDepth(TextCoord / TextureDims, CurrDepth);
	vec3  CoordVS = vec3(WorldUpdate.DebugView * vec4(CoordWS, 1.0));

	vec3  Volumetric = vec3(0);
	uint  Layer = 0;
	for(uint CascadeIdx = 1;
		CascadeIdx <= DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		if(abs(CoordVS.z) < WorldUpdate.CascadeSplits[CascadeIdx])
		{
			Layer = CascadeIdx - 1;
			break;
		}
	}
	{
		vec3 LightViewDir = normalize(CoordWS - WorldUpdate.CameraPos.xyz);

		uint LightSamplesCount = 64;
		vec3 RayP = WorldUpdate.CameraPos.xyz;
		vec3 RayD = LightViewDir * (1.0 / LightSamplesCount);
		for(uint StepIdx = 0; StepIdx < LightSamplesCount; ++StepIdx)
		{
			RayP += RayD;

			vec4 ProjectedRayCoord = WorldUpdate.LightProj[Layer] * WorldUpdate.LightView[Layer] * vec4(RayP, 1.0);
			ProjectedRayCoord.xyz /= ProjectedRayCoord.w;
			ProjectedRayCoord.xy   = ProjectedRayCoord.xy * vec2(0.5, -0.5) + 0.5;

			float SampledDepth = texture(ShadowMap[Layer], ProjectedRayCoord.xy).r;
			if((ProjectedRayCoord.z + 0.01) < SampledDepth)
			{
				Volumetric += MieScattering(dot(LightViewDir, -GlobalLightDirWS)) * vec3(0.8235, 0.7215, 0.0745);
			}
		}

		Volumetric /= LightSamplesCount;
	}

	float Metallic  = 1.0;
	float Roughness = 0.8;
	if(CurrDepth == 1.0)
	{
		imageStore(ColorTarget , ivec2(gl_GlobalInvocationID.xy), vec4(Volumetric, 1));
		if(dot(Volumetric, vec3(0.2126, 0.7152, 0.0722)) > 1.0)
			imageStore(BrightTarget, ivec2(gl_GlobalInvocationID.xy), vec4(Volumetric, 1));
		else
			imageStore(BrightTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0), 1));
		imageStore(BrightTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0), 1));
		return;
	}

	vec3  RotationAngles = texture(RandomAnglesTexture, CoordVS / 32).xyz;
	vec2  Rotation2D = vec2(cos(RotationAngles.x), sin(RotationAngles.y));
	vec3  Rotation3D = vec3(cos(RotationAngles.x)*sin(RotationAngles.y), sin(RotationAngles.x)*sin(RotationAngles.y), cos(RotationAngles.y));

	vec3  Diffuse  = texelFetch(GBuffer[1], ivec2(TextCoord), 0).rgb;
	vec3  Emmit    = texelFetch(GBuffer[2], ivec2(TextCoord), 0).rgb;
	float Specular = texelFetch(GBuffer[3], ivec2(TextCoord), 0).r;
	float EmmitSze = texelFetch(GBuffer[3], ivec2(TextCoord), 0).g;
	float AmbientOcclusion = texelFetch(AmbientOcclusionBuffer, ivec2(TextCoord), 0).r;

	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[0], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  FragmentNormalVS = normalize(inverse(transpose(mat3(WorldUpdate.DebugView))) * FragmentNormalWS).xyz;

	vec3  ViewDirVS = normalize(-CoordVS);
	vec3  ReflDirVS = normalize(reflect(CoordVS, FragmentNormalVS));

	vec3  Jitt = mix(vec3(0.0), VecHash(CoordWS), Specular);
	float FresnelSSR = 2.0 * pow(1.0 - max(dot(ViewDirVS, FragmentNormalVS), 0.0), 2.0);

	vec2  ReflTextCoord = ReflectedRayCast(CoordVS, ReflDirVS + Jitt);
	if(ReflTextCoord.x > 0.0)
	{
		vec2  dReflCoord = smoothstep(0.2, 0.6, abs(vec2(0.5) - ReflTextCoord));
		float ScreenEdgeFactor = clamp(1.0 - (dReflCoord.x + dReflCoord.y), 0.0, 1.0);
		float ReflectionMultiplier = pow(Metallic, 3.0) * ScreenEdgeFactor * -ReflDirVS.z;
		vec3  ReflDiffuse = texture(PrevColorTarget, ReflTextCoord).rgb * clamp(ReflectionMultiplier, 0.0, 0.8) * FresnelSSR;

		Diffuse = ReflDiffuse; //mix(ReflDiffuse, Diffuse, 0.4);
	}

	vec4 ShadowPosLS[4];
	vec4 CameraPosLS[4];
	for(uint CascadeIdx = 0;
		CascadeIdx < DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		ShadowPosLS[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * vec4(CoordWS, 1.0);
		CameraPosLS[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * WorldUpdate.CameraPos;
	}

	vec3 LightDiffuse  = vec3(0);
	vec3 LightSpecular = vec3(0);
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
			vec3 LightDirVS = LightSourcePosVS - CoordVS;
			GetPBRColor(LightDiffuse, LightSpecular, CoordVS, FragmentNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightDirVS, LightSourceCol, Intensity, Diffuse, Roughness, Metallic);
			if(WorldUpdate.LightSourceShadowsEnabled)
			{
				LightShadow += GetPointLightShadow(PointShadowMaps[PointLightShadowMapIdx], CoordWS, LightSourcePosWS, FragmentNormalWS, TextCoord/TextureDims, Rotation2D, WorldUpdate.CascadeSplits[0], 0.1);
			}
			PointLightShadowMapIdx++;
		}
		else if(LightSources[LightIdx].Type == light_type_spot)
		{
			GetPBRColor(LightDiffuse, LightSpecular, CoordVS, FragmentNormalVS, ViewDirVS, LightSourcePosVS, Radius, LightSourceDirVS, LightSourceCol, Intensity, Diffuse, Roughness, Metallic);
			SpotLightShadowMapIdx++;
		}
	}
	LightShadow /= float(TotalLightSourceCount);

	float GlobalShadow = 0.0;
	vec3  CascadeCol;
	{
		GlobalShadow = GetShadow(ShadowMap[Layer], ShadowPosLS[Layer], Rotation2D, WorldUpdate.CascadeSplits[Layer], FragmentNormalWS, GlobalLightPosWS, WorldUpdate.GlobalLightSize);
		CascadeCol = CascadeColors[Layer];

		float Fade = clamp((1.0 - (CoordVS.z * CoordVS.z) / (WorldUpdate.CascadeSplits[Layer + 1] * WorldUpdate.CascadeSplits[Layer + 1])) / 0.2, 0.0, 1.0);
		if(Fade > 0.0 && Fade < 1.0)
		{
			float NextShadow = GetShadow(ShadowMap[Layer + 1], ShadowPosLS[Layer + 1], Rotation2D, WorldUpdate.CascadeSplits[Layer + 1], FragmentNormalWS, GlobalLightPosWS, WorldUpdate.GlobalLightSize);
			vec3  NextCascadeCol = CascadeColors[Layer + 1];
			GlobalShadow = mix(NextShadow, GlobalShadow, Fade);
			CascadeCol = mix(NextCascadeCol, CascadeCol, Fade);
		}
	}

	float Shadow    = (GlobalShadow + LightShadow) / 2.0;
#if DEBUG_COLOR_BLEND
	imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(Diffuse, 1), vec4(1.0 / 2.0)));
#else
	if(WorldUpdate.DebugColors)
	{
		vec3 ShadowCol  = vec3(0.2) * (1.0 - Shadow);
		vec3 FinalLight = ShadowCol * CascadeCol;
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(pow(FinalLight, vec3(1.0 / 2.0)), 1.0));
	}
	else
	{
		vec3 FinalLight = (LightDiffuse + LightSpecular);// * AmbientOcclusion;
			 FinalLight = Volumetric + Emmit * EmmitSze + mix(vec3(0.001), FinalLight, 1.0 - Shadow);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(FinalLight, 1));
		if(dot(FinalLight, vec3(0.2126, 0.7152, 0.0722)) > 1.0)
			imageStore(BrightTarget, ivec2(gl_GlobalInvocationID.xy), vec4(FinalLight, 1));
		else
			imageStore(BrightTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0), 1));
	}
#endif
}
