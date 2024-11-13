#pragma once

struct frustum_culling : public shader_compute_view_context
{
	struct parameter_type
	{
		buffer_ref MeshCommonCullingInputBuffer;
		buffer_ref GeometryOffsets;
		buffer_ref MeshDrawCommandDataBuffer;
		buffer_ref MeshDrawVisibilityDataBuffer;
		buffer_ref IndirectDrawIndexedCommands;
		buffer_ref MeshDrawCommandBuffer;
	};

	shader_input() parameters
	{
		mesh_comp_culling_common_input MeshCommonCullingData;
		mesh::offset GeometryOffsets;
		mesh_draw_command MeshDrawCommandData;
		u32 MeshDrawVisibilityData;
		indirect_draw_indexed_command IndirectDrawIndexedCommands; // TODO: With counter
		mesh_draw_command MeshDrawCommands;
	};

	frustum_culling()
	{
		Shader  = "../shaders/indirect_cull_frust.comp.glsl";
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};

struct occlusion_culling : public shader_compute_view_context 
{
	struct parameter_type
	{
		buffer_ref  MeshCommonCullingInputBuffer;
		buffer_ref  GeometryOffsets;
		buffer_ref  MeshDrawCommandDataBuffer;
		texture_ref DepthPyramid;
		buffer_ref  MeshDrawVisibilityDataBuffer;
	};

	shader_input() parameters
	{
		mesh_comp_culling_common_input MeshCommonCullingData;
		mesh::offset GeometryOffsets;
		mesh_draw_command MeshDrawCommandData;
		texture_ref DepthPyramid;
		u32 MeshDrawVisibilityData;
	};

public:
	occlusion_culling()
	{
		Shader = "../shaders/indirect_cull_occl.comp.glsl";
	}
};

struct generate_all : public shader_compute_view_context
{
	struct parameter_type
	{
		buffer_ref MeshCommonCullingInputBuffer;
		buffer_ref GeometryOffsets;
		buffer_ref MeshDrawCommandDataBuffer;
		buffer_ref MeshDrawVisibilityDataBuffer;
		buffer_ref IndirectDrawIndexedCommands;
		buffer_ref MeshDrawCommandBuffer;
	};

	shader_input() parameters
	{
		mesh_comp_culling_common_input MeshCommonCullingData;
		mesh::offset GeometryOffsets;
		mesh_draw_command MeshDrawCommandDataBuffer;
		u32 MeshDrawVisibilityDataBuffer;
		indirect_draw_indexed_command IndirectDrawIndexedCommands; // TODO: With counter
		mesh_draw_command MeshDrawCommandBuffer;
	};

	generate_all()
	{
		Shader = "../shaders/mesh.dbg.comp.glsl";
	}
};
