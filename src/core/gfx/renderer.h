#pragma once

struct shader_pass
{
	std::string Name;
	pass_type Type;
	void* Parameters;
	bool HaveStaticStorage;
};


class global_graphics_context
{
	global_graphics_context(const global_graphics_context&) = delete;
	global_graphics_context& operator=(const global_graphics_context&) = delete;

	using setup_func = std::function<void()>;
	using execute_func = std::function<void(command_list*, void*)>;

	void ParseShaderParam(meta_descriptor* Descriptor, void* Parameters);

public:
	global_graphics_context() = default;
	global_graphics_context(renderer_backend* NewBackend, backend_type BackendType);

	global_graphics_context(global_graphics_context&& Oth) noexcept;
	global_graphics_context& operator=(global_graphics_context&& Oth) noexcept;

	// TODO: Get buffer_ref
	buffer* PushBuffer(std::string DebugName, u64 DataSize, u64 Count, u32 Flags)
	{
		return Backend->GlobalHeap->PushBuffer(Backend, DebugName, DataSize, Count, Flags);
	}

	// TODO: Get buffer_ref
	buffer* PushBuffer(std::string DebugName, void* Data, u64 DataSize, u64 Count, u32 Flags)
	{
		return Backend->GlobalHeap->PushBuffer(Backend, DebugName, Data, DataSize, Count, Flags);
	}

	// TODO: Get texture_ref
	texture* PushTexture(std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		return Backend->GlobalHeap->PushTexture(Backend, DebugName, Width, Height, Depth, InputData);
	}

	// TODO: Get texture_ref
	texture* PushTexture(std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		return Backend->GlobalHeap->PushTexture(Backend, DebugName, Data, Width, Height, Depth, InputData);
	}

	// TODO: Implement better resource handling
	// Get the texture by texture_ref
	texture_ref UseTextureArray(std::vector<texture*> ArrayOfTextures)
	{
		texture_ref NewRef;
		NewRef.SubresourceIndex = SUBRESOURCES_ALL;
		NewRef.Handle = ArrayOfTextures;
		return NewRef;
	}

	// Get the texture by texture_ref
	texture_ref UseTexture(texture* Texture)
	{
		texture_ref NewRef;
		NewRef.SubresourceIndex = SUBRESOURCES_ALL;
		NewRef.Handle.push_back(Texture);
		return NewRef;
	}

	// Get the texture by texture_ref
	texture_ref UseTextureMip(texture* Texture, u32 Idx)
	{
		texture_ref NewRef;
		NewRef.SubresourceIndex = Idx;
		NewRef.Handle.push_back(Texture);
		return NewRef;
	}

	// Get the buffer by texture_ref
	buffer_ref UseBuffer(buffer* Buffer)
	{
		buffer_ref NewRef;
		NewRef.Handle = Buffer;
		return NewRef;
	}

	resource_binder* CreateResourceBinder()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_resource_binder(Backend);
#if _WIN32
			case backend_type::directx12:
				return new directx12_resource_binder(Backend);
#endif
			default:
				return nullptr;
		}
	}

	memory_heap* CreateMemoryHeap()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_memory_heap(Backend);
#if 0
#if _WIN32
			case backend_type::directx12:
				return new directx12_memory_heap(Backend);
#endif
#endif
			default:
				return nullptr;
		}
	}

	command_list* CreateGlobalPipelineContext()
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_command_list(Backend);
#if _WIN32
			case backend_type::directx12:
				return new directx12_command_list(Backend);
#endif
			default:
				return nullptr;
		}
	}

	render_context* CreateRenderContext(load_op LoadOp, store_op StoreOp, std::vector<std::string> ShaderList, const std::vector<texture*>& ColorTargets, 
										const utils::render_context::input_data& InputData = {cull_mode::back, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {})
	{
		std::vector<image_format> ColorTargetFormats;
		for(u32 FormatIdx = 0; FormatIdx < ColorTargets.size(); ++FormatIdx)
		{
			ColorTargetFormats.push_back(ColorTargets[FormatIdx]->Info.Format);
		}
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_render_context(Backend, LoadOp, StoreOp, ShaderList, ColorTargetFormats, InputData, ShaderDefines);
#if _WIN32
			case backend_type::directx12:
				return new directx12_render_context(Backend, LoadOp, StoreOp, ShaderList, ColorTargetFormats, InputData, ShaderDefines);
#endif
			default:
				return nullptr;
		}
	}

	render_context* CreateRenderContext(load_op LoadOp, store_op StoreOp, std::vector<std::string> ShaderList, const std::vector<image_format>& ColorTargets, 
										const utils::render_context::input_data& InputData = {cull_mode::back, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {})
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

	render_context* CreateRenderContext(load_op LoadOp, store_op StoreOp, std::vector<std::string> ShaderList,
										const utils::render_context::input_data& InputData = {cull_mode::back, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {})
	{
		switch(BackendType)
		{
			case backend_type::vulkan:
				return new vulkan_render_context(Backend, LoadOp, StoreOp, ShaderList, {}, InputData, ShaderDefines);
#if _WIN32
			case backend_type::directx12:
				return new directx12_render_context(Backend, LoadOp, StoreOp, ShaderList, {}, InputData, ShaderDefines);
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

	general_context* GetOrCreateContext(shader_pass* Pass);
	void SetContext(shader_pass* Pass, command_list* Context);

	template<typename context_type, typename param_type>
	shader_pass* AddPass(std::string Name, param_type Parameters, pass_type Type, execute_func Exec);

	shader_pass* AddTransferPass(std::string Name, execute_func Exec);

	void Compile();
	void Execute(scene_manager& SceneManager);

	renderer_backend* Backend;
	backend_type BackendType;
	memory_heap* GlobalHeap;

	std::unordered_map<std::type_index, general_context*> ContextMap;
	std::unordered_map<std::type_index, shader_view_context*> GeneralShaderViewMap;
	
	std::unordered_map<shader_pass*, execute_func> Dispatches;
	std::unordered_map<shader_pass*, std::type_index> PassToContext;

	std::vector<shader_pass*> Passes;

	general_context* CurrentContext = nullptr;

	command_list* ExecutionContext;

	//////////////////////////////////////////////////////
	u32 BackBufferIndex = 0;

	buffer* PoissonDiskBuffer;
	buffer* RandomSamplesBuffer;

	texture* GfxColorTarget[2];
	texture* GfxDepthTarget;
	texture* DebugCameraViewDepthTarget;

	texture* VoxelGridTarget;
	texture* HdrColorTarget;
	texture* BrightTarget;
	texture* TempBrTarget;

	std::vector<texture*> GlobalShadow;
	std::vector<texture*> GBuffer;

	texture* AmbientOcclusionData;
	texture* BlurTemp;
	texture* DepthPyramid;
	texture* RandomAnglesTexture;
	texture* NoiseTexture;

	texture* VolumetricLightOut;
	texture* IndirectLightOut;
	texture* LightColor;
};
