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

layout(binding = 0) readonly uniform block1 { global_world_data WorldUpdate; };
layout(binding = 1) uniform block2 { light_source LightSources[256]; };
layout(binding = 2) buffer  block3 { vec2 PoissonDisk[64]; };
layout(binding = 3) uniform sampler3D RandomAnglesTexture;
layout(binding = 4) uniform sampler2D PositionBuffer;
layout(binding = 5) uniform sampler2D VertexNormalBuffer;
layout(binding = 6) uniform sampler2D FragmentNormalBuffer;
layout(binding = 7) uniform sampler2D DiffuseBuffer;
layout(binding = 8) uniform sampler2D SpecularBuffer;
layout(binding = 9) uniform writeonly image2D ColorTarget;
layout(binding = 10) uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
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

#if 0
	float ConvSize = 9.0f;
	float ConvX = floor((ConvSize / 3.0) + 0.5);
	float ConvY = floor((ConvSize / 3.0) + 0.5);
	for(float x = -ConvX; x <= ConvX; x++)
	{
		for(float y = -ConvY; y <= ConvY; y++)
		{
			float ShadowDepth = texture(ShadowSampler, ShadowCoord + vec2(x, y) * TextureSize).r;
			Result += (ObjectDepth + Bias) > ShadowDepth ? 1.0 : 0.0;
		}
	}
	Result /= (ConvSize * ConvSize);
#else
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
#endif
	return Result;
}

void DirectionalLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Normal, vec3 CameraDir, vec4 LightDir, vec4 LightCol, vec3 Diffuse, float Specular)
{
	float AndleOfIncidence = max(dot(LightDir.xyz, Normal), 0.0);
	vec3 Light = LightCol.xyz;

	vec3 HalfA = normalize(LightDir.xyz + CameraDir);
	float BlinnTerm = clamp(dot(Normal, HalfA), 0, 1);
	BlinnTerm = pow(BlinnTerm, Specular);

	DiffuseResult  += Light * Diffuse * AndleOfIncidence;
	SpecularResult += Light * BlinnTerm;
}

void PointLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec4 LightPos, vec4 LightCol, vec3 Diffuse, float Specular)
{
	vec3  LightDir = LightPos.xyz - Coord.xyz;
	float Distance = length(LightDir);

	// Diffuse Calculations
	float Attenuation = 1.0 / Distance;
	float AngleOfIncidence = max(dot(normalize(LightDir), Normal), 0.0);

	vec3 Light = LightCol.xyz * Attenuation;

	// Specular Calculations
	vec3  HalfA = normalize(LightDir + CameraDir);
	float BlinnTerm = clamp(dot(Normal, HalfA), 0, 1);
	BlinnTerm = pow(BlinnTerm, Specular);

	DiffuseResult  += Light * Diffuse * AngleOfIncidence;
	SpecularResult += Light * BlinnTerm;
}

void SpotLight(inout vec3 DiffuseResult, inout vec3 SpecularResult, vec3 Coord, vec3 Normal, vec3 CameraDir, vec4 LightPos, vec4 LightView, vec4 LightCol, vec3 Diffuse, float Specular)
{
	vec3  LightDir = LightPos.xyz - Coord.xyz;
	float Distance = dot(LightDir, LightDir);

	// Diffuse Calculations
	float Attenuation = 1.0 / Distance;
	LightDir = normalize(LightDir);
	float AngleOfIncidence = max(dot(LightDir, Normal), 0.0);

	float MinCos = cos(radians(LightPos.w));
	float MaxCos = (MinCos + 1.0f) / 2.0f;
	float CosA   = dot(-LightDir, LightView.xyz);
	float SpotIntensity = smoothstep(MinCos, MaxCos, CosA);

	vec3 Light = LightCol.xyz * Attenuation * SpotIntensity;

	// Specular Calculations
	vec3 HalfA = normalize(LightDir + CameraDir);
	float BlinnTerm = clamp(dot(Normal, HalfA), 0, 1);
	BlinnTerm = pow(BlinnTerm, Specular);

	DiffuseResult  += Light * Diffuse * AngleOfIncidence;
	SpecularResult += Light * BlinnTerm;
}

