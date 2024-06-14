#version 450

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_atomic_float: require
#extension GL_EXT_shader_atomic_int64: require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;


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
	uint DrawID;
    uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    int  VertexOffset;
    uint FirstInstance;
};

layout(binding = 0) buffer b0 { global_world_data WorldUpdate; };
layout(binding = 1) buffer readonly b1 { mesh_comp_culling_common_input MeshCullingCommonInput; };
layout(binding = 2) buffer b2 { offset MeshOffsets[]; };
layout(binding = 3) buffer readonly b3 { mesh_draw_command MeshDrawCommandData[]; };
layout(binding = 4) buffer readonly b4 { bool MeshDrawVisibilityData[]; };
layout(binding = 5) buffer b5 { indirect_draw_indexed_command IndirectDrawIndexedCommands[]; };
layout(binding = 6) buffer c0 { uint IndirectDrawIndexedCommandsCounter; };
layout(binding = 7) buffer b6 { mesh_draw_command MeshDrawCommands[]; };

layout(push_constant) uniform pushConstant { uint DrawCount; uint MeshCount; };

shared int intVariable;

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

	IndirectDrawIndexedCommands[CommandIdx].DrawID     = CommandIdx;
	IndirectDrawIndexedCommands[CommandIdx].IndexCount = MeshOffsets[CommandIdx].IndexCount;
	IndirectDrawIndexedCommands[CommandIdx].FirstIndex = MeshOffsets[CommandIdx].IndexOffset;

	if(!MeshDrawVisibilityData[DrawIndex])
	{
		return;
	}

	mat4 Proj = MeshCullingCommonInput.Proj;
	mat4 View = MeshCullingCommonInput.View;

	vec3 _SphereCenter = MeshOffsets[CommandIdx].BoundingSphere.Center.xyz * MeshDrawCommandData[DrawIndex].Scale.xyz + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float SphereRadius = MeshOffsets[CommandIdx].BoundingSphere.Radius * MeshDrawCommandData[DrawIndex].Scale.x;
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
		uint InstanceIdx = atomicAdd(IndirectDrawIndexedCommands[CommandIdx].InstanceCount, 1);

		InstanceIdx += MeshOffsets[CommandIdx].InstanceOffset;
		MeshDrawCommands[InstanceIdx].MeshIndex	= CommandIdx;
		MeshDrawCommands[InstanceIdx].Translate = MeshDrawCommandData[DrawIndex].Translate;
		MeshDrawCommands[InstanceIdx].Scale     = MeshDrawCommandData[DrawIndex].Scale;
		MeshDrawCommands[InstanceIdx].Rotate    = MeshDrawCommandData[DrawIndex].Rotate;
	}
}
