#pragma once

struct shader_pass
{
	string Name;
	pass_type Type;
	void* Parameters;
	meta_descriptor* ReflectionData;

	array<binding_packet> Bindings;

	array<u64> Inputs;
	array<u64> Outputs;
	array<u64> Statics;
};

struct resource_lifetime
{
	size_t FirstUsagePassIndex = std::numeric_limits<size_t>::max();
	size_t LastUsagePassIndex = 0;
};

class global_graphics_context
{
	global_graphics_context(const global_graphics_context&) = delete;
	global_graphics_context& operator=(const global_graphics_context&) = delete;

	using execute_func = std::function<void(command_list*)>;

public:
	global_graphics_context() = default;
	global_graphics_context(renderer_backend* NewBackend);
	~global_graphics_context() { DestroyObject(); };

	global_graphics_context(global_graphics_context&& Oth) noexcept;
	global_graphics_context& operator=(global_graphics_context&& Oth) noexcept;

	void DestroyObject();

	resource_binder* CreateResourceBinder()
	{
		switch(Backend->Type)
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

	command_list* CreateGlobalPipelineContext()
	{
		switch(Backend->Type)
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
		switch(Backend->Type)
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
		switch(Backend->Type)
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
		switch(Backend->Type)
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
		switch(Backend->Type)
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

	std::unordered_map<std::type_index, std::unique_ptr<general_context>> ContextMap;
	std::unordered_map<std::type_index, std::unique_ptr<shader_view_context>> GeneralShaderViewMap;
	
	std::unordered_map<shader_pass*, execute_func> Dispatches;
	std::unordered_map<shader_pass*, std::type_index> PassToContext;

	std::vector<shader_pass*> Passes;

	resource_binder* Binder = nullptr;
	general_context* CurrentContext = nullptr;
	gpu_memory_heap* GpuMemoryHeap = nullptr;
	command_list* ExecutionContext = nullptr;

	//////////////////////////////////////////////////////
	u32 BackBufferIndex = 0;

	resource_descriptor PoissonDiskBuffer;
	resource_descriptor RandomSamplesBuffer;

	resource_descriptor GfxColorTarget[2];
	resource_descriptor GfxDepthTarget;
	resource_descriptor DebugCameraViewDepthTarget;

	resource_descriptor VoxelGridTarget;
	resource_descriptor VoxelGridNormal;
	resource_descriptor HdrColorTarget;
	resource_descriptor BrightTarget;
	resource_descriptor TempBrTarget;

	std::vector<resource_descriptor> GlobalShadow;
	std::vector<resource_descriptor> GBuffer;

	resource_descriptor AmbientOcclusionData;
	resource_descriptor BlurTemp;
	resource_descriptor DepthPyramid;
	resource_descriptor RandomAnglesTexture;
	resource_descriptor NoiseTexture;

	resource_descriptor VolumetricLightOut;
	resource_descriptor IndirectLightOut;
	resource_descriptor LightColor;
};