void main()
{
	vec2 TextureDims = textureSize(PositionBuffer, 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y) {
        return;
    }

	vec4  Coord    = texelFetch(PositionBuffer, ivec2(TextCoord), 0);
	vec3  VertexNormal   = normalize(texelFetch(VertexNormalBuffer, ivec2(TextCoord), 0).xyz);
	vec3  FragmentNormal = normalize(texelFetch(FragmentNormalBuffer, ivec2(TextCoord), 0).xyz);
	vec4  Diffuse  = texelFetch(DiffuseBuffer, ivec2(TextCoord), 0);
	float Specular = texelFetch(SpecularBuffer, ivec2(TextCoord), 0).x;

	vec4 ShadowPos[4];
	for(uint CascadeIdx = 0;
		CascadeIdx < DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		ShadowPos[CascadeIdx] = WorldUpdate.LightProj[CascadeIdx] * WorldUpdate.LightView[CascadeIdx] * Coord;
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

	vec3  Pos = (WorldUpdate.DebugView * Coord).xyz;
	ivec3 AnglePos = ivec3(Pos + 31) % 32;
	vec2  Rotation = texture(RandomAnglesTexture, AnglePos).xy;

	vec4 GlobalLightPos = WorldUpdate.GlobalLightPos;
	vec4 GlobalLightDir = normalize(-WorldUpdate.GlobalLightPos);

	vec3 CamDir   = WorldUpdate.CameraPos.xyz - Coord.xyz;

	vec3 LightDiffuse   = vec3(0);
	vec3 LightSpecular  = vec3(0);
	for(uint LightIdx = 0;
		LightIdx < WorldUpdate.ColorSourceCount;
		++LightIdx)
	{
		vec4 LightSourcePos = LightSources[LightIdx].Pos;
		vec4 LightSourceDir = LightSources[LightIdx].Dir;
		vec4 LightSourceCol = LightSources[LightIdx].Col;

		if(LightSources[LightIdx].Type == light_type_directional)
			DirectionalLight(LightDiffuse, LightSpecular, FragmentNormal, CamDir, LightSourceDir, LightSourceCol, Diffuse.xyz, Specular);
		else if(LightSources[LightIdx].Type == light_type_point)
			PointLight(LightDiffuse, LightSpecular, Coord.xyz, FragmentNormal, CamDir, LightSourcePos, LightSourceCol, Diffuse.xyz, Specular);
		else if(LightSources[LightIdx].Type == light_type_spot)
			SpotLight(LightDiffuse, LightSpecular, Coord.xyz, FragmentNormal, CamDir, LightSourcePos, LightSourceDir, LightSourceCol, Diffuse.xyz, Specular);
	}

	uint  Layer = 0;
	float Shadow = 0.0;
	vec3  CascadeCol;
	for(uint CascadeIdx = 1;
		CascadeIdx <= DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		if(abs(Pos.z) < CascadeSplits[CascadeIdx])
		{
			Layer  = CascadeIdx - 1;
			Shadow = GetShadow(ShadowMap[Layer], ShadowPos[Layer], Rotation, CascadeSplits[CascadeIdx - 1], VertexNormal, GlobalLightPos.xyz);
			CascadeCol = CascadeColors[Layer];

			float Fade = clamp((1.0 - (Pos.z * Pos.z) / (CascadeSplits[CascadeIdx] * CascadeSplits[CascadeIdx])) / 0.2, 0.0, 1.0);
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
		vec3 FinalLight = (Diffuse.xyz*0.2 + LightDiffuse + LightSpecular);
		FinalLight += mix(ShadowCol, FinalLight, 1.0 - Shadow);
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), pow(vec4(FinalLight, 1), vec4(1.0 / 2.0)));
	}
#endif
}
