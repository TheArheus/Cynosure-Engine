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

#if 1
	shader_input() parameters
	{
		global_world_data WorldUpdate;
		vec4 RandomSamples[64];
		texture_ref NoiseTexture;
		texture_ref DepthTarget;
		texture_ref GBuffer;
		texture_ref Output;
	};
#else
	shader_input_begin(parameters)
		shader_param_var(global_world_data, WorldUpdate);
		shader_param_var(vec4, RandomSamples[64]);
		shader_param_texture(texture_ref, NoiseTexture);
		shader_param_texture(texture_ref, DepthTarget);
		shader_param_texture(texture_ref, GBuffer);
		shader_param_texture(texture_ref, Output);
	shader_input_end();
#endif

	ssao()
	{
		Shader = "../shaders/screen_space_ambient_occlusion.comp.glsl";
		Defines = {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}};
	}
};
