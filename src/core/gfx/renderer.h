#pragma once

#include "common.hpp"
#include "renderer_utils.hpp"
#include "resource_manager.hpp"

struct shader_pass
{
	string Name;
	pass_type Type;

	u32 Width  = 0;
	u32 Height = 0;

	void* ShaderParameters;
	meta_descriptor* ShaderReflection;

	void* RasterParameters;
	meta_descriptor* RasterReflection;

	array<binding_packet> Bindings;
	array<buffer_barrier> BufferBarriers;
	array<texture_barrier> TextureBarriers;

	array<u64> Inputs;
	array<u64> Outputs;

	bool ParamsChanged = false;
	bool IsNewContext  = false;
};

struct resource_lifetime
{
	size_t FirstUsagePassIndex = std::numeric_limits<size_t>::max();
	size_t LastUsagePassIndex = 0;
};

struct resource_state
{
    bool IsWritable = false;
    barrier_state State = barrier_state::undefined;
    u32 ShaderStageAccess = 0; 
	u32 ShaderAspect = 0;
    bool Valid = false;
    
    bool operator==(const resource_state& Oth) const
    {
        return (IsWritable == Oth.IsWritable) &&
               (State == Oth.State) &&
			   (ShaderAspect == Oth.ShaderAspect) && 
               (ShaderStageAccess == Oth.ShaderStageAccess);
    }

	bool operator!=(const resource_state& Oth) const
	{
		return !(*this == Oth);
	}
};

struct bound_resource
{
    u64 ResourceID;
    u64 SubresourceIdx;

    bool operator==(const bound_resource& other) const
    {
        return (ResourceID == other.ResourceID && SubresourceIdx == other.SubresourceIdx);
    }
};

class global_graphics_context
{
	global_graphics_context(const global_graphics_context&) = delete;
	global_graphics_context& operator=(const global_graphics_context&) = delete;

	using execute_func = std::function<void(command_list*)>;

	resource_binder* CreateResourceBinder();

	render_context* CreateRenderContext(load_op LoadOp, store_op StoreOp, std::vector<std::string> ShaderList, const std::vector<image_format>& ColorTargets, 
										const utils::render_context::input_data& InputData = {cull_mode::back, blend_factor::src_alpha, blend_factor::one_minus_src_alpha, topology::triangle_list, front_face::counter_clock_wise, true, true, true, false}, const std::vector<shader_define>& ShaderDefines = {});
	compute_context* CreateComputeContext(const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});

	buffer*  BoundVertexBuffer = nullptr;
	buffer*  BoundIndexBuffer = nullptr;
	std::vector<texture*> BoundColorTargets;
	texture* BoundDepthTarget = nullptr;
	bool RenderingActive = false;

	void RasterPassExecution(shader_pass* Pass, command_list* ExecutionContext, std::vector<buffer_barrier>& AttachmentBufferBarriers, std::vector<texture_barrier>& AttachmentImageBarriers, bool ShouldRebind, bool IsSecondary);
	void ComputePassExecution(shader_pass* Pass, command_list* ExecutionContext, std::vector<buffer_barrier>& AttachmentBufferBarriers, std::vector<texture_barrier>& AttachmentImageBarriers, bool ShouldRebind, bool IsSecondary);

	void (global_graphics_context::*PassExecutionType[pass_type_count])(shader_pass*, command_list*, std::vector<buffer_barrier>&, std::vector<texture_barrier>&, bool, bool);

public:
	RENDERER_API global_graphics_context() = default;

	u64 CurrHash = 0;
	u64 NextHash = 0;

#ifdef _WIN32
	RENDERER_API global_graphics_context(backend_type _BackendType, HINSTANCE Instance, HWND Window, ImGuiContext* imguiContext, global_memory_allocator* NewAllocator);
#else
	RENDERER_API global_graphics_context(backend_type _BackendType, GLFWwindow* Window, global_memory_allocator* NewAllocator);
