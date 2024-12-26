
#include "intrinsics.h"
#include "renderer.h"

#include <imgui/imgui.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_widgets.cpp>

#include "renderer_utils.cpp"

#include "core/gfx/backend/vulkan/vulkan_gfx.hpp"
#include "core/gfx/backend/vulkan/vulkan_command_queue.cpp"
#include "core/gfx/backend/vulkan/vulkan_pipeline_context.cpp"
#include "core/gfx/backend/vulkan/vulkan_backend.cpp"

#if _WIN32
	#include "core/gfx/backend/dx12/directx12_gfx.hpp"
	#include "core/gfx/backend/dx12/directx12_backend.cpp"
	#include "core/gfx/backend/dx12/directx12_pipeline_context.cpp"
#endif

#include "resource_manager.cpp"

// NOTE: I wish this creation was not that ugly
global_graphics_context::
#ifdef _WIN32
global_graphics_context(backend_type _BackendType, HINSTANCE Instance, HWND Window, ImGuiContext* imguiContext)
#else
global_graphics_context(backend_type _BackendType, GLFWwindow* Window)
#endif
{
	if(_BackendType == backend_type::vulkan)
#ifdef _WIN32
		Backend = new vulkan_backend(Instance, Window, imguiContext);
#else
		Backend = new vulkan_backend(Window);
#endif
#ifdef _WIN32
	else if(_BackendType == backend_type::directx12)
		Backend = new directx12_backend(Window, imguiContext);
#endif
	else
		assert("Backend type is unavalable");

	Binder = CreateResourceBinder();
	for(u32 i = 0; i < Backend->ImageCount; i++) ExecutionContexts.push_back(CreateGlobalPipelineContext());
	GpuMemoryHeap = new gpu_memory_heap(Backend);

	utils::texture::input_data TextureInputData = {};
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Sampled;
	for(u32 i = 0; i < Backend->ImageCount; i++)
	{
		GfxColorTarget.push_back(GpuMemoryHeap->CreateTexture("ColorTarget" + std::to_string(i), Backend->Width, Backend->Height, 1, TextureInputData));
	}
	TextureInputData.Format    = image_format::D32_SFLOAT;
	TextureInputData.Usage     = TF_DepthTexture | TF_Sampled;
	GfxDepthTarget = GpuMemoryHeap->CreateTexture("DepthTarget", Backend->Width, Backend->Height, 1, TextureInputData);

	float QuadVertices[] =
	{
		// Positions    // Texture Coords
		-1.0f, -1.0f,   0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f,   1.0f, 0.0f,  // Bottom-right
		 1.0f,  1.0f,   1.0f, 1.0f,  // Top-right
		-1.0f,  1.0f,   0.0f, 1.0f   // Top-left
	};

	u32 QuadIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	QuadVertexBuffer = GpuMemoryHeap->CreateBuffer("QuadVertexBuffer", QuadVertices, sizeof(vec4), 4, RF_StorageBuffer);
	QuadIndexBuffer  = GpuMemoryHeap->CreateBuffer("QuadIndexBuffer" , QuadIndices , sizeof(u32) , 6, RF_IndexBuffer);
}

void global_graphics_context::
DestroyObject()
{
	GeneralShaderViewMap.clear();

	for(command_list* ExecutionContext : ExecutionContexts) delete ExecutionContext;
	ContextMap.clear();

	if(GpuMemoryHeap) delete GpuMemoryHeap;
	if(Binder) delete Binder;
	if(Backend) delete Backend;

	Binder = nullptr;
    Backend = nullptr;
	GpuMemoryHeap = nullptr;
    CurrentContext = nullptr;
    ExecutionContexts.clear();
}

