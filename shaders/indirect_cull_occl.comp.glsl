#version 450

#extension GL_EXT_scalar_block_layout: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

struct sphere
{
	vec4  Center;
	float Radius;
};

struct aabb
{
	vec4 Min;
	vec4 Max;
};

struct offset
{
	aabb AABB;
	sphere BoundingSphere;

	uint VertexOffset;
	uint VertexCount;

	uint IndexOffset;
	uint IndexCount;

	uint InstanceOffset;
	uint InstanceCount;
};

struct mesh_draw_command
{
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
	uint MeshIndex;
};

struct plane
{
	vec4 P;
	vec4 N;
};

struct mesh_comp_culling_common_input
{
	mat4  Proj;
	mat4  View;
	plane Planes[6];
	bool  FrustrumCullingEnabled;
	bool  OcclusionCullingEnabled;
	float NearZ;
	uint  DrawCount;
	uint  MeshCount;
	uint  DebugDrawCount;
	uint  DebugMeshCount;
};

struct indirect_draw_indexed_command
{
    uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    int  VertexOffset;
    uint FirstInstance;
	uint DrawID;
};


layout(binding = 0) buffer readonly b0 { mesh_comp_culling_common_input MeshCullingCommonInput; };
layout(binding = 1) buffer readonly b1 { offset MeshOffsets[]; };
layout(binding = 2) buffer readonly b2 { mesh_draw_command MeshDrawCommandData[]; };
layout(binding = 3) uniform sampler2D DepthPyramid;
layout(binding = 4) buffer b3 { bool MeshDrawVisibilityData[]; };

layout(push_constant) uniform pushConstant { uint DrawCount; uint MeshCount; };


void main()
{
	uint DrawIndex = gl_GlobalInvocationID.x;
	if(DrawIndex >= DrawCount) return;

	uint MeshIndex = MeshDrawCommandData[DrawIndex].MeshIndex;

	mat4 Proj = MeshCullingCommonInput.Proj;
	mat4 View = MeshCullingCommonInput.View;

	vec3 _SphereCenter = MeshOffsets[MeshIndex].BoundingSphere.Center.xyz * MeshDrawCommandData[DrawIndex].Scale.xyz + MeshDrawCommandData[DrawIndex].Translate.xyz;
	vec4  SphereCenter = View * vec4(_SphereCenter, 1);
	vec3  SphereRadius = vec3(MeshOffsets[MeshIndex].BoundingSphere.Radius) * MeshDrawCommandData[DrawIndex].Scale.xyz;

	// Frustum Culling
	bool IsVisible = true;
	if(MeshCullingCommonInput.FrustrumCullingEnabled)
	{
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[0].N, SphereCenter) > -SphereRadius.x);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[1].N, SphereCenter) > -SphereRadius.x);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[2].N, SphereCenter) > -SphereRadius.y);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[3].N, SphereCenter) > -SphereRadius.y);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[4].N, SphereCenter) > -SphereRadius.z);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[5].N, SphereCenter) > -SphereRadius.z);
	}

	vec3 BoxMin = MeshOffsets[MeshIndex].AABB.Min.xyz * MeshDrawCommandData[DrawIndex].Scale.xyz + MeshDrawCommandData[DrawIndex].Translate.xyz;
	vec3 BoxMax = MeshOffsets[MeshIndex].AABB.Max.xyz * MeshDrawCommandData[DrawIndex].Scale.xyz + MeshDrawCommandData[DrawIndex].Translate.xyz;
	vec3 BoxSize = BoxMax - BoxMin;
	vec3 BoxCorners[] = { BoxMin,
						  BoxMin + vec3(BoxSize.x,0,0),
						  BoxMin + vec3(0, BoxSize.y,0),
						  BoxMin + vec3(0, 0, BoxSize.z),
						  BoxMin + vec3(BoxSize.xy,0),
						  BoxMin + vec3(0, BoxSize.yz),
						  BoxMin + vec3(BoxSize.x, 0, BoxSize.z),
						  BoxMin + BoxSize
						 };

	float FltMax = 3.402823466e+38;
	vec3  NewMin = vec3( FltMax);
	vec3  NewMax = vec3(-FltMax);
	for(uint i = 0; i < 8; i++)
	{
		vec4 ClipPos = Proj * View * vec4(BoxCorners[i], 1);
		ClipPos.xyz  = ClipPos.xyz / ClipPos.w;

		NewMin = min(ClipPos.xyz, NewMin);
		NewMax = max(ClipPos.xyz, NewMax);
	}
	NewMin = clamp(NewMin * vec3(0.5, -0.5, 1) + vec3(0.5, 0.5, 0), 0, 1);
	NewMax = clamp(NewMax * vec3(0.5, -0.5, 1) + vec3(0.5, 0.5, 0), 0, 1);

	vec2 HiZSize = textureSize(DepthPyramid, 0);

	// Occlusion Culling
	if(IsVisible && MeshCullingCommonInput.OcclusionCullingEnabled)
	{
		vec2  BoxSize = (NewMax.xy - NewMin.xy) * HiZSize;
		float Lod = ceil(log2(max(BoxSize.x, BoxSize.y)));

		vec4 BoxMinSS = Proj * View * vec4(BoxMin, 1);
		     BoxMinSS.xyz /= BoxMinSS.w;

		vec4 BoxMaxSS = Proj * View * vec4(BoxMax, 1);
		     BoxMaxSS.xyz /= BoxMaxSS.w;

		float a = textureLod(DepthPyramid, BoxMinSS.xy * vec2(0.5, -0.5) + 0.5, Lod).x;
		float b = textureLod(DepthPyramid, vec2(BoxMinSS.x, BoxMaxSS.y) * vec2(0.5, -0.5) + 0.5, Lod).x;
		float c = textureLod(DepthPyramid, vec2(BoxMaxSS.x, BoxMinSS.y) * vec2(0.5, -0.5) + 0.5, Lod).x;
		float d = textureLod(DepthPyramid, BoxMaxSS.xy * vec2(0.5, -0.5) + 0.5, Lod).x;

		IsVisible = IsVisible && (NewMin.z - 0.005 <= (min(min(min(a, b), c), d)));
	}

	MeshDrawVisibilityData[DrawIndex] = IsVisible;
}
