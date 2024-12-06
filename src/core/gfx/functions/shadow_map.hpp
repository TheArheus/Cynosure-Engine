#pragma once

struct mesh_shadow : public shader_graphics_view_context
{
	shader_input() parameters
	{
		gpu_buffer VertexBuffer;
		gpu_buffer CommandBuffer;
		gpu_buffer GeometryOffsets;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseDepth = true;
		SetupData.CullMode = cull_mode::front;

		return SetupData;
	}

	mesh_shadow()
	{
		Shaders = {"../shaders/mesh.sdw.vert.glsl", "../shaders/mesh.sdw.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

#if 0
// TODO: Do this the right way
template<u32 FaceIdx = 0>
struct point_shadow : public shader_graphics_view_context
{
	shader_input() parameters
	{
		gpu_buffer VertexBuffer;
		gpu_buffer CommandBuffer;
		gpu_buffer GeometryOffsets;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseDepth = true;
		SetupData.ViewMask = 1 << FaceIdx;
		SetupData.CullMode = cull_mode::front;

		return SetupData;
	}

	point_shadow()
	{
		Shaders = {"../shaders/mesh.pnt.sdw.vert.glsl", "../shaders/mesh.pnt.sdw.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};
#endif

struct depth_prepass : public shader_graphics_view_context
{
	struct parameters
	{
		gpu_buffer VertexBuffer;
		gpu_buffer CommandBuffer;
		gpu_buffer GeometryOffsets;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseDepth = true;

		return SetupData;
	}

	depth_prepass()
	{
		Shaders = {"../shaders/mesh.sdw.vert.glsl", "../shaders/mesh.sdw.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};
