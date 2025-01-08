#pragma once

#include "common.hpp"
#include "renderer_utils.hpp"
#include "resource_manager.hpp"

struct shader_pass
{
	string Name;
	pass_type Type;

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

class global_graphics_context
{
	global_graphics_context(const global_graphics_context&) = delete;
	global_graphics_context& operator=(const global_graphics_context&) = delete;

	using execute_func = std::function<void(command_list*)>;

	resource_binder* CreateResourceBinder();
	command_list* CreateGlobalPipelineContext();

	render_context* CreateRenderContext(load_op LoadOp, store_op StoreOp, std::vector<std::string> ShaderList, const std::vector<image_format>& ColorTargets, 
										const utils::render_context::input_data& InputData = {cull_mode::back, blend_factor::src_alpha, blend_factor::one_minus_src_alpha, topology::triangle_list, front_face::counter_clock_wise, true, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {});
	compute_context* CreateComputeContext(const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});

public:
	RENDERER_API global_graphics_context() = default;

#ifdef _WIN32
	RENDERER_API global_graphics_context(backend_type _BackendType, HINSTANCE Instance, HWND Window, ImGuiContext* imguiContext);
#else
	RENDERER_API global_graphics_context(backend_type _BackendType, GLFWwindow* Window);
#endif

	RENDERER_API ~global_graphics_context() { DestroyObject(); };

	RENDERER_API global_graphics_context(global_graphics_context&& Oth) noexcept;
	RENDERER_API global_graphics_context& operator=(global_graphics_context&& Oth) noexcept;

	RENDERER_API void DestroyObject();

	RENDERER_API general_context* GetOrCreateContext(shader_pass* Pass);
	RENDERER_API bool SetContext(shader_pass* Pass, command_list* Context);

	template<typename context_type, typename param_type>
	RENDERER_API void AddPass(std::string Name, param_type Parameters, pass_type Type, execute_func Exec);
	RENDERER_API void AddTransferPass(std::string Name, execute_func Exec);

	RENDERER_API void PushCircle(vec2 Pos, float Radius, vec3 Color);
	RENDERER_API void PushRectangle(vec2 Pos, vec2 Scale, vec3 Color);
	RENDERER_API void PushRectangle(vec2 Pos, vec2 Scale, resource_descriptor& Texture);
	RENDERER_API void PushRectangle(vec2 Pos, vec2 Scale, vec2 Offset, vec2 Dims, resource_descriptor& Atlas);

	RENDERER_API void Compile();
	RENDERER_API void Execute();
	RENDERER_API void SwapBuffers();

	renderer_backend* Backend;

	std::unordered_map<std::type_index, std::unique_ptr<general_context>> ContextMap;
	std::unordered_map<std::type_index, std::unique_ptr<shader_view_context>> GeneralShaderViewMap;
	
	std::unordered_map<shader_pass*, execute_func> Dispatches;
	std::unordered_map<shader_pass*, std::type_index> PassToContext;

	std::vector<shader_pass*> Passes;

	resource_binder* Binder = nullptr;
	general_context* CurrentContext = nullptr;
	gpu_memory_heap* GpuMemoryHeap = nullptr;
	std::vector<command_list*> ExecutionContexts;

	//////////////////////////////////////////////////////
	u32 BackBufferIndex = 0;

	resource_descriptor QuadVertexBuffer;
	resource_descriptor QuadIndexBuffer;

	std::vector<resource_descriptor> GfxColorTarget;
	resource_descriptor GfxDepthTarget;
};

template<typename context_type, typename param_type>
void global_graphics_context::
AddPass(std::string Name, param_type Parameters, pass_type Type, execute_func Exec)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string(Name);
	NewPass->Type = Type;
	NewPass->ShaderReflection = reflect<param_type>::Get();

	NewPass->ShaderParameters = PushStruct(param_type);
	*((param_type*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = nullptr;
	NewPass->RasterParameters = nullptr;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	PassToContext.emplace(NewPass, std::type_index(typeid(context_type)));

	auto FindIt = ContextMap.find(std::type_index(typeid(context_type)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(context_type))] = std::make_unique<context_type>();
	}
}
