#pragma once

struct frustum_culling : public shader_compute_view_context
{
	struct input_type
	{
		buffer* MeshCommonCullingInputBuffer;
		buffer* GeometryOffsets;
		buffer* MeshDrawCommandDataBuffer;
		buffer* MeshDrawVisibilityDataBuffer;
	};

	struct output_type
	{
		buffer* IndirectDrawIndexedCommands;
		buffer* MeshDrawCommandBuffer;
	};

	frustum_culling()
	{
		Shader  = "../shaders/indirect_cull_frust.comp.glsl";
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};

struct occlusion_culling : public shader_compute_view_context 
{
	struct input_type
	{
		buffer*  MeshCommonCullingInputBuffer;
		buffer*  GeometryOffsets;
		buffer*  MeshDrawCommandDataBuffer;
		texture_ref DepthPyramid;
	};

	struct output_type
	{
		buffer* MeshDrawVisibilityDataBuffer;
	};

public:
	occlusion_culling()
	{
		Shader = "../shaders/indirect_cull_occl.comp.glsl";
	}
};

struct generate_all : public shader_compute_view_context
{
	struct input_type
	{
		buffer* MeshCommonCullingInputBuffer;
		buffer* GeometryOffsets;
		buffer* MeshDrawCommandDataBuffer;
		buffer* MeshDrawVisibilityDataBuffer;
	};

	struct output_type
	{
		buffer* IndirectDrawIndexedCommands;
		buffer* MeshDrawCommandBuffer;
	};

	generate_all()
	{
		Shader = "../shaders/mesh.dbg.comp.glsl";
	}
};
