#pragma once

struct frustum_culling : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_buffer MeshCommonCullingInputBuffer;
		gpu_buffer GeometryOffsets;
		gpu_buffer MeshDrawCommandDataBuffer;
		gpu_buffer MeshDrawVisibilityDataBuffer;
		gpu_buffer IndirectDrawIndexedCommands;
		gpu_buffer MeshDrawCommandBuffer;
	};

	frustum_culling()
	{
		Shader  = "../shaders/indirect_cull_frust.comp.glsl";
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};

struct occlusion_culling : public shader_compute_view_context 
{
	shader_input() parameters
	{
		gpu_buffer  MeshCommonCullingInputBuffer;
		gpu_buffer  GeometryOffsets;
		gpu_buffer  MeshDrawCommandDataBuffer;
		gpu_texture DepthPyramid;
		gpu_buffer  MeshDrawVisibilityDataBuffer;
	};

public:
	occlusion_culling()
	{
		Shader = "../shaders/indirect_cull_occl.comp.glsl";
	}
};

struct generate_all : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_buffer MeshCommonCullingInputBuffer;
		gpu_buffer GeometryOffsets;
		gpu_buffer MeshDrawCommandDataBuffer;
		gpu_buffer MeshDrawVisibilityDataBuffer;
		gpu_buffer IndirectDrawIndexedCommands; // TODO: With counter
		gpu_buffer MeshDrawCommandBuffer;
	};

	generate_all()
	{
		Shader = "../shaders/mesh.dbg.comp.glsl";
	}
};
