#version 450

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

struct material
{
	vec4  LightEmmit;
	float Specular;
	uint  TextureIdx;
	uint  NormalMapIdx;
	uint  LightType;
};

struct offset
{
	aabb AABB;
	sphere BoundingSphere;

	uint VertexOffset;
	uint VertexCount;

	uint IndexOffset;
	uint IndexCount;
};

struct mesh_draw_command_input
{
	material Mat;
	vec4 Translate;
	vec4 Scale;
	uint MeshIndex;
	bool IsVisible;
};

struct mesh_draw_command
{
	material Mat;
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
};

struct plane
{
	vec4 P;
	vec4 N;
};

struct mesh_comp_culling_common_input
{
	plane Planes[6];
	bool  FrustrumCullingEnabled;
	bool  OcclusionCullingEnabled;
	float HiZWidth;
	float HiZHeight;
	float NearZ;
	uint  DrawCount;
	uint  MeshCount;
	mat4  Proj;
	mat4  View;
};

struct indirect_draw_indexed_command
{
    uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    int  VertexOffset;
    uint FirstInstance;
	uint CommandIdx;
};

layout(binding = 0) buffer readonly b0 { offset MeshOffsets[]; };
layout(binding = 1) buffer readonly b1 { mesh_draw_command_input MeshDrawCommandData[]; };
layout(binding = 2) buffer b2 { indirect_draw_indexed_command IndirectDrawIndexedCommands[]; };
layout(binding = 3) buffer c0 { uint IndirectDrawIndexedCommandsCounter; };
layout(binding = 4) buffer b3 { indirect_draw_indexed_command ShadowIndirectDrawIndexedCommands[]; };
layout(binding = 5) buffer c1 { uint ShadowIndirectDrawIndexedCommandsCounter; };
layout(binding = 6) buffer b4 { mesh_draw_command MeshDrawCommands[]; };
layout(binding = 7) buffer b5 { mesh_draw_command MeshDrawShadowCommands[]; };
layout(binding = 8) buffer readonly b6 { mesh_comp_culling_common_input MeshCullingCommonInput; };

void main()
{
	uint DrawIndex = gl_GlobalInvocationID.x;
	if(DrawIndex >= MeshCullingCommonInput.DrawCount) return;

	uint MeshIndex = MeshDrawCommandData[DrawIndex].MeshIndex - 1;
	if(DrawIndex == 0)
	{
		for(uint MI = 0; MI < MeshCullingCommonInput.MeshCount; ++MI)
		{
			IndirectDrawIndexedCommands[MI].InstanceCount = 0;
			ShadowIndirectDrawIndexedCommands[MI].InstanceCount = 0;
		}
		IndirectDrawIndexedCommandsCounter = MeshCullingCommonInput.MeshCount;
		ShadowIndirectDrawIndexedCommandsCounter = MeshCullingCommonInput.MeshCount;
	}

	{
		uint CommandIdx = MeshIndex;
		uint InstanceIdx = atomicAdd(ShadowIndirectDrawIndexedCommands[MeshIndex].InstanceCount, 1);

		ShadowIndirectDrawIndexedCommands[CommandIdx].IndexCount    = MeshOffsets[MeshIndex].IndexCount;
		ShadowIndirectDrawIndexedCommands[CommandIdx].FirstIndex    = MeshOffsets[MeshIndex].IndexOffset;
		ShadowIndirectDrawIndexedCommands[CommandIdx].VertexOffset  = int(MeshOffsets[MeshIndex].VertexOffset);
		ShadowIndirectDrawIndexedCommands[CommandIdx].FirstInstance = ShadowIndirectDrawIndexedCommands[CommandIdx - 1].FirstInstance + ((CommandIdx - 1) < 0 ? 0 : ShadowIndirectDrawIndexedCommands[CommandIdx - 1].InstanceCount);

		InstanceIdx += ShadowIndirectDrawIndexedCommands[CommandIdx].FirstInstance;
		MeshDrawShadowCommands[InstanceIdx].Mat       = MeshDrawCommandData[DrawIndex].Mat;
		MeshDrawShadowCommands[InstanceIdx].Translate = MeshDrawCommandData[DrawIndex].Translate;
		MeshDrawShadowCommands[InstanceIdx].Scale     = MeshDrawCommandData[DrawIndex].Scale;
	}

	if(!MeshDrawCommandData[DrawIndex].IsVisible)
	{
		return;
	}

	mat4 Proj = MeshCullingCommonInput.Proj;
	mat4 View = MeshCullingCommonInput.View;

	vec3 _SphereCenter = MeshOffsets[MeshIndex].BoundingSphere.Center.xyz * MeshDrawCommandData[DrawIndex].Scale.xyz + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float SphereRadius = MeshOffsets[MeshIndex].BoundingSphere.Radius * MeshDrawCommandData[DrawIndex].Scale.x;
	vec4  SphereCenter = View * vec4(_SphereCenter, 1);

	// Frustum Culling
	bool IsVisible = true;
	if(MeshCullingCommonInput.FrustrumCullingEnabled)
	{
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[0].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[1].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[2].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[3].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[4].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput.Planes[5].N, SphereCenter) > -SphereRadius);
	}

	if(IsVisible)
	{
		uint CommandIdx = MeshIndex;
		uint InstanceIdx = atomicAdd(IndirectDrawIndexedCommands[MeshIndex].InstanceCount, 1);

		IndirectDrawIndexedCommands[CommandIdx].IndexCount    = MeshOffsets[MeshIndex].IndexCount;
		IndirectDrawIndexedCommands[CommandIdx].FirstIndex    = MeshOffsets[MeshIndex].IndexOffset;
		IndirectDrawIndexedCommands[CommandIdx].VertexOffset  = int(MeshOffsets[MeshIndex].VertexOffset);
		IndirectDrawIndexedCommands[CommandIdx].FirstInstance = IndirectDrawIndexedCommands[CommandIdx - 1].FirstInstance + ((CommandIdx - 1) < 0 ? 0 : IndirectDrawIndexedCommands[CommandIdx - 1].InstanceCount);

		InstanceIdx += IndirectDrawIndexedCommands[CommandIdx].FirstInstance;
		MeshDrawCommands[InstanceIdx].Mat = MeshDrawCommandData[DrawIndex].Mat;
		MeshDrawCommands[InstanceIdx].Translate = MeshDrawCommandData[DrawIndex].Translate;
		MeshDrawCommands[InstanceIdx].Scale = MeshDrawCommandData[DrawIndex].Scale;
	}
}

