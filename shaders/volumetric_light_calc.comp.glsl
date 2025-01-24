#version 450

#extension GL_EXT_scalar_block_layout: require

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

layout(set = 0, binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };
layout(set = 0, binding = 1) uniform sampler2D DepthTarget;
layout(set = 0, binding = 2) uniform sampler2D GBuffer[GBUFFER_COUNT];
layout(set = 0, binding = 3) uniform sampler2D ShadowMap[DEPTH_CASCADES_COUNT];
layout(set = 0, binding = 4) uniform writeonly image2D ColorOutput;
layout(push_constant) uniform pushConstant { float GScat; };


const float Pi = 3.14159265358979;

float MieScattering(float Angle)
{
	return (1.0 - GScat * GScat) / (4.0 * Pi * pow(1.0 + GScat * GScat - 2.0 * GScat * Angle, 1.5));
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

void main()
{
	vec2 TextureDims = textureSize(GBuffer[0], 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y)
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

	vec3  VertexNormalWS = normalize(texelFetch(GBuffer[0], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[1], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  LightViewDir = normalize(CoordWS - WorldUpdate.CameraPos.xyz);

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

	float Density       = 0.8;
	float GroundLevel   = 0.0;
	float Transmittance = 1.0;

	vec3  Volumetric = vec3(0);
	uint  LightSamplesCount = 64;
	float StepSize = (1.0 / LightSamplesCount);
	vec3  RayP = WorldUpdate.CameraPos.xyz;
	vec3  RayD = LightViewDir;
	for(uint StepIdx = 0; StepIdx < LightSamplesCount; ++StepIdx)
	{
		RayP += RayD * StepSize;

		vec4 ProjectedRayCoord = WorldUpdate.LightProj[Layer] * WorldUpdate.LightView[Layer] * vec4(RayP, 1.0);
		ProjectedRayCoord.xyz /= ProjectedRayCoord.w;
		ProjectedRayCoord.xy   = ProjectedRayCoord.xy * vec2(0.5, -0.5) + 0.5;

		float SampledDepth = texture(ShadowMap[Layer], ProjectedRayCoord.xy).r;
		float ShadowFactor = ((ProjectedRayCoord.z - 0.005) < SampledDepth) ? 1.0 : 0.0;

		vec3  Phase = MieScattering(dot(LightViewDir, -GlobalLightDirWS)) * vec3(0.8235, 0.7215, 0.0745);

		Volumetric += Phase * ShadowFactor * Density * Transmittance;
		Transmittance *= exp(-Density * StepSize);

		if(Transmittance < 0.001) break;
	}

	Volumetric /= LightSamplesCount;

	imageStore(ColorOutput, ivec2(TextCoord), vec4(Volumetric, 1));
}
