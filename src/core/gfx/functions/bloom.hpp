#pragma once

struct bloom_combine : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture A;
		gpu_texture B;
		gpu_texture Output;
	};

	bloom_combine()
	{
		Shader = "../shaders/bloom_combine.comp.glsl";
	}
};

struct bloom_downsample : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture Input;
		gpu_texture Output;
	};

	bloom_downsample()
	{
		Shader = "../shaders/bloom_down.comp.glsl";
	}
};

struct bloom_upsample : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_texture A;
		gpu_texture B;
		gpu_texture Output;
	};

	bloom_upsample()
	{
		Shader = "../shaders/bloom_up.comp.glsl";
	}
};
