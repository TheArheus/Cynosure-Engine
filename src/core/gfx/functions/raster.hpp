#pragma once

struct debug_raster : public shader_graphics_view_context
{
	struct input_type
	{
		buffer* WorldUpdateBuffer;
		buffer* VertexBuffer;
		buffer* MeshDrawCommandBuffer;
		buffer* MeshMaterialsBuffer;
		buffer* GeometryOffsets;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseColor	 = true;
		SetupData.UseDepth	 = true;
		SetupData.CullMode   = cull_mode::back;
		SetupData.UseOutline = true;

		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::B8G8R8A8_UNORM};
	}

	debug_raster()
	{
		Shaders = {"../shaders/mesh.dbg.vert.glsl", "../shaders/mesh.dbg.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

struct gbuffer_raster : public shader_graphics_view_context
{
	struct input_type
	{
		buffer* WorldUpdateBuffer;
		buffer* VertexBuffer;
		buffer* MeshDrawCommandBuffer;
		buffer* MeshMaterialsBuffer;
		buffer* GeometryOffsets;
	};

	struct static_storage_type
	{
		std::vector<texture*> DiffuseTextures;
		std::vector<texture*> NormalTextures;
		std::vector<texture*> SpecularTextures;
		std::vector<texture*> HeightTextures;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseColor	 = true;
		SetupData.UseDepth	 = true;
		SetupData.CullMode   = cull_mode::back;

		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::R16G16B16A16_SNORM, image_format::R16G16B16A16_SNORM, image_format::R8G8B8A8_UNORM, image_format::R8G8B8A8_UNORM, image_format::R32G32_SFLOAT};
	}

	gbuffer_raster()
	{
		Shaders = {"../shaders/mesh.vert.glsl", "../shaders/mesh.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

struct voxelization : public shader_graphics_view_context
{
	struct input_type
	{
		buffer* WorldUpdateBuffer;
		buffer* VertexBuffer;
		buffer* MeshDrawCommandBuffer;
		buffer* MeshMaterialsBuffer;
		buffer* GeometryOffsets;
		buffer* LightSources;
	};

	struct output_type
	{
		texture_ref VoxelGrid;
	};

	struct static_storage_type
	{
		std::vector<texture*> DiffuseTextures;
		std::vector<texture*> NormalTextures;
		std::vector<texture*> SpecularTextures;
		std::vector<texture*> HeightTextures;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};

		SetupData.UseConservativeRaster = true;

		return SetupData;
	}

	voxelization()
	{
		Shaders = {"../shaders/voxel.vert.glsl", "../shaders/voxel.geom.glsl", "../shaders/voxel.frag.glsl"};
		Defines = {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

struct color_pass : public shader_compute_view_context
{
	struct input_type
	{
		buffer* WorldUpdateBuffer;
		buffer* LightSourcesBuffer;
		buffer* PoissonDiskBuffer;
		buffer* RandomSamplesBuffer;
		texture_ref GfxColorTarget;
		texture_ref GfxDepthTarget;
		texture_ref VoxelGridTarget;
		texture_ref RandomAnglesTexture;
		std::vector<texture*> GBuffer;
		texture_ref AmbientOcclusionData;
		std::vector<texture*> GlobalShadow;
	};

	struct output_type
	{
		texture_ref HdrOutput;
		texture_ref BrightOutput;
	};

	struct static_storage_type
	{
		std::vector<texture*> LightShadows;
		std::vector<texture*> PointLightShadows;
	};

	color_pass()
	{
		Shader = "../shaders/color_pass.comp.glsl";
		Defines = {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};
