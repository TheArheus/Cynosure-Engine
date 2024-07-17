#pragma once

struct ssao : public shader_compute_view_context
{
	struct input_type
	{
		buffer* WorldUpdateBuffer;
		buffer* RandomSamplesBuffer;
		texture_ref NoiseTexture;
		texture_ref DepthTarget;
		std::vector<texture*> GBuffer;
	};

	struct output_type
	{
		texture_ref Output;
	};

	ssao()
	{
		Shader = "../shaders/screen_space_ambient_occlusion.comp.glsl";
		Defines = {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};
