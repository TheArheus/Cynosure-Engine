#pragma once

struct blur : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture Input;
		gpu_texture Output;
	};

	blur()
	{
		Shader = "../shaders/blur.comp.glsl";
	}
};
