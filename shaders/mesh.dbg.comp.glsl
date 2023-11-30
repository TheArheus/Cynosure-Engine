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

struct plane
{
	vec4 P;
	vec4 N;
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

struct mesh_draw_command
{
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
	uint MeshIndex;
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

struct mesh_comp_culling_common_input
{
	plane Planes[6];
	bool  FrustrumCullingEnabled;
	bool  OcclusionCullingEnabled;
	float NearZ;
	uint  DrawCount;
	uint  MeshCount;
	uint  DebugDrawCount;
	uint  DebugMeshCount;
	mat4  Proj;
	mat4  View;
};

layout(binding = 0, std430) uniform readonly b0 { mesh_comp_culling_common_input MeshCullingCommonInput; };
layout(binding = 1) buffer readonly b1 { offset MeshOffsets[]; };
layout(binding = 2) buffer readonly b2 { mesh_draw_command MeshDrawCommandData[]; };
layout(binding = 3) buffer readonly b3 { uint MeshDrawVisibilityData[]; };
layout(binding = 4) buffer b4 { indirect_draw_indexed_command IndirectDrawIndexedCommands[]; };
layout(binding = 5) buffer c0 { uint IndirectDrawIndexedCommandsCounter; };
layout(binding = 6) buffer b5 { mesh_draw_command MeshDrawCommands[]; };


void main()
{
	uint DrawIndex = gl_GlobalInvocationID.x;
	if(DrawIndex >= MeshCullingCommonInput.DebugDrawCount) return;

	uint CommandIdx = MeshDrawCommandData[DrawIndex].MeshIndex - 1;
	if(DrawIndex == 0)
	{
		for(uint MI = 0; MI < MeshCullingCommonInput.DebugMeshCount; ++MI)
		{
			IndirectDrawIndexedCommands[MI].InstanceCount = 0;
		}
		IndirectDrawIndexedCommandsCounter = MeshCullingCommonInput.DebugMeshCount;
	}

	if(MeshDrawVisibilityData[DrawIndex] == 0) return;

	uint InstanceIdx = atomicAdd(IndirectDrawIndexedCommands[CommandIdx].InstanceCount, 1);

	IndirectDrawIndexedCommands[CommandIdx].VertexOffset  = int(MeshOffsets[CommandIdx].VertexOffset);
	IndirectDrawIndexedCommands[CommandIdx].FirstIndex    = MeshOffsets[CommandIdx].IndexOffset;
	IndirectDrawIndexedCommands[CommandIdx].IndexCount    = MeshOffsets[CommandIdx].IndexCount;
	IndirectDrawIndexedCommands[CommandIdx].FirstInstance = CommandIdx == 0 ? 0 : IndirectDrawIndexedCommands[CommandIdx - 1].FirstInstance + IndirectDrawIndexedCommands[CommandIdx - 1].InstanceCount;

	InstanceIdx += IndirectDrawIndexedCommands[CommandIdx].FirstInstance;
	MeshDrawCommands[InstanceIdx].MeshIndex   = CommandIdx;
	MeshDrawCommands[InstanceIdx].Translate = MeshDrawCommandData[DrawIndex].Translate;
	MeshDrawCommands[InstanceIdx].Scale     = MeshDrawCommandData[DrawIndex].Scale;
}
