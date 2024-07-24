#pragma once

struct ssao : public shader_compute_view_context
{
	struct input_type
	{
		buffer_ref WorldUpdateBuffer;
		buffer_ref RandomSamplesBuffer;
		texture_ref NoiseTexture;
		texture_ref DepthTarget;
		texture_ref GBuffer;
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
