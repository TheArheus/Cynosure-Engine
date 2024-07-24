#pragma once

struct frustum_culling : public shader_compute_view_context
{
	struct input_type
	{
		buffer_ref MeshCommonCullingInputBuffer;
		buffer_ref GeometryOffsets;
		buffer_ref MeshDrawCommandDataBuffer;
		buffer_ref MeshDrawVisibilityDataBuffer;
	};

	struct output_type
	{
		buffer_ref IndirectDrawIndexedCommands;
		buffer_ref MeshDrawCommandBuffer;
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
		buffer_ref  MeshCommonCullingInputBuffer;
		buffer_ref  GeometryOffsets;
		buffer_ref  MeshDrawCommandDataBuffer;
		texture_ref DepthPyramid;
	};

	struct output_type
	{
		buffer_ref MeshDrawVisibilityDataBuffer;
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
		buffer_ref MeshCommonCullingInputBuffer;
		buffer_ref GeometryOffsets;
		buffer_ref MeshDrawCommandDataBuffer;
		buffer_ref MeshDrawVisibilityDataBuffer;
	};

	struct output_type
	{
		buffer_ref IndirectDrawIndexedCommands;
		buffer_ref MeshDrawCommandBuffer;
	};

	generate_all()
	{
		Shader = "../shaders/mesh.dbg.comp.glsl";
	}
};
