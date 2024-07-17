#pragma once

struct blur : public shader_compute_view_context
{
	struct input_type
	{
		texture_ref Input;
	};

	struct output_type
	{
		texture_ref Output;
	};

	blur()
	{
		Shader = "../shaders/blur.comp.glsl";
	}
};