global_graphics_context::
global_graphics_context(global_graphics_context&& Oth) noexcept
    : Binder(Oth.Binder),
	  Backend(Oth.Backend),
	  GpuMemoryHeap(Oth.GpuMemoryHeap),
      ContextMap(std::move(Oth.ContextMap)),
      GeneralShaderViewMap(std::move(Oth.GeneralShaderViewMap)),
      Dispatches(std::move(Oth.Dispatches)),
      PassToContext(std::move(Oth.PassToContext)),
      Passes(std::move(Oth.Passes)),
      CurrentContext(Oth.CurrentContext),
      BackBufferIndex(Oth.BackBufferIndex),
      GfxDepthTarget(std::move(Oth.GfxDepthTarget)),
      QuadVertexBuffer(std::move(Oth.QuadVertexBuffer)),
      QuadIndexBuffer(std::move(Oth.QuadIndexBuffer))
{
	GfxColorTarget = std::move(Oth.GfxColorTarget);
	ExecutionContexts = std::move(Oth.ExecutionContexts),

	Oth.Binder = nullptr;
    Oth.Backend = nullptr;
	Oth.GpuMemoryHeap = nullptr;
    Oth.CurrentContext = nullptr;
}

global_graphics_context& global_graphics_context::
operator=(global_graphics_context&& Oth) noexcept
{
	if (this != &Oth)
	{
		Binder = Oth.Binder;
        Backend = Oth.Backend;
		GpuMemoryHeap = Oth.GpuMemoryHeap;
        ContextMap = std::move(Oth.ContextMap);
        GeneralShaderViewMap = std::move(Oth.GeneralShaderViewMap);
        Dispatches = std::move(Oth.Dispatches);
        PassToContext = std::move(Oth.PassToContext);
        Passes = std::move(Oth.Passes);
        CurrentContext = Oth.CurrentContext;
		ExecutionContexts = std::move(Oth.ExecutionContexts),

        BackBufferIndex = Oth.BackBufferIndex;

		GfxColorTarget = std::move(Oth.GfxColorTarget);

		QuadVertexBuffer = std::move(Oth.QuadVertexBuffer);
		QuadIndexBuffer  = std::move(Oth.QuadIndexBuffer);

		Oth.Binder = nullptr;
        Oth.Backend = nullptr;
		Oth.GpuMemoryHeap = nullptr;
        Oth.CurrentContext = nullptr;
    }

    return *this;
}


resource_binder* global_graphics_context::
CreateResourceBinder()
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

command_list* global_graphics_context::
CreateGlobalPipelineContext()
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

render_context* global_graphics_context::
CreateRenderContext(load_op LoadOp, store_op StoreOp, std::vector<std::string> ShaderList, const std::vector<image_format>& ColorTargets, 
					const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines)
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

compute_context* global_graphics_context::
CreateComputeContext(const std::string& Shader, const std::vector<shader_define>& ShaderDefines)
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

general_context* global_graphics_context::
GetOrCreateContext(shader_pass* Pass)
{
    std::type_index ContextType = PassToContext.at(Pass);
    
    if (auto it = ContextMap.find(ContextType); it != ContextMap.end())
    {
        return it->second.get();
    }

    shader_view_context* ContextView = GeneralShaderViewMap[ContextType].get();
    general_context* NewContext = nullptr;

    if (Pass->Type == pass_type::graphics)
    {
        auto* GraphicsContextView = static_cast<shader_graphics_view_context*>(ContextView);
        NewContext = CreateRenderContext(
            GraphicsContextView->LoadOp,
            GraphicsContextView->StoreOp,
            GraphicsContextView->Shaders,
            GraphicsContextView->SetupAttachmentDescription(),
            GraphicsContextView->SetupPipelineState(),
            GraphicsContextView->Defines
        );
		ContextMap[ContextType] = std::unique_ptr<general_context>(NewContext);
    }
    else if (Pass->Type == pass_type::compute)
    {
        auto* ComputeContextView = static_cast<shader_compute_view_context*>(ContextView);
        NewContext = CreateComputeContext(ComputeContextView->Shader, ComputeContextView->Defines);
		ContextMap[ContextType] = std::unique_ptr<general_context>(NewContext);
    }

    return NewContext;
}

