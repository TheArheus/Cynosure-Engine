#pragma once

struct ssao : public shader_compute_view_context
{
	shader_input() parameters
	{
		gpu_buffer  WorldUpdateBuffer;
		gpu_buffer  RandomSamplesBuffer;
		gpu_texture NoiseTexture;
		gpu_texture DepthTarget;
		gpu_texture_array GBuffer;
		gpu_texture Output;
	};
#if 0
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
