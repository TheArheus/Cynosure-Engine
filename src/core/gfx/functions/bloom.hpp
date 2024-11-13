#pragma once

struct bloom_combine : public shader_compute_view_context
{
	struct parameter_type
	{
		texture_ref A;
		texture_ref B;
		texture_ref Output;
	};

	shader_input() parameters
	{
		texture_ref A;
		texture_ref B;
		texture_ref Out;
	};

	bloom_combine()
	{
		Shader = "../shaders/bloom_combine.comp.glsl";
	}
};

struct bloom_downsample : public shader_compute_view_context
{
	struct parameter_type
	{
		texture_ref Input;
		texture_ref Output;
	};

	shader_input() parameters
	{
		texture_ref Input;
		texture_ref Output;
	};

	bloom_downsample()
	{
		Shader = "../shaders/bloom_down.comp.glsl";
	}
};

struct bloom_upsample : public shader_compute_view_context
{
	struct parameter_type
	{
		texture_ref A;
		texture_ref B;
		texture_ref Output;
	};

	shader_input() parameters
	{
		texture_ref A;
		texture_ref B;
		texture_ref Output;
	};

	bloom_upsample()
	{
		Shader = "../shaders/bloom_up.comp.glsl";
	}
};