bool global_graphics_context::
SetContext(shader_pass* Pass, command_list* Context)
{
    CurrentContext = GetOrCreateContext(Pass);
    if (Pass->Type == pass_type::graphics)
    {
        return Context->SetGraphicsPipelineState(static_cast<render_context*>(CurrentContext));
    }
    else if (Pass->Type == pass_type::compute)
    {
        return Context->SetComputePipelineState(static_cast<compute_context*>(CurrentContext));
    }
}

// TODO: move to instanced draw
void global_graphics_context::
PushRectangle(vec2 Pos, vec2 Scale, vec3 Color)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string("Rectangle draw");
	NewPass->Type = pass_type::graphics;

	full_screen_pass_color::raster_parameters RasterParameters = {};
	RasterParameters.ColorTarget = GfxColorTarget[BackBufferIndex];
	RasterParameters.IndexBuffer = QuadIndexBuffer;

	full_screen_pass_color::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;

	NewPass->ShaderReflection = reflect<full_screen_pass_color::parameters>::Get();
	NewPass->ShaderParameters = PushStruct(full_screen_pass_color::parameters);
	*((full_screen_pass_color::parameters*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = reflect<full_screen_pass_color::raster_parameters>::Get();
	NewPass->RasterParameters = PushStruct(full_screen_pass_color::raster_parameters);
	*((full_screen_pass_color::raster_parameters*)NewPass->RasterParameters) = RasterParameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Scale, Color, ColorTarget = GfxColorTarget[BackBufferIndex]]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Scale;
			vec2 Offset;
			vec2 Dims;
			vec3 Color;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget.Width, ColorTarget.Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Scale = Scale / vec2(ColorTarget.Width, ColorTarget.Height);
		Transform.Offset = vec2(0);
		Transform.Dims = vec2(1);
		Transform.Color = Color;

		Cmd->SetConstant((void*)&Transform, sizeof(transform));
		Cmd->DrawIndexed(0, 6, 0, 0, 1);
	};

	PassToContext.emplace(NewPass, std::type_index(typeid(full_screen_pass_color)));

	auto FindIt = ContextMap.find(std::type_index(typeid(full_screen_pass_color)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(full_screen_pass_color))] = std::make_unique<full_screen_pass_color>();
	}
}

// TODO: move to instanced draw
void global_graphics_context::
PushRectangle(vec2 Pos, vec2 Scale, resource_descriptor& Texture)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string("Textured rectangle draw");
	NewPass->Type = pass_type::graphics;

	full_screen_pass_texture::raster_parameters RasterParameters = {};
	RasterParameters.ColorTarget = GfxColorTarget[BackBufferIndex];
	RasterParameters.IndexBuffer = QuadIndexBuffer;

	full_screen_pass_texture::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;
	Parameters.Texture  = Texture;

	NewPass->ShaderReflection = reflect<full_screen_pass_texture::parameters>::Get();
	NewPass->ShaderParameters = PushStruct(full_screen_pass_texture::parameters);
	*((full_screen_pass_texture::parameters*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = reflect<full_screen_pass_texture::raster_parameters>::Get();
	NewPass->RasterParameters = PushStruct(full_screen_pass_texture::raster_parameters);
	*((full_screen_pass_texture::raster_parameters*)NewPass->RasterParameters) = RasterParameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Scale, Texture = Texture, ColorTarget = GfxColorTarget[BackBufferIndex]]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Scale;
			vec2 Offset;
			vec2 Dims;
			vec3 Color;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget.Width, ColorTarget.Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Scale = Scale / vec2(ColorTarget.Width, ColorTarget.Height);
		Transform.Offset = vec2(0);
		Transform.Dims = vec2(1);
		Transform.Color = vec3(1);

		Cmd->SetConstant((void*)&Transform, sizeof(transform));
		Cmd->DrawIndexed(0, 6, 0, 0, 1);
	};

	PassToContext.emplace(NewPass, std::type_index(typeid(full_screen_pass_texture)));

	auto FindIt = ContextMap.find(std::type_index(typeid(full_screen_pass_texture)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(full_screen_pass_texture))] = std::make_unique<full_screen_pass_texture>();
	}
}

