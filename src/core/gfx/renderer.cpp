
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
	ExecutionContext = CreateGlobalPipelineContext();
	GpuMemoryHeap = new gpu_memory_heap(Backend);

	utils::texture::input_data TextureInputData = {};
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled | TF_CopySrc;
	GfxColorTarget[0] = GpuMemoryHeap->CreateTexture("ColorTarget0", Backend->Width, Backend->Height, 1, TextureInputData);
	GfxColorTarget[1] = GpuMemoryHeap->CreateTexture("ColorTarget1", Backend->Width, Backend->Height, 1, TextureInputData);
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
	ContextMap.clear();

	if(ExecutionContext) delete ExecutionContext;
	if(GpuMemoryHeap) delete GpuMemoryHeap;
	if(Binder) delete Binder;
	if(Backend) delete Backend;

	Binder = nullptr;
    Backend = nullptr;
	GpuMemoryHeap = nullptr;
    CurrentContext = nullptr;
    ExecutionContext = nullptr;
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
      ExecutionContext(Oth.ExecutionContext),
      BackBufferIndex(Oth.BackBufferIndex),
      GfxDepthTarget(std::move(Oth.GfxDepthTarget)),
      QuadVertexBuffer(std::move(Oth.QuadVertexBuffer)),
      QuadIndexBuffer(std::move(Oth.QuadIndexBuffer))
{
    for (size_t i = 0; i < 2; ++i)
	{
        GfxColorTarget[i] = std::move(Oth.GfxColorTarget[i]);
    }

	Oth.Binder = nullptr;
    Oth.Backend = nullptr;
	Oth.GpuMemoryHeap = nullptr;
    Oth.CurrentContext = nullptr;
    Oth.ExecutionContext = nullptr;
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
        ExecutionContext = Oth.ExecutionContext;

        BackBufferIndex = Oth.BackBufferIndex;

		for (size_t i = 0; i < 2; ++i)
		{
            GfxColorTarget[i] = std::move(Oth.GfxColorTarget[i]);
        }

		QuadVertexBuffer = std::move(Oth.QuadVertexBuffer);
		QuadIndexBuffer  = std::move(Oth.QuadIndexBuffer);

		Oth.Binder = nullptr;
        Oth.Backend = nullptr;
		Oth.GpuMemoryHeap = nullptr;
        Oth.CurrentContext = nullptr;
        Oth.ExecutionContext = nullptr;
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

void global_graphics_context::
SetContext(shader_pass* Pass, command_list* Context)
{
    CurrentContext = GetOrCreateContext(Pass);
    if (Pass->Type == pass_type::graphics)
    {
        Context->SetGraphicsPipelineState(static_cast<render_context*>(CurrentContext));
    }
    else if (Pass->Type == pass_type::compute)
    {
        Context->SetComputePipelineState(static_cast<compute_context*>(CurrentContext));
    }
}

// TODO: move to instanced draw
void global_graphics_context::
PushRectangle(vec2 Pos, vec2 Dims, vec3 Color)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string("Rectangle draw");
	NewPass->Type = pass_type::graphics;
	NewPass->ReflectionData = reflect<full_screen_pass_color::parameters>::Get();

	full_screen_pass_color::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;

	NewPass->Parameters = PushStruct(full_screen_pass_color::parameters);
	*((full_screen_pass_color::parameters*)NewPass->Parameters) = Parameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Dims, Color, QuadIndices = GpuMemoryHeap->GetBuffer(QuadIndexBuffer), ColorTarget = GpuMemoryHeap->GetTexture(GfxColorTarget[BackBufferIndex])]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Dim;
			vec3 Color;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget->Width, ColorTarget->Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Dim = Dims / vec2(ColorTarget->Width, ColorTarget->Height);
		Transform.Color = Color;

		Cmd->SetViewport(0, 0, ColorTarget->Width, ColorTarget->Height);
		Cmd->SetColorTarget({ColorTarget});

		Cmd->SetIndexBuffer(QuadIndices);
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
PushRectangle(vec2 Pos, vec2 Dims, resource_descriptor Texture)
{
	shader_pass* NewPass = PushStruct(shader_pass);
	NewPass->Name = string("Textured rectangle draw");
	NewPass->Type = pass_type::graphics;
	NewPass->ReflectionData = reflect<full_screen_pass_texture::parameters>::Get();

	full_screen_pass_texture::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;
	Parameters.Texture  = Texture;

	NewPass->Parameters = PushStruct(full_screen_pass_texture::parameters);
	*((full_screen_pass_texture::parameters*)NewPass->Parameters) = Parameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Dims, QuadIndices = GpuMemoryHeap->GetBuffer(QuadIndexBuffer), ColorTarget = GpuMemoryHeap->GetTexture(GfxColorTarget[BackBufferIndex])]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Dim;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget->Width, ColorTarget->Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Dim = Dims / vec2(ColorTarget->Width, ColorTarget->Height);

		Cmd->SetViewport(0, 0, ColorTarget->Width, ColorTarget->Height);
		Cmd->SetColorTarget({ColorTarget});

		Cmd->SetIndexBuffer(QuadIndices);
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
	NewPass->ReflectionData = reflect<full_screen_pass_circle::parameters>::Get();

	full_screen_pass_circle::parameters Parameters = {};
	Parameters.Vertices = QuadVertexBuffer;

	NewPass->Parameters = PushStruct(full_screen_pass_circle::parameters);
	*((full_screen_pass_circle::parameters*)NewPass->Parameters) = Parameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = [Pos, Radius, Color, QuadIndices = GpuMemoryHeap->GetBuffer(QuadIndexBuffer), ColorTarget = GpuMemoryHeap->GetTexture(GfxColorTarget[BackBufferIndex])]
	(command_list* Cmd)
	{
		struct transform
		{
			vec2 Pos;
			vec2 Dim;
			vec3 Color;
		};
		transform Transform = {};
		Transform.Pos = (Pos / vec2(ColorTarget->Width, ColorTarget->Height)) * vec2(2.0f) - vec2(1.0f);
		Transform.Dim = vec2(2.0 * Radius) / vec2(ColorTarget->Width, ColorTarget->Height);
		Transform.Color = Color;

		Cmd->SetViewport(0, 0, ColorTarget->Width, ColorTarget->Height);
		Cmd->SetColorTarget({ColorTarget});

		Cmd->SetIndexBuffer(QuadIndices);
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
	NewPass->Parameters = nullptr;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	PassToContext.emplace(NewPass, std::type_index(typeid(transfer)));

	auto FindIt = ContextMap.find(std::type_index(typeid(transfer)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(transfer))] = std::make_unique<transfer>();
	}
}


