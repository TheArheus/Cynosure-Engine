#version 450

#define SAMPLES_COUNT 64

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
	vec4 Col;
};

layout(binding = 0) readonly uniform block1 { global_world_data WorldUpdate; };
layout(binding = 1) uniform block2 { light_source LightSources[256]; };
layout(binding = 2) buffer  block3 { vec2 PoissonDisk[64]; };
layout(binding = 3) uniform sampler3D RandomAnglesTexture;
layout(binding = 4) uniform sampler2D PositionBuffer;
layout(binding = 5) uniform sampler2D NormalBuffer;
layout(binding = 6) uniform sampler2D DiffuseBuffer;
layout(binding = 7) uniform sampler2D SpecularBuffer;
layout(binding = 8) uniform writeonly image2D ColorTarget;
layout(binding = 9) uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
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

void main()
{
	vec2 TextureDims = textureSize(PositionBuffer, 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y) {
        return;
    }
	//TextCoord = floor(TextCoord / TextureDims);

	vec4  Coord    = texelFetch(PositionBuffer, ivec2(TextCoord), 0); //texture(PositionBuffer, TextCoord);
	vec3  Normal   = texelFetch(NormalBuffer, ivec2(TextCoord), 0).xyz; //texture(NormalBuffer, TextCoord).xyz;
	vec4  Diffuse  = texelFetch(DiffuseBuffer, ivec2(TextCoord), 0); //texture(DiffuseBuffer, TextCoord);
	float Specular = texelFetch(SpecularBuffer, ivec2(TextCoord), 0).x; //texture(SpecularBuffer, TextCoord).x;

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

	vec3 CamDir   = WorldUpdate.CameraDir.xyz;

	vec3 LightDiffuse   = vec3(0);
	vec3 LightSpecular  = vec3(0);
	for(uint LightIdx = 0;
		LightIdx < WorldUpdate.ColorSourceCount;
		++LightIdx)
	{
		vec4 LightCol = LightSources[LightIdx].Col;
		vec3 LightPos = LightSources[LightIdx].Pos.xyz;
		vec3 LightDir = LightPos - LightSources[LightIdx].Pos.w * Coord.xyz;

		// Diffuse Calculations
		float Attenuation = 1.0 / dot(LightDir, LightDir);
		LightDir = normalize(LightDir);
		float AndleOfIncidence = max(dot(LightDir, Normal), 0.0);

		vec3 Light = LightCol.xyz * LightCol.w * Attenuation;

		// Specular Calculations
		vec3 HalfA = normalize(LightDir + CamDir);
		float BlinnTerm = clamp(dot(Normal, HalfA), 0, 1);
		BlinnTerm = pow(BlinnTerm, 2.0);

		LightDiffuse  += Light * Diffuse.xyz * AndleOfIncidence;
		LightSpecular += Light * BlinnTerm;
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
			Shadow = GetShadow(ShadowMap[Layer], ShadowPos[Layer], Rotation, CascadeSplits[CascadeIdx - 1], Normal, GlobalLightPos.xyz);
			CascadeCol = CascadeColors[Layer];

			float Fade = clamp((1.0 - (Pos.z * Pos.z) / (CascadeSplits[CascadeIdx] * CascadeSplits[CascadeIdx])) / 0.1, 0.0, 1.0);
			if(Fade > 0.0 && Fade < 1.0)
			{
				float NextShadow = GetShadow(ShadowMap[Layer + 1], ShadowPos[Layer + 1], Rotation, CascadeSplits[CascadeIdx - 1], Normal, GlobalLightPos.xyz);
				vec3  NextCascadeCol = CascadeColors[Layer + 1];
				Shadow = mix(NextShadow, Shadow, Fade);
				CascadeCol = mix(NextCascadeCol, CascadeCol, Fade);
			}

			break;
		}
	}

	vec3 ShadowCol = vec3(0.2);

#if DEBUG_COLOR_BLEND
	imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0.1), 1));
#else
	if(WorldUpdate.DebugColors)
	{
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4(Diffuse.xyz*0.8 + (1.0 - Shadow), 1.0) * vec4(CascadeCol, 1.0));
	}
	else
	{
		imageStore(ColorTarget, ivec2(gl_GlobalInvocationID.xy), vec4((Diffuse.xyz*0.8 + LightDiffuse + LightSpecular) * (1.0 - Shadow) + ShadowCol * Shadow, 1.0));
	}
#endif
}
