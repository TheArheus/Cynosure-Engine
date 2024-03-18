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

	buffer* PushBuffer(std::string DebugName, u64 DataSize, u64 Count, bool NewWithCounter, u32 Flags)
	{
		return GlobalHeap->PushBuffer(Backend, DebugName, DataSize, Count, NewWithCounter, Flags);
	}

	buffer* PushBuffer(std::string DebugName, void* Data, u64 DataSize, u64 Count, bool NewWithCounter, u32 Flags)
	{
		return GlobalHeap->PushBuffer(Backend, DebugName, Data, DataSize, Count, NewWithCounter, Flags);
	}

	texture* PushTexture(std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		return GlobalHeap->PushTexture(Backend, DebugName, Width, Height, Depth, InputData);
	}

	texture* PushTexture(std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		return GlobalHeap->PushTexture(Backend, DebugName, Data, Width, Height, Depth, InputData);
	}

	memory_heap* CreateMemoryHeap()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_memory_heap(Backend);
#if _WIN32
			case backend_type::directx12:
				return new directx12_memory_heap(Backend);
#endif
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
#if _WIN32
			case backend_type::directx12:
				return new directx12_global_pipeline_context(Backend);
#endif
			default:
				return nullptr;
		}
	}

	render_context* CreateRenderContext(load_op LoadOp, store_op StoreOp, std::initializer_list<const std::string> ShaderList, const std::vector<texture*>& ColorTargets, 
										const utils::render_context::input_data& InputData = {true, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {})
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_render_context(Backend, LoadOp, StoreOp, ShaderList, ColorTargets, InputData, ShaderDefines);
#if _WIN32
			case backend_type::directx12:
				return new directx12_render_context(Backend, LoadOp, StoreOp, ShaderList, ColorTargets, InputData, ShaderDefines);
#endif
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
#if _WIN32
			case backend_type::directx12:
				return new directx12_compute_context(Backend, Shader, ShaderDefines);
#endif
			default:
				return nullptr;
		}
	}

	renderer_backend* Backend;
	backend_type BackendType;

	std::vector<texture*> SwapchainImages;

	texture* GfxColorTarget[2];
	texture* GfxDepthTarget;
	texture* DebugCameraViewDepthTarget;

	std::vector<texture*> GlobalShadow;
	std::vector<texture*> GBuffer;

	texture* AmbientOcclusionData;
	texture* BlurTemp;
	texture* DepthPyramid;
	texture* RandomAnglesTexture;
	texture* NoiseTexture;

	buffer* PoissonDiskBuffer;
	buffer* RandomSamplesBuffer;

	std::vector<render_context*> CubeMapShadowContexts;
	std::vector<compute_context*> DepthReduceContext;

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
	compute_context* BlurContextV;
	compute_context* BlurContextH;
	compute_context* DebugComputeContext;
};
