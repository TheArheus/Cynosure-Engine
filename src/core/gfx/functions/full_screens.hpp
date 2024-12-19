
// TODO: something better with full screen passes
struct full_screen_pass_color : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_color_target ColorTarget;
	};

	shader_input() parameters
	{
		gpu_buffer Vertices;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};
		SetupData.UseColor = true;
		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::B8G8R8A8_UNORM};
	}

	full_screen_pass_color()
	{
		Shaders = {"../shaders/primitive/full_screen.vert.glsl", "../shaders/primitive/full_screen_color.frag.glsl"};
		LoadOp  = load_op::load;
		StoreOp = store_op::store;
	}
};

// TODO: something better with full screen passes
struct full_screen_pass_circle : public shader_graphics_view_context
{
	shader_input() raster_parameters
	{
		gpu_index_buffer IndexBuffer;
		gpu_color_target ColorTarget;
	};

	shader_input() parameters
	{
		gpu_buffer Vertices;
	};

	utils::render_context::input_data SetupPipelineState() override
	{
		utils::render_context::input_data SetupData = {};
		SetupData.UseColor = true;
		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::B8G8R8A8_UNORM};
	}

	full_screen_pass_circle()
	{
		Shaders = {"../shaders/primitive/full_screen.vert.glsl", "../shaders/primitive/full_screen_circle.frag.glsl"};
		LoadOp  = load_op::load;
		StoreOp = store_op::store;
	}
};

// TODO: something better with full screen passes
struct full_screen_pass_texture : public shader_graphics_view_context
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
		return SetupData;
	}

	std::vector<image_format> SetupAttachmentDescription() override
	{
		return {image_format::B8G8R8A8_UNORM};
	}

	full_screen_pass_texture()
	{
		Shaders = {"../shaders/primitive/full_screen.vert.glsl", "../shaders/primitive/full_screen_texture.frag.glsl"};
		LoadOp  = load_op::load;
		StoreOp = store_op::store;
	}
};