#endif

	RENDERER_API ~global_graphics_context() { DestroyObject(); };

	RENDERER_API global_graphics_context(global_graphics_context&& Oth) noexcept;
	RENDERER_API global_graphics_context& operator=(global_graphics_context&& Oth) noexcept;

	RENDERER_API void DestroyObject();

	RENDERER_API gpu_sync* CreateGpuSync(renderer_backend* BackendToUse);

	RENDERER_API general_context* GetOrCreateContext(shader_pass* Pass);
	RENDERER_API bool SetContext(shader_pass* Pass, command_list* Context);

	template<typename context_type, typename param_type, typename raster_param_type>
	RENDERER_API void AddRasterPass(std::string Name, u32 Width, u32 Height, param_type Parameters, raster_param_type RasterParameters, execute_func Exec, execute_func SetupExec = [](command_list* Cmd){});
	template<typename context_type, typename param_type>
	RENDERER_API void AddComputePass(std::string Name, param_type Parameters, execute_func Exec, execute_func SetupExec = [](command_list* Cmd){});

	RENDERER_API void Compile();
	RENDERER_API void ExecuteAsync();
	RENDERER_API void SwapBuffers();

	RENDERER_API void ExportGraphViz();

	renderer_backend* Backend;

	std::unordered_map<std::type_index, std::unique_ptr<general_context>> ContextMap;
	std::unordered_map<std::type_index, std::unique_ptr<shader_view_context>> GeneralShaderViewMap;
	
	std::unordered_map<shader_pass*, execute_func> Dispatches;
	std::unordered_map<shader_pass*, execute_func> SetupDispatches;
	std::unordered_map<shader_pass*, std::type_index> PassToContext;

	std::vector<gpu_sync*> GpuSyncs;
	std::vector<shader_pass*> Passes;

	resource_binder* Binder = nullptr;
	general_context* CurrentContext = nullptr;
	gpu_memory_heap* GpuMemoryHeap = nullptr;
	command_list* PrimaryExecutionContext = nullptr;
	std::vector<command_list*> SecondaryExecutionContexts;

	std::vector<u32> InDegree;
	std::vector<std::vector<u32>> Adjacency;
	std::vector<std::vector<u32>> LevelGroups;

	//////////////////////////////////////////////////////

	u32 BackBufferIndex = 0;

	resource_descriptor QuadVertexBuffer;
	resource_descriptor QuadIndexBuffer;

	std::vector<resource_descriptor> ColorTarget;
	resource_descriptor DepthTarget;
};

template<typename context_type, typename param_type, typename raster_param_type>
void global_graphics_context::
AddRasterPass(std::string Name, u32 Width, u32 Height, param_type Parameters, raster_param_type RasterParameters, execute_func Exec, execute_func SetupExec)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string(Name);
	NewPass->Type = pass_type::raster;
	NewPass->Width  = Width;
	NewPass->Height = Height;

	NewPass->ShaderReflection = reflect<param_type>::Get();
	NewPass->ShaderParameters = PushStruct(param_type);
	*((param_type*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = reflect<raster_param_type>::Get();
	NewPass->RasterParameters = PushStruct(raster_param_type);
	*((raster_param_type*)NewPass->RasterParameters) = RasterParameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	SetupDispatches[NewPass] = SetupExec;
	PassToContext.emplace(NewPass, std::type_index(typeid(context_type)));

	auto FindIt = ContextMap.find(std::type_index(typeid(context_type)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(context_type))] = std::make_unique<context_type>();
	}
}

template<typename context_type, typename param_type>
void global_graphics_context::
AddComputePass(std::string Name, param_type Parameters, execute_func Exec, execute_func SetupExec)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string(Name);
	NewPass->Type = pass_type::compute;
	NewPass->ShaderReflection = reflect<param_type>::Get();

	NewPass->ShaderParameters = PushStruct(param_type);
	*((param_type*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = nullptr;
	NewPass->RasterParameters = nullptr;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	SetupDispatches[NewPass] = SetupExec;
	PassToContext.emplace(NewPass, std::type_index(typeid(context_type)));

	auto FindIt = ContextMap.find(std::type_index(typeid(context_type)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(context_type))] = std::make_unique<context_type>();
	}
}
