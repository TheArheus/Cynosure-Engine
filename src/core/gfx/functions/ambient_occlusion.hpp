#pragma once

struct ssao : public shader_compute_view_context
{
	struct parameter_type
	{
		buffer_ref WorldUpdateBuffer;
		buffer_ref RandomSamplesBuffer;
		texture_ref NoiseTexture;
		texture_ref DepthTarget;
		texture_ref GBuffer;
		texture_ref Output;
	};

	introspect()
	shader_input() parameters
	{
		shader_param(test1, test2, test3) 
		global_world_data WorldUpdate;
		vec4 RandomSamples[64];
		texture_ref NoiseTexture;
		texture_ref DepthTarget;
		texture_ref GBuffer;
		texture_ref Output;
	};

	ssao()
	{
		Shader = "../shaders/screen_space_ambient_occlusion.comp.glsl";
		Defines = {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};