// TODO: Prepass and postpass barriers (postpass barrier is for when the resource is not fully in the same state, for example, after mip generation etc)
//       Move barrier generation to here?
// TODO: Command parallelization maybe
// TODO: Prepass barrier generation and optimization
void global_graphics_context::
Compile()
{
	std::unordered_map<u64, resource_lifetime> Lifetimes;

	for(u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		shader_pass* Pass = Passes[PassIndex];
		if(Pass->Type == pass_type::transfer) continue;
		general_context* UseContext = GetOrCreateContext(Pass);
		Binder->SetContext(UseContext);

		u32   MemberIdx  = 0;
		u32   StaticOffs = 0;
		bool  HaveStatic = UseContext->ParameterLayout.size() > 1;
		void* ParametersToParse = Pass->Parameters;
		member_definition* Member = nullptr;

		// TODO: remove this vectors
		std::vector<binding_packet> Packets;
		std::vector<u64> InputIDs;
		std::vector<u64> OutputIDs;

		// Inputs and outputs
		for(u32 ParamIdx = 0; ParamIdx < UseContext->ParameterLayout[0].size(); ParamIdx++, MemberIdx++, StaticOffs++)
		{
			descriptor_param Parameter = UseContext->ParameterLayout[0][ParamIdx];
			Member = Pass->ReflectionData->Data + MemberIdx;
			if(Member->Type == meta_type::gpu_buffer)
			{
				gpu_buffer* Data = (gpu_buffer*)((uptr)ParametersToParse + Member->Offset);

				auto& Lifetime = Lifetimes[Data->ID];
				Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
				Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

				binding_packet NewPacket;
				NewPacket.Resource = GpuMemoryHeap->GetBuffer(Data->ID);
				Packets.push_back(NewPacket);

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
				gpu_texture* Data = (gpu_texture*)((uptr)ParametersToParse + Member->Offset);

				auto& Lifetime = Lifetimes[Data->ID];
				Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
				Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

				binding_packet NewPacket;
				NewPacket.Resources = array<resource*>(1);
				NewPacket.Resources[0] = GpuMemoryHeap->GetTexture(Data->ID);
				NewPacket.SubresourceIndex = Data->SubresourceIdx == SUBRESOURCES_ALL ? 0 : Data->SubresourceIdx;
				NewPacket.Mips = Data->SubresourceIdx;
				Packets.push_back(NewPacket);

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
				gpu_texture_array* Data = (gpu_texture_array*)((uptr)ParametersToParse + Member->Offset);

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

					if(!Parameter.IsWritable)
					{
						InputIDs.push_back((*Data)[i]);
					}
					else
					{
						OutputIDs.push_back((*Data)[i]);
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
				Member = Pass->ReflectionData->Data + MemberIdx;
				if(Member->Type == meta_type::gpu_texture_array)
				{
					gpu_texture_array* Data = (gpu_texture_array*)((uptr)ParametersToParse + Member->Offset);

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

		Pass->Inputs  = array<u64>(InputIDs.data() , InputIDs.size());
		Pass->Outputs = array<u64>(OutputIDs.data(), OutputIDs.size());

		Pass->Bindings = array<binding_packet>(Packets.data(), Packets.size());
		if(HaveStatic)
		{
			Binder->AppendStaticStorage(UseContext, Pass->Bindings, StaticOffs);
		}
	}

	Binder->BindStaticStorage(Backend);
	Binder->DestroyObject();
}

// TODO: Consider of pushing pass work in different thread if some of them are not dependent and can use different queue type
void global_graphics_context::
Execute()
{
	ExecutionContext->AcquireNextImage();
	ExecutionContext->Begin();
	texture* ColorTarget = GpuMemoryHeap->GetTexture(GfxColorTarget[BackBufferIndex]);
	ExecutionContext->FillTexture(ColorTarget, vec4(vec3(0), 1.0f));
	for(u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		shader_pass* Pass = Passes[PassIndex];
		SetContext(Pass, ExecutionContext);
		ExecutionContext->BindShaderParameters(Pass->Bindings);
		Dispatches[Pass](ExecutionContext);
	}

	// TODO: Think about how to implement this properly now
#if 0
	ExecutionContext->DebugGuiBegin(GpuMemoryHeap->GetTexture(GfxColorTarget[BackBufferIndex]));
	ImGui::NewFrame();

	SceneManager.RenderUI();

	if(SceneManager.Count() > 1)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 300));
		ImGui::SetNextWindowSize(ImVec2(150, 100));
		ImGui::Begin("Active Scenes", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		for(u32 SceneIdx = 0; SceneIdx < SceneManager.Count(); ++SceneIdx)
		{
			if(ImGui::Button(SceneManager.Infos[SceneIdx].Name.c_str()))
			{
				SceneManager.CurrentScene = SceneIdx;
			}
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ExecutionContext->DebugGuiEnd();
#endif

	ExecutionContext->EmplaceColorTarget(ColorTarget);
	ExecutionContext->Present();

	for(shader_pass* Pass : Passes)
	{
		if(Pass->Type == pass_type::transfer) continue;
		ContextMap[PassToContext.at(Pass)]->Clear();
	}

	Passes.clear();
	Dispatches.clear();
	PassToContext.clear();

	BackBufferIndex = (BackBufferIndex + 1) % 2;
}
