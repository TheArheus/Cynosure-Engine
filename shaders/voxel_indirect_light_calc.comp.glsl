#version 450
#define CONE_SAMPLES 16

#extension GL_EXT_scalar_block_layout: require

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

const float Pi = 3.14159265358979;

vec3 DiffuseConeDirections[CONE_SAMPLES];
float DiffuseConeWeights[CONE_SAMPLES];

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
layout(set = 0, binding = 3) uniform sampler3D VoxelGridR;
layout(set = 0, binding = 4) uniform sampler3D VoxelGridG;
layout(set = 0, binding = 5) uniform sampler3D VoxelGridB;
layout(set = 0, binding = 6) uniform sampler3D VoxelGridNormalX;
layout(set = 0, binding = 7) uniform sampler3D VoxelGridNormalY;
layout(set = 0, binding = 8) uniform sampler3D VoxelGridNormalZ;
layout(set = 0, binding = 9) uniform sampler3D VoxelGridNormalW;
layout(set = 0, binding = 10) uniform writeonly image2D ColorOutput;


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

vec3 WorldPosFromDepth(vec2 TextCoord, float Depth) 
{
	TextCoord.y = 1 - TextCoord.y;
    vec4 ClipSpacePosition = vec4(TextCoord * 2.0 - 1.0, Depth, 1.0);
    vec4 ViewSpacePosition = inverse(WorldUpdate.Proj * WorldUpdate.DebugView) * ClipSpacePosition;
    ViewSpacePosition /= ViewSpacePosition.w;

    return ViewSpacePosition.xyz;
}

vec3 TangentToWorld(vec3 dir, vec3 normal)
{
    vec3 up = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent   = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    return tangent * dir.x + bitangent * dir.y + normal * dir.z;
}

vec3 HemisphereDirection(float u1, float u2, vec3 normal)
{
    float theta = acos(u1); // Polar angle
    float phi = 2.0 * Pi * u2; // Azimuthal angle

    vec3 direction;
    direction.x = sin(theta) * cos(phi);
    direction.y = sin(theta) * sin(phi);
    direction.z = cos(theta);

    // Transform to align with the normal
    vec3 up = abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return tangent * direction.x + bitangent * direction.y + normal * direction.z;
}

void InitializeCones()
{
    for (int i = 0; i < CONE_SAMPLES; i++)
	{
        float u1 = float(i) / float(CONE_SAMPLES);
        float u2 = fract(sin(float(i) * 12.9898) * 43758.5453);
        DiffuseConeDirections[i] = HemisphereDirection(u1, u2, vec3(0, 1, 0));
        DiffuseConeWeights[i] = 1.0 / float(CONE_SAMPLES);
    }
}

bool IsInsideVoxelGrid(vec3 Coord)
{
    return all(greaterThanEqual(Coord, vec3(0.0))) && all(lessThan(Coord, vec3(1.0)));
}

