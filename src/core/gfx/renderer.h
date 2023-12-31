#pragma once

class global_graphics_context
{
	memory_heap* GlobalHeap;

	global_graphics_context(const global_graphics_context&) = delete;
	global_graphics_context& operator=(const global_graphics_context&) = delete;

public:
	global_graphics_context() = default;
	global_graphics_context(renderer_backend* NewBackend, backend_type BackendType);

	global_graphics_context(global_graphics_context&& Oth) noexcept;
	global_graphics_context& operator=(global_graphics_context&& Oth) noexcept;

	buffer* PushBuffer(u64 DataSize, bool NewWithCounter, u32 Flags)
	{
		return GlobalHeap->PushBuffer(Backend, DataSize, NewWithCounter, Flags);
	}

	buffer* PushBuffer(void* Data, u64 DataSize, bool NewWithCounter, u32 Flags)
	{
		return GlobalHeap->PushBuffer(Backend, Data, DataSize, NewWithCounter, Flags);
	}

	texture* PushTexture(u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		return GlobalHeap->PushTexture(Backend, Width, Height, Depth, InputData);
	}

	texture* PushTexture(void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		return GlobalHeap->PushTexture(Backend, Data, Width, Height, Depth, InputData);
	}

	memory_heap* CreateMemoryHeap()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_memory_heap(Backend);
			default:
				return nullptr;
		}
	}

	global_pipeline_context* CreateGlobalPipelineContext()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_global_pipeline_context(Backend);
			default:
				return nullptr;
		}
	}

	render_context* CreateRenderContext(std::initializer_list<const std::string> ShaderList, const std::vector<texture*>& ColorTargets, 
										const utils::render_context::input_data& InputData = {true, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {})
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_render_context(Backend, ShaderList, ColorTargets, InputData, ShaderDefines);
			default:
				return nullptr;
		}
	}

	compute_context* CreateComputeContext(const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {})
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_compute_context(Backend, Shader, ShaderDefines);
			default:
				return nullptr;
		}
	}

	shader_input* CreateShaderInput()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_shader_input;
			default:
				return nullptr;
		}
	}

	renderer_backend* Backend;
	backend_type BackendType;

	std::vector<texture*> SwapchainImages;

	texture* GfxColorTarget;
	texture* GfxDepthTarget;
	texture* DebugCameraViewDepthTarget;

	std::vector<texture*> GlobalShadow;
	std::vector<texture*> GBuffer;

	texture* AmbientOcclusionData;
	texture* DepthPyramid;
	texture* RandomAnglesTexture;
	texture* NoiseTexture;

	buffer* PoissonDiskBuffer;
	buffer* RandomSamplesBuffer;

	std::vector<render_context*> CubeMapShadowContexts;
	render_context* GfxContext;
	render_context* CascadeShadowContext;
	render_context* ShadowContext;
	render_context* DebugCameraViewContext;
	render_context* DebugContext;

	compute_context* ColorPassContext;
	compute_context* AmbientOcclusionContext;
	compute_context* ShadowComputeContext;
	compute_context* FrustCullingContext;
	compute_context* OcclCullingContext;
	compute_context* DepthReduceContext;
	compute_context* BlurContext;
	compute_context* DebugComputeContext;
};
