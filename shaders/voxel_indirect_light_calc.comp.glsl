#version 450

#extension GL_EXT_scalar_block_layout: require
#define CONE_SAMPLES  6

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

const float Pi = 3.14159265358979;

const vec3  DiffuseConeDirections[CONE_SAMPLES] = {vec3(0.0, 1.0, 0.0), vec3(0.0, 0.5, 0.866025), vec3(0.823639, 0.5, 0.267617), vec3(0.509037, 0.5, -0.7006629), vec3(-0.50937, 0.5, -0.7006629), vec3(-0.823639, 0.5, 0.267617)};
const float DiffuseConeWeights[CONE_SAMPLES]    = {Pi / 4.0, 3.0 * Pi / 20.0, 3.0 * Pi / 20.0, 3.0 * Pi / 20.0, 3.0 * Pi / 20.0, 3.0 * Pi / 20.0};

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
layout(set = 0, binding = 3) uniform sampler3D VoxelGrid;
layout(set = 0, binding = 4) uniform writeonly image2D ColorOutput;


vec3 WorldPosFromDepth(vec2 TextCoord, float Depth) 
{
	TextCoord.y = 1 - TextCoord.y;
    vec4 ClipSpacePosition = vec4(TextCoord * 2.0 - 1.0, Depth, 1.0);
    vec4 ViewSpacePosition = inverse(WorldUpdate.Proj * WorldUpdate.DebugView) * ClipSpacePosition;
    ViewSpacePosition /= ViewSpacePosition.w;

    return ViewSpacePosition.xyz;
}

bool IsInsideUnitCube(vec3 Coord)
{
	return abs(Coord.x) < 1.0 && abs(Coord.y) < 1.0 && abs(Coord.z) < 1.0;
}

vec4 TraceCone(vec3 Coord, vec3 Direction, vec3 Normal, float Angle)
{
	vec3 VoxelSceneScale = WorldUpdate.SceneScale.xyz;

	float Aperture  = max(0.01, tan(Angle / 2.0));
	float Distance  = 0.1;
	vec3  AccumColor = vec3(0);
	float AccumOcclusion = 0.0;

	vec3 StartPos   = (Coord - WorldUpdate.SceneCenter.xyz) + Normal * Distance;
	while(Distance <= 1.0 && AccumOcclusion < 0.9)
	{
		vec3  ConePos = (StartPos + Direction * Distance) * VoxelSceneScale;
		if(!IsInsideUnitCube(ConePos)) break;

		ConePos = ConePos * 0.5 + 0.5;

		float Diameter = 2.0 * Aperture * Distance;
		int Mip = int(floor(log2(Distance)));
		vec4  VoxelSample = textureLod(VoxelGrid, ConePos, Mip);

		AccumColor += (1.0 - AccumOcclusion) * VoxelSample.rgb;
		AccumOcclusion += (1.0 - AccumOcclusion) * VoxelSample.a;

		Distance += Diameter;
	}

	AccumOcclusion = min(AccumOcclusion, 1.0);
	return vec4(AccumColor, AccumOcclusion);
}

void main()
{
	vec2 TextureDims = textureSize(GBuffer[0], 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	float CurrDepth = texelFetch(DepthTarget, ivec2(TextCoord), 0).r;
	vec3  CoordWS = WorldPosFromDepth(TextCoord / TextureDims, CurrDepth);

	if(CurrDepth == 1.0)
	{
		imageStore(ColorOutput, ivec2(TextCoord), vec4(vec3(0), 1));
		return;
	}

	vec3  Diffuse  = texelFetch(GBuffer[2], ivec2(TextCoord), 0).rgb;
	float Specular = texelFetch(GBuffer[4], ivec2(TextCoord), 0).r;

	vec3  VertexNormalWS   = normalize(texelFetch(GBuffer[0], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[1], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);

	vec4  IndirectDiffuse  = vec4(0);
	vec4  IndirectSpecular = vec4(0);
	{
		vec3 Guide = vec3(0, 1, 0);
		if(abs(dot(VertexNormalWS, Guide)) == 1.0)
			 Guide = vec3(0, 0, 1);
		vec3 Right = normalize(Guide - dot(Guide, VertexNormalWS) * VertexNormalWS);
		vec3 Up    = cross(Right, VertexNormalWS);

		for(uint i = 0; i < CONE_SAMPLES; i++)
		{
			vec3 ConeDirection = VertexNormalWS;
			ConeDirection += DiffuseConeDirections[i].x * Right + DiffuseConeDirections[i].z * Up;
			ConeDirection  = normalize(ConeDirection);

			IndirectDiffuse += TraceCone(CoordWS, ConeDirection, VertexNormalWS, Pi / 4.0) * DiffuseConeWeights[i];
		}

		IndirectDiffuse.rgb *= Diffuse * 0.01;
	}

	{
		vec3 ViewDirWS = normalize(WorldUpdate.CameraPos.xyz - CoordWS);
		vec3 ReflDirWS = normalize(reflect(-ViewDirWS, VertexNormalWS));
		IndirectSpecular = TraceCone(CoordWS, ReflDirWS, VertexNormalWS, Pi / 12.0);
		IndirectSpecular.rgb *= Specular;
	}

	imageStore(ColorOutput, ivec2(TextCoord), IndirectDiffuse + IndirectSpecular);
} 
