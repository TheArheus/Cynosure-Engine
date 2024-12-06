#pragma once

struct texel_reduce_2d : shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture Input;
		gpu_texture Output;
	};

	texel_reduce_2d()
	{
		Shader = "../shaders/texel_reduce_2d.comp.glsl";
	}
};

struct texel_reduce_3d : shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture Input;
		gpu_texture Output;
	};

	texel_reduce_3d()
	{
		Shader = "../shaders/texel_reduce_3d.comp.glsl";
	}
};