void global_graphics_context::
PushRectangle(vec2 Pos, vec2 Scale, vec2 Offset, vec2 Dims, resource_descriptor& Atlas)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string("Textured rectangle draw");
	NewPass->Type = pass_type::graphics;

	full_screen_pass_texture::raster_parameters RasterParameters = {};
	RasterParameters.ColorTarget = GfxColorTarget[BackBufferIndex];
	RasterParameters.IndexBuffer = QuadIndexBuffer;

	full_screen_pass_texture::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;
	Parameters.Texture  = Atlas;

	NewPass->ShaderReflection = reflect<full_screen_pass_texture::parameters>::Get();
	NewPass->ShaderParameters = PushStruct(full_screen_pass_texture::parameters);
	*((full_screen_pass_texture::parameters*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = reflect<full_screen_pass_texture::raster_parameters>::Get();
	NewPass->RasterParameters = PushStruct(full_screen_pass_texture::raster_parameters);
	*((full_screen_pass_texture::raster_parameters*)NewPass->RasterParameters) = RasterParameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Scale, Offset, Dims, Atlas, ColorTarget = GfxColorTarget[BackBufferIndex]]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Scale;
			vec2 Offset;
			vec2 Dims;
			vec3 Color;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget.Width, ColorTarget.Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Scale = Scale / vec2(ColorTarget.Width, ColorTarget.Height);
		Transform.Offset = Offset / vec2(Atlas.Width, Atlas.Height);
		Transform.Dims = Dims / vec2(Atlas.Width, Atlas.Height);
		Transform.Color = vec3(1);

		Cmd->SetConstant((void*)&Transform, sizeof(transform));
		Cmd->DrawIndexed(0, 6, 0, 0, 1);
	};

	PassToContext.emplace(NewPass, std::type_index(typeid(full_screen_pass_texture)));

	auto FindIt = ContextMap.find(std::type_index(typeid(full_screen_pass_texture)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(full_screen_pass_texture))] = std::make_unique<full_screen_pass_texture>();
	}
}

// TODO: move to instanced draw
void global_graphics_context::
PushCircle(vec2 Pos, float Radius, vec3 Color)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string("Circle draw");
	NewPass->Type = pass_type::graphics;

	full_screen_pass_circle::raster_parameters RasterParameters = {};
	RasterParameters.ColorTarget = GfxColorTarget[BackBufferIndex];
	RasterParameters.IndexBuffer = QuadIndexBuffer;

	full_screen_pass_circle::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;

	NewPass->ShaderReflection = reflect<full_screen_pass_circle::parameters>::Get();
	NewPass->ShaderParameters = PushStruct(full_screen_pass_circle::parameters);
	*((full_screen_pass_circle::parameters*)NewPass->ShaderParameters) = Parameters;

	NewPass->RasterReflection = reflect<full_screen_pass_circle::raster_parameters>::Get();
	NewPass->RasterParameters = PushStruct(full_screen_pass_circle::raster_parameters);
	*((full_screen_pass_circle::raster_parameters*)NewPass->RasterParameters) = RasterParameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Radius, Color, ColorTarget = GfxColorTarget[BackBufferIndex]]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Scale;
			vec2 Offset;
			vec2 Dims;
			vec3 Color;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget.Width, ColorTarget.Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Scale = vec2(2.0 * Radius) / vec2(ColorTarget.Width, ColorTarget.Height);
		Transform.Offset = vec2(0);
		Transform.Dims = vec2(1);
		Transform.Color = Color;

		Cmd->SetConstant((void*)&Transform, sizeof(transform));
		Cmd->DrawIndexed(0, 6, 0, 0, 1);
	};

	PassToContext.emplace(NewPass, std::type_index(typeid(full_screen_pass_circle)));

	auto FindIt = ContextMap.find(std::type_index(typeid(full_screen_pass_circle)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(full_screen_pass_circle))] = std::make_unique<full_screen_pass_circle>();
	}
}

