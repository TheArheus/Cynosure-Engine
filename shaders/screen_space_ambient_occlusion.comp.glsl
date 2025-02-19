#version 450

#extension GL_EXT_scalar_block_layout: require

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


layout(binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };
layout(binding = 1) readonly buffer b1 { vec4 HemisphereSamples[SAMPLES_COUNT]; };
layout(binding = 2) uniform sampler2D NoiseTexture;
layout(binding = 3) uniform sampler2D DepthTarget;
layout(binding = 4) uniform sampler2D GBuffer[GBUFFER_COUNT];
layout(binding = 5) uniform writeonly image2D OcclusionTarget;

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
	vec2 NoiseUV     = (gl_GlobalInvocationID.xy + 0.5) / TextureDims;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	float CurrDepth = texelFetch(DepthTarget, ivec2(TextCoord), 0).r;
	if(CurrDepth == 1.0)
	{
		imageStore(OcclusionTarget, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(0), 1));
		return;
	}

	vec3  CoordVS = ViewPosFromDepth(TextCoord / TextureDims, CurrDepth);

	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[1], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  FragmentNormalVS = normalize((transpose(inverse(WorldUpdate.DebugView)) * vec4(FragmentNormalWS, 1)).xyz);
	vec2  Rotation = texture(NoiseTexture, NoiseUV).xy;

	vec3  Tangent   = normalize(vec3(Rotation, 0) - FragmentNormalVS * dot(vec3(Rotation, 0), FragmentNormalVS));
	vec3  Bitangent = cross(FragmentNormalVS, Tangent);
	mat3  TBNVS     = mat3(Tangent, Bitangent, FragmentNormalVS);

	float Radius = 0.05;
	float OcclusionResult = 0.0;
	for(uint SampleIdx = 0; SampleIdx < SAMPLES_COUNT; ++SampleIdx)
	{
		vec3 SamplePosVS = TBNVS * HemisphereSamples[SampleIdx].xyz;
			 SamplePosVS = CoordVS + SamplePosVS * Radius;

		vec4 Offset = vec4(SamplePosVS, 1.0);
		Offset = WorldUpdate.Proj * Offset;
		Offset.xyz /= Offset.w;
		Offset.xy   = Offset.xy * vec2(0.5, -0.5) + 0.5;

		if (Offset.x < 0.0 || Offset.x > 1.0 ||
			Offset.y < 0.0 || Offset.y > 1.0)
		{
			continue;
		}

		vec3 SampledPosVS = ViewPosFromDepth(Offset.xy, texture(DepthTarget, Offset.xy).x);

		float Range = smoothstep(0.0, 1.0, Radius / abs(CoordVS.z - SampledPosVS.z));
		OcclusionResult += (((SampledPosVS.z - 0.005) > SamplePosVS.z) ? 1.0 : 0.0) * Range;
	}

	OcclusionResult = 1.0 - (OcclusionResult / SAMPLES_COUNT);
	imageStore(OcclusionTarget, ivec2(gl_GlobalInvocationID.xy), vec4(OcclusionResult, 0, 0, 0));
}
