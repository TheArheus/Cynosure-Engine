// TODO: BILLBOARDS

struct full_screen : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_color_target ColorTarget;
	};

	shader_input() parameters
	{
		gpu_buffer Vertices;
		gpu_texture Texture;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};
		SetupData.UseColor = true;
		SetupData.UseBlend = true;
		SetupData.Topology = topology::triangle_list;
		SetupData.BlendSrc = blend_factor::src_alpha;
		SetupData.BlendDst = blend_factor::one_minus_src_alpha;
		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::B8G8R8A8_UNORM};
	}

	full_screen()
	{
		Shaders = {"../shaders/primitive/full_screen.vert.glsl", "../shaders/primitive/full_screen_texture.frag.glsl"};
		LoadOp  = load_op::load;
		StoreOp = store_op::store;
	}
};

struct primitive_2d : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_color_target ColorTarget;
	};

	shader_input() parameters
	{
		gpu_buffer Vertices;
		gpu_texture Texture;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};
		SetupData.UseColor = true;
		SetupData.UseBlend = true;
		SetupData.Topology = topology::triangle_list;
		SetupData.BlendSrc = blend_factor::src_alpha;
		SetupData.BlendDst = blend_factor::one_minus_src_alpha;
		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::B8G8R8A8_UNORM};
	}

	primitive_2d()
	{
		Shaders = {"../shaders/primitive/primitive.2d.vert.glsl", "../shaders/primitive/primitive.2d.frag.glsl"};
		LoadOp  = load_op::load;
		StoreOp = store_op::store;
	}
};
