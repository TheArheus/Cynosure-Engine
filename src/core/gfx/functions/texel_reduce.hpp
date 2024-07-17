#pragma once

struct texel_reduce_2d : shader_compute_view_context
{
	struct input_type
	{
		texture_ref Input;
	};

	struct output_type
	{
		texture_ref Output;
	};

	texel_reduce_2d()
	{
		Shader = "../shaders/texel_reduce_2d.comp.glsl";
	}
};

struct texel_reduce_3d : shader_compute_view_context
{
	struct input_type
	{
		texture_ref Input;
	};

	struct output_type
	{
		texture_ref Output;
	};

	texel_reduce_3d()
	{
		Shader = "../shaders/texel_reduce_3d.comp.glsl";
	}
};
