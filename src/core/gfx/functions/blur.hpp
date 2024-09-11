#pragma once

struct blur : public shader_compute_view_context
{
	struct parameter_type
	{
		texture_ref Input;
		texture_ref Output;
	};

	introspect() struct parameters
	{
		texture_ref Input;
		texture_ref Output;
	};

	blur()
	{
		Shader = "../shaders/blur.comp.glsl";
	}
};
