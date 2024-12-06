#pragma once

struct debug_raster : public shader_graphics_view_context
{
	shader_input() parameters
	{
		gpu_buffer WorldUpdateBuffer;
		gpu_buffer VertexBuffer;
		gpu_buffer MeshDrawCommandBuffer;
		gpu_buffer MeshMaterialsBuffer;
		gpu_buffer GeometryOffsets;
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
	shader_input() parameters
	{
		gpu_buffer WorldUpdateBuffer;
		gpu_buffer VertexBuffer;
		gpu_buffer MeshDrawCommandBuffer;
		gpu_buffer MeshMaterialsBuffer;
		gpu_buffer GeometryOffsets;

		gpu_texture_array DiffuseTextures;
		gpu_texture_array NormalTextures;
		gpu_texture_array SpecularTextures;
		gpu_texture_array HeightTextures;
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
	shader_input() parameters
	{
		gpu_buffer WorldUpdateBuffer;
		gpu_buffer VertexBuffer;
		gpu_buffer MeshDrawCommandBuffer;
		gpu_buffer MeshMaterialsBuffer;
		gpu_buffer GeometryOffsets;
		gpu_buffer LightSources;
		gpu_texture VoxelGrid;
		gpu_texture VoxelGridNormal;

		gpu_texture_array DiffuseTextures;
		gpu_texture_array NormalTextures;
		gpu_texture_array SpecularTextures;
		gpu_texture_array HeightTextures;
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
		Defines = {{STRINGIFY(VOXEL_SIZE), std::to_string(VOXEL_SIZE)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}};
		LoadOp  = load_op::clear;
		StoreOp = store_op::store;
	}
};

struct color_pass : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_buffer WorldUpdateBuffer;
		gpu_buffer LightSourcesBuffer;
		gpu_buffer PoissonDiskBuffer;
		gpu_buffer RandomSamplesBuffer;

		gpu_texture PrevColorTarget;
		gpu_texture GfxDepthTarget;
		gpu_texture VolumetricLightTexture;
		gpu_texture IndirectLightTexture;
		gpu_texture RandomAnglesTexture;
		gpu_texture_array GBuffer;
		gpu_texture AmbientOcclusionData;
		gpu_texture_array GlobalShadow;

		gpu_texture HdrOutput;
		gpu_texture BrightOutput;

		gpu_texture_array LightShadows;
		gpu_texture_array PointLightShadows;
	};

	color_pass()
	{
		Shader = "../shaders/color_pass.comp.glsl";
		Defines = {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};

struct voxel_indirect_light_calc : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_buffer WorldUpdateBuffer;
		gpu_texture DepthTarget;
		gpu_texture_array GBuffer;
		gpu_texture VoxelGrid;
		gpu_texture VoxelGridNormal;
		gpu_texture Out;
	};

	voxel_indirect_light_calc()
	{
		Shader = "../shaders/voxel_indirect_light_calc.comp.glsl";
		Defines = {{STRINGIFY(VOXEL_SIZE), std::to_string(VOXEL_SIZE)}, {STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};

struct volumetric_light_calc : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_buffer WorldUpdateBuffer;
		gpu_texture DepthTarget;
		gpu_texture_array GBuffer;
		gpu_texture_array GlobalShadow;
		gpu_texture Output;
	};

	volumetric_light_calc()
	{
		Shader  = "../shaders/volumetric_light_calc.comp.glsl";
		Defines = {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};

struct textures_combine : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture A;
		gpu_texture B;
		gpu_texture Output;
	};

	textures_combine()
	{
		Shader = "../shaders/texture_combine.comp.glsl";
	}
};