vec4 TraceCone(vec3 Coord, vec3 Direction, vec3 Normal, float Angle, int BounceCount)
{
    vec3 VoxelSceneScale = WorldUpdate.SceneScale.xyz;
    int  MaxConeRadius = int(textureQueryLevels(VoxelGridR) - 1);

	float VoxelSize = VOXEL_SIZE;
	vec3  VoxelGridMin = WorldUpdate.SceneCenter.xyz - VoxelSize * VoxelSceneScale;
	vec3  VoxelGridMax = WorldUpdate.SceneCenter.xyz + VoxelSize * VoxelSceneScale;
	vec3  VoxelGridSize = VoxelGridMax - VoxelGridMin;
	float VoxelsPerWorldUnit = VoxelSize / VoxelGridSize.x;

    float Aperture  = max(0.01, tan(Angle / 2.0));
    float Distance  = 1e-5;
    vec3  AccumColor = vec3(0);
    float AccumOcclusion = 0.0;

    vec3 StartPos   = Coord + Normal * Distance;
    while(Distance <= 1.0 && AccumOcclusion < 0.9)
    {
        vec3 ConePosWorld = StartPos + Direction * Distance;
		vec3 ConePos = (ConePosWorld - VoxelGridMin) / VoxelGridSize;
        if(!IsInsideVoxelGrid(ConePos)) break;

        float Diameter = 2.0 * Aperture * 1e-2; //Distance;
        int Mip = int(floor(log2(Diameter * VoxelsPerWorldUnit)));
            Mip = clamp(int(floor(Mip)), 0, MaxConeRadius);

		float SampleAccumCount = textureLod(VoxelGridNormalW, ConePos, Mip).x;
        vec3 VoxelSample = vec3(textureLod(VoxelGridR, ConePos, Mip).x, 
								textureLod(VoxelGridG, ConePos, Mip).x, 
								textureLod(VoxelGridB, ConePos, Mip).x);// / SampleAccumCount;
		vec3 VoxelNormal = vec3(textureLod(VoxelGridNormalX, ConePos, 0).x,
								textureLod(VoxelGridNormalY, ConePos, 0).x,
								textureLod(VoxelGridNormalZ, ConePos, 0).x) / SampleAccumCount;
		VoxelNormal = normalize(VoxelNormal);

		float AngleCosine = dot(-Direction, VoxelNormal);
		float Weight = 1.0;
		//float Weight = pow(clamp(AngleCosine, 0.0, 1.0), 2.0);
		//float Weight = smoothstep(0.0, 1.0, AngleCosine);
		AccumColor += (1.0 - AccumOcclusion) * Weight * VoxelSample.rgb;
		AccumOcclusion += (1.0 - AccumOcclusion) * Weight;// * VoxelSample.a;

        Distance += Diameter;
    }

    AccumOcclusion = min(AccumOcclusion, 1.0);
    return vec4(AccumColor, AccumOcclusion);
}


void main()
{
	vec2 TextureDims = textureSize(GBuffer[0], 0).xy;
	vec2 TextCoord   = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y)
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

	InitializeCones();

	float Metallic  = 1.0;
	float Roughness = 0.8;

	vec3  Diffuse  = texelFetch(GBuffer[2], ivec2(TextCoord), 0).rgb;
	float Specular = texelFetch(GBuffer[4], ivec2(TextCoord), 0).r;

	vec3  VertexNormalWS   = normalize(texelFetch(GBuffer[0], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);
	vec3  FragmentNormalWS = normalize(texelFetch(GBuffer[1], ivec2(TextCoord), 0).xyz * 2.0 - 1.0);

	vec3 N = VertexNormalWS;
	vec3 V = normalize(WorldUpdate.CameraPos.xyz - CoordWS);
	vec3 R = reflect(-V, N);

	vec3 F0 = mix(vec3(0.04), Diffuse, Metallic);

	vec3 IndirectLight = vec3(0);

	for(uint i = 0; i < CONE_SAMPLES; i++)
	{
		vec3 L = normalize(TangentToWorld(DiffuseConeDirections[i], N));
		vec3 H = normalize(V + L);

		vec4 IncomingRadiance = TraceCone(CoordWS, L, N, Pi / 4.0, 3);

		float NDF = NormalDistributionGGX(N, H, Roughness);
		float G = GeometrySmith(N, V, L, Roughness);
		vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 SpecularBRDF = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001);

		vec3 kD = (1.0 - F) * (1.0 - Metallic);
		vec3 DiffuseBRDF = kD * Diffuse / Pi;

		float NdotL = max(dot(N, L), 0.0);
		IndirectLight += (DiffuseBRDF + SpecularBRDF) * IncomingRadiance.rgb * NdotL * DiffuseConeDirections[i];
	}

	imageStore(ColorOutput, ivec2(TextCoord), vec4(IndirectLight, 1.0));
} 