void global_graphics_context::
AddTransferPass(std::string Name, execute_func Exec)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string(Name);
	NewPass->Type = pass_type::transfer;
	NewPass->ShaderParameters = nullptr;
	NewPass->RasterParameters = nullptr;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	PassToContext.emplace(NewPass, std::type_index(typeid(transfer)));

	auto FindIt = ContextMap.find(std::type_index(typeid(transfer)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(transfer))] = std::make_unique<transfer>();
	}
}

// TODO: Command parallelization maybe
// TODO: Maybe compile render graph only once per change
// TODO: Make so the resources wouldn't binded to descriptors multiple times(only when they are actually changed in the descriptor)
//       Plus bind static descriptors only once
void global_graphics_context::
Compile()
{
	std::unordered_map<u64, resource_lifetime> Lifetimes;
	std::unordered_map<u64, resource_state> LastKnownResourceStates;
	std::vector<u64> CurrParamsToBind;

	for(u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		shader_pass* Pass = Passes[PassIndex];
		if(Pass->Type == pass_type::transfer) continue;
		general_context* UseContext = GetOrCreateContext(Pass);
		Binder->SetContext(UseContext);

		u32   MemberIdx  = 0;
		u32   StaticOffs = 0;
		bool  HaveStatic = UseContext->ParameterLayout.size() > 1;
		void* ShaderParameters = Pass->ShaderParameters;
		member_definition* Member = nullptr;

		// TODO: remove this vectors
		std::vector<buffer_barrier> AttachmentBufferBarriers;
		std::vector<texture_barrier> AttachmentImageBarriers;

		std::vector<binding_packet> Packets;
		std::vector<u64> ParamsToBind;
		std::vector<u64> InputIDs;
		std::vector<u64> OutputIDs;

		// Inputs and outputs
		for(u32 ParamIdx = 0; ParamIdx < UseContext->ParameterLayout[0].size(); ParamIdx++, MemberIdx++, StaticOffs++)
		{
			descriptor_param Parameter = UseContext->ParameterLayout[0][ParamIdx];
			Member = Pass->ShaderReflection->Data + MemberIdx;
			if(Member->Type == meta_type::gpu_buffer)
			{
				gpu_buffer* Data = (gpu_buffer*)((uptr)ShaderParameters + Member->Offset);

				auto& Lifetime = Lifetimes[Data->ID];
				Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
				Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

				binding_packet NewPacket;
				NewPacket.Resource = GpuMemoryHeap->GetBuffer(Data->ID);
				Packets.push_back(NewPacket);

				resource_state NewState;
				NewState.IsWritable = Parameter.IsWritable;
				NewState.ShaderStageAccess = Parameter.ShaderToUse;
				NewState.ShaderAspect = Parameter.AspectMask;
				NewState.Valid = true;
				auto& LastState = LastKnownResourceStates[Data->ID];

				if(!LastState.Valid || LastState != NewState)
				{
					AttachmentBufferBarriers.push_back({(buffer*)NewPacket.Resource, Parameter.AspectMask, Parameter.ShaderToUse});
				}
				LastState = NewState;

				ParamsToBind.push_back(Data->ID);
				if(!Parameter.IsWritable)
				{
					InputIDs.push_back(Data->ID);
				}
				else
				{
					OutputIDs.push_back(Data->ID);
				}

				ParamIdx += Data->WithCounter;
			}
			else if(Member->Type == meta_type::gpu_texture)
			{
				gpu_texture* Data = (gpu_texture*)((uptr)ShaderParameters + Member->Offset);

				auto& Lifetime = Lifetimes[Data->ID];
				Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
				Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

				binding_packet NewPacket;
				NewPacket.Resources = array<resource*>(1);
				NewPacket.Resources[0] = GpuMemoryHeap->GetTexture(Data->ID);
				NewPacket.SubresourceIndex = Data->SubresourceIdx == SUBRESOURCES_ALL ? 0 : Data->SubresourceIdx;
				NewPacket.Mips = Data->SubresourceIdx;
				Packets.push_back(NewPacket);

				resource_state NewState;
				NewState.IsWritable = Parameter.IsWritable;
				NewState.State = Parameter.BarrierState;
				NewState.ShaderStageAccess = Parameter.ShaderToUse;
				NewState.ShaderAspect = Parameter.AspectMask;
				NewState.Valid = true;
				auto& LastState = LastKnownResourceStates[Data->ID];

				if(!LastState.Valid || LastState != NewState)
				{
					texture* CurrentTexture = (texture*)NewPacket.Resources[0];
					AttachmentImageBarriers.push_back({CurrentTexture, Parameter.AspectMask, Parameter.BarrierState, NewPacket.Mips, Parameter.ShaderToUse});
				}
				LastState = NewState;

				ParamsToBind.push_back(Data->ID);
				if(!Parameter.IsWritable)
				{
					InputIDs.push_back(Data->ID);
				}
				else
				{
					OutputIDs.push_back(Data->ID);
				}
			}
			else if(Member->Type == meta_type::gpu_texture_array)
			{
				gpu_texture_array* Data = (gpu_texture_array*)((uptr)ShaderParameters + Member->Offset);

				binding_packet NewPacket;
				NewPacket.Resources = array<resource*>(Data->IDs.size());
				NewPacket.SubresourceIndex = Data->SubresourceIdx == SUBRESOURCES_ALL ? 0 : Data->SubresourceIdx;
				NewPacket.Mips = Data->SubresourceIdx;

				for(size_t i = 0; i < Data->IDs.size(); i++)
				{
					u64 ID = (*Data)[i];
					auto& Lifetime = Lifetimes[ID];
					Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
					Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

					NewPacket.Resources[i] = GpuMemoryHeap->GetTexture(ID);

					resource_state NewState;
					NewState.IsWritable = Parameter.IsWritable;
					NewState.State = Parameter.BarrierState;
					NewState.ShaderStageAccess = Parameter.ShaderToUse;
					NewState.ShaderAspect = Parameter.AspectMask;
					NewState.Valid = true;
					auto& LastState = LastKnownResourceStates[ID];

					if(!LastState.Valid || LastState != NewState)
					{
						texture* CurrentTexture = (texture*)NewPacket.Resources[i];
						AttachmentImageBarriers.push_back({CurrentTexture, Parameter.AspectMask, Parameter.BarrierState, NewPacket.Mips, Parameter.ShaderToUse});
					}
					LastState = NewState;

					ParamsToBind.push_back(ID);
					if(!Parameter.IsWritable)
					{
						InputIDs.push_back(ID);
					}
					else
					{
						OutputIDs.push_back(ID);
					}
				}
				Packets.push_back(NewPacket);
			}
			else
			{
				// TODO: Implement buffers for basic types
			}
		}

		// Only inputs (static storage)
		for(u32 LayoutIdx = 1; LayoutIdx < UseContext->ParameterLayout.size(); LayoutIdx++)
		{
			for(u32 ParamIdx = 0; ParamIdx < UseContext->ParameterLayout[LayoutIdx].size(); ParamIdx++, MemberIdx++)
			{
				descriptor_param Parameter = UseContext->ParameterLayout[LayoutIdx][ParamIdx];
				Member = Pass->ShaderReflection->Data + MemberIdx;
				if(Member->Type == meta_type::gpu_texture_array)
				{
					gpu_texture_array* Data = (gpu_texture_array*)((uptr)ShaderParameters + Member->Offset);

					binding_packet NewPacket;
					NewPacket.Resources = array<resource*>(Data->IDs.size());
					NewPacket.SubresourceIndex = Data->SubresourceIdx == SUBRESOURCES_ALL ? 0 : Data->SubresourceIdx;
					NewPacket.Mips = Data->SubresourceIdx;

					for(size_t i = 0; i < Data->IDs.size(); i++)
					{
						auto& Lifetime = Lifetimes[(*Data)[i]];
						Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
						Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

						NewPacket.Resources[i] = GpuMemoryHeap->GetTexture((*Data)[i]);
					}
					Packets.push_back(NewPacket);
				}
			}
		}

		Pass->ParamsChanged = (ParamsToBind.size() != CurrParamsToBind.size()) ||
							  !std::equal(CurrParamsToBind.begin(), CurrParamsToBind.end(), ParamsToBind.begin());

		CurrParamsToBind = ParamsToBind;

		Pass->Inputs  = array<u64>(InputIDs.data() , InputIDs.size());
		Pass->Outputs = array<u64>(OutputIDs.data(), OutputIDs.size());
		Pass->BufferBarriers = array<buffer_barrier>(AttachmentBufferBarriers.data(), AttachmentBufferBarriers.size());
		Pass->TextureBarriers = array<texture_barrier>(AttachmentImageBarriers.data(), AttachmentImageBarriers.size());

		Pass->ParamsChanged ? Pass->Bindings = array<binding_packet>(Packets.data(), Packets.size()) : NULL;
		if(HaveStatic)
		{
			Binder->AppendStaticStorage(UseContext, Pass->Bindings, StaticOffs);
		}
	}

	Binder->BindStaticStorage(Backend);
	Binder->DestroyObject();
}

