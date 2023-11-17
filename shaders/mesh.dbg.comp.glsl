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
layout(binding = 2) buffer readonly b2 { uint MeshDrawVisibilityData[]; };
layout(binding = 3) buffer b3 { indirect_draw_indexed_command IndirectDrawIndexedCommands[]; };
layout(binding = 4) buffer c0 { uint IndirectDrawIndexedCommandsCounter; };
layout(push_constant) uniform pushConstant { uint DrawCount; uint MeshCount; bool InstanceCountCalculation; };


void main()
{
	// TODO: More efficient solution using only one pass?
	uint DrawIndex = gl_GlobalInvocationID.x;
	if(DrawIndex >= DrawCount) return;

	uint CommandIdx = MeshDrawCommandData[DrawIndex].MeshIndex - 1;
	if(InstanceCountCalculation)
	{
		if(DrawIndex == 0)
		{
			for(uint MI = 0; MI < MeshCount; ++MI)
			{
				IndirectDrawIndexedCommands[MI].FirstInstance = 0;
				IndirectDrawIndexedCommands[MI].InstanceCount = 0;
			}
			IndirectDrawIndexedCommandsCounter = MeshCount;
		}

		if(MeshDrawVisibilityData[DrawIndex] == 0) return;

		memoryBarrier();
		atomicAdd(IndirectDrawIndexedCommands[CommandIdx].InstanceCount, 1);

		IndirectDrawIndexedCommands[CommandIdx].VertexOffset = int(MeshOffsets[CommandIdx].VertexOffset);
		IndirectDrawIndexedCommands[CommandIdx].FirstIndex   = MeshOffsets[CommandIdx].IndexOffset;
		IndirectDrawIndexedCommands[CommandIdx].IndexCount   = MeshOffsets[CommandIdx].IndexCount;
	}
	else
	{
		if(MeshDrawVisibilityData[DrawIndex] == 0) return;
		if(DrawIndex == 0)
		{
			for(uint MI = 1; MI < MeshCount; ++MI)
			{
				IndirectDrawIndexedCommands[MI].FirstInstance = IndirectDrawIndexedCommands[MI - 1].FirstInstance + IndirectDrawIndexedCommands[MI - 1].InstanceCount;
			}
		}
	}
}
