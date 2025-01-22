#pragma once

struct mesh_depth : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_indirect_buffer IndirectBuffer;
		gpu_depth_target DepthTarget;
	};

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
		SetupData.Topology = topology::triangle_list;

		return SetupData;
	}

	mesh_depth()
	{
		Shaders = {"../shaders/mesh.sdw.vert.glsl", "../shaders/empty.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

struct mesh_depth_variance_exp : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_indirect_buffer IndirectBuffer;
		gpu_color_target ColorTarget;
		gpu_depth_target DepthTarget;
	};

	shader_input() parameters
	{
		gpu_buffer VertexBuffer;
		gpu_buffer CommandBuffer;
		gpu_buffer GeometryOffsets;
	};

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return { image_format::R32G32B32A32_SFLOAT };
	}

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseColor = true;
		SetupData.UseDepth = true;
		SetupData.CullMode = cull_mode::front;
		SetupData.Topology = topology::triangle_list;

		return SetupData;
	}

	mesh_depth_variance_exp()
	{
		Shaders = {"../shaders/mesh.sdw.vert.glsl", "../shaders/mesh.sdw.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

struct mesh_depth_cubemap : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_indirect_buffer IndirectBuffer;
		gpu_depth_target DepthTarget;
	};

	shader_input() parameters
	{
		gpu_buffer VertexBuffer;
		gpu_buffer CommandBuffer;
		gpu_buffer GeometryOffsets;
		gpu_buffer LightSourcesMatrixBuffer;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseDepth = true;
		//SetupData.ViewMask = 1 << FaceIdx;
		SetupData.CullMode = cull_mode::front;
		SetupData.Topology = topology::triangle_list;

		return SetupData;
	}

	mesh_depth_cubemap()
	{
		Shaders = {"../shaders/mesh.pnt.sdw.vert.glsl", "../shaders/mesh.pnt.sdw.geom.glsl", "../shaders/mesh.pnt.sdw.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};