// TODO: Consider of pushing pass work in different thread if some of them are not dependent and can use different queue type
// TODO: Better viewport setup
void global_graphics_context::
Execute()
{
	command_list* ExecutionContext = ExecutionContexts[BackBufferIndex];
	ExecutionContext->AcquireNextImage();
	ExecutionContext->Begin();

	texture* CurrentColorTarget = GpuMemoryHeap->GetTexture(ExecutionContext, GfxColorTarget[BackBufferIndex]);
	ExecutionContext->FillTexture(CurrentColorTarget, vec4(vec3(0), 1.0f));
	ExecutionContext->SetViewport(0, 0, CurrentColorTarget->Width, CurrentColorTarget->Height);

	// NOTE: Little binding cache for some of the resources:
	buffer*  BoundVertexBuffer = nullptr;
	buffer*  BoundIndexBuffer = nullptr;
	std::vector<texture*> BoundColorTargets;
	texture* BoundDepthTarget = nullptr;

	bool RenderingActive = false;
	for(u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		shader_pass* Pass = Passes[PassIndex];
		bool ShouldRebind = SetContext(Pass, ExecutionContext) || Pass->ParamsChanged;

        if (ShouldRebind && RenderingActive)
        {
			ExecutionContext->EndRendering();
			RenderingActive = false;
        }

        if (Pass->RasterReflection && ShouldRebind)
        {
            void* RasterParameters = Pass->RasterParameters;
            std::vector<texture*> DesiredColorTargets; 
            texture* DesiredDepthTarget = nullptr;

            for (u32 RasterParamIdx = 0; RasterParamIdx < Pass->RasterReflection->Size; ++RasterParamIdx)
            {
                member_definition* Member = Pass->RasterReflection->Data + RasterParamIdx;

                if (Member->Type == meta_type::gpu_index_buffer)
                {
                    gpu_index_buffer* Data = (gpu_index_buffer*)((uptr)RasterParameters + Member->Offset);
                    buffer* IndexBuffer = GpuMemoryHeap->GetBuffer(ExecutionContext, Data->ID);

                    if (BoundIndexBuffer != IndexBuffer)
                    {
                        ExecutionContext->AttachmentBufferBarriers.push_back({IndexBuffer, RF_IndexBuffer, PSF_VertexShader});
                        ExecutionContext->SetIndexBuffer(IndexBuffer);
                        BoundIndexBuffer = IndexBuffer;
                    }
                }
                else if (Member->Type == meta_type::gpu_color_target)
                {
                    gpu_color_target* Data = (gpu_color_target*)((uptr)RasterParameters + Member->Offset);
                    DesiredColorTargets.clear();
                    DesiredColorTargets.reserve(Data->IDs.size());

                    for (size_t i = 0; i < Data->IDs.size(); i++)
                    {
                        u64 ID = (*Data)[i];
                        texture* ColorTarget = GpuMemoryHeap->GetTexture(ExecutionContext, ID);
                        DesiredColorTargets.push_back(ColorTarget);
                    }
					if (ShouldRebind)
					{
						if((BoundColorTargets.size() != DesiredColorTargets.size()) ||
						   !std::equal(BoundColorTargets.begin(), BoundColorTargets.end(), DesiredColorTargets.begin()))
						{
							for (texture* ColorTarget : DesiredColorTargets)
							{
								ExecutionContext->AttachmentImageBarriers.push_back({
									ColorTarget, 
									AF_ColorAttachmentWrite, 
									barrier_state::color_attachment, 
									SUBRESOURCES_ALL, 
									PSF_ColorAttachment
								});
							}
							ExecutionContext->SetColorTarget(DesiredColorTargets);
							BoundColorTargets = DesiredColorTargets;
						}
						else
						{
							ExecutionContext->SetColorTarget(BoundColorTargets);
						}
					}
                }
#if 0
                else if (Member->Type == meta_type::gpu_depth_target)
                {
                    gpu_depth_target* Data = (gpu_depth_target*)((uptr)RasterParameters + Member->Offset);
                    texture* DepthTarget = GpuMemoryHeap->GetTexture(ExecutionContext, (*Data)[0]);
                    DesiredDepthTarget = DepthTarget;
					if (ShouldRebind)
					{
						if (BoundDepthTarget != DesiredDepthTarget)
						{
							ExecutionContext->AttachmentImageBarriers.push_back({
								DesiredDepthTarget, 
								AF_DepthStencilAttachmentWrite, 
								barrier_state::depth_stencil_attachment, 
								SUBRESOURCES_ALL, 
								PSF_EarlyFragment
							});
							ExecutionContext->SetDepthTarget(DesiredDepthTarget);
							BoundDepthTarget = DesiredDepthTarget;
						}
						else
						{
							ExecutionContext->SetDepthTarget(BoundDepthTarget);
						}
					}
                }
#endif
            }
        }

		ExecutionContext->AttachmentBufferBarriers.insert(ExecutionContext->AttachmentBufferBarriers.end(), Pass->BufferBarriers.begin(), Pass->BufferBarriers.end());
		ExecutionContext->AttachmentImageBarriers.insert(ExecutionContext->AttachmentImageBarriers.end(), Pass->TextureBarriers.begin(), Pass->TextureBarriers.end());

		ExecutionContext->SetBufferBarriers(ExecutionContext->AttachmentBufferBarriers);
		ExecutionContext->SetImageBarriers(ExecutionContext->AttachmentImageBarriers);

		ExecutionContext->BindShaderParameters(Pass->Bindings);

        if (ShouldRebind || !RenderingActive)
        {
            ExecutionContext->BeginRendering(Backend->Width, Backend->Height);
            RenderingActive = true;
        }

		Dispatches[Pass](ExecutionContext);

		ExecutionContext->AttachmentBufferBarriers.clear();
		ExecutionContext->AttachmentImageBarriers.clear();
	}

	if(RenderingActive)
	{
		ExecutionContext->EndRendering();
		RenderingActive = false;
	}

	ExecutionContext->DebugGuiBegin(CurrentColorTarget);
	ImGui::Render();
	ExecutionContext->DebugGuiEnd();

	ExecutionContext->EmplaceColorTarget(CurrentColorTarget);
	ExecutionContext->Present();

	for(shader_pass* Pass : Passes)
	{
		if(Pass->Type == pass_type::transfer) continue;
		ContextMap[PassToContext.at(Pass)]->Clear();
	}

	Passes.clear();
	Dispatches.clear();
	PassToContext.clear();

	BackBufferIndex = (BackBufferIndex + 1) % Backend->ImageCount;
}
