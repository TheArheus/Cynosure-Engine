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

struct mesh_draw_command
{
	material Mat;
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
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
layout(binding = 5) buffer b4 { mesh_draw_command MeshDrawCommands[]; };
layout(push_constant) uniform pushConstant { uint DrawCount; uint MeshCount; };


void main()
{
	uint DrawIndex = gl_GlobalInvocationID.x;
	if(DrawIndex >= DrawCount) return;

	uint CommandIdx = MeshDrawCommandData[DrawIndex].MeshIndex - 1;
	if(DrawIndex == 0)
	{
		for(uint MI = 0; MI < MeshCount; ++MI)
		{
			IndirectDrawIndexedCommands[MI].InstanceCount = 0;
		}
		IndirectDrawIndexedCommandsCounter = MeshCount;
	}

	if(MeshDrawVisibilityData[DrawIndex] == 0) return;

	uint InstanceIdx = atomicAdd(IndirectDrawIndexedCommands[CommandIdx].InstanceCount, 1);

	IndirectDrawIndexedCommands[CommandIdx].VertexOffset  = int(MeshOffsets[CommandIdx].VertexOffset);
	IndirectDrawIndexedCommands[CommandIdx].FirstIndex    = MeshOffsets[CommandIdx].IndexOffset;
	IndirectDrawIndexedCommands[CommandIdx].IndexCount    = MeshOffsets[CommandIdx].IndexCount;
	IndirectDrawIndexedCommands[CommandIdx].FirstInstance = CommandIdx == 0 ? 0 : IndirectDrawIndexedCommands[CommandIdx - 1].FirstInstance + IndirectDrawIndexedCommands[CommandIdx - 1].InstanceCount;

	InstanceIdx += IndirectDrawIndexedCommands[CommandIdx].FirstInstance;
	MeshDrawCommands[InstanceIdx].Mat       = MeshDrawCommandData[DrawIndex].Mat;
	MeshDrawCommands[InstanceIdx].Translate = MeshDrawCommandData[DrawIndex].Translate;
	MeshDrawCommands[InstanceIdx].Scale     = MeshDrawCommandData[DrawIndex].Scale;
}
