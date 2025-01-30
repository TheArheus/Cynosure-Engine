
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


// TODO: Combine buffer and texture barriers

// NOTE: I wish this creation was not that ugly
global_graphics_context::
#ifdef _WIN32
global_graphics_context(backend_type _BackendType, HINSTANCE Instance, HWND Window, ImGuiContext* imguiContext, global_memory_allocator* NewAllocator)
#else
global_graphics_context(backend_type _BackendType, GLFWwindow* Window, global_memory_allocator* NewAllocator)
#endif
{
	Allocator = NewAllocator;
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
	TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled;
	for(u32 i = 0; i < Backend->ImageCount; i++)
	{
		ColorTarget.push_back(GpuMemoryHeap->CreateTexture("ColorTarget" + std::to_string(i), Backend->Width, Backend->Height, 1, TextureInputData));
	}
	TextureInputData.Format    = image_format::D32_SFLOAT;
	TextureInputData.Usage     = TF_DepthTexture | TF_Sampled;
	DepthTarget = GpuMemoryHeap->CreateTexture("DepthTarget", Backend->Width, Backend->Height, 1, TextureInputData);

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
      SetupDispatches(std::move(Oth.SetupDispatches)),
      PassToContext(std::move(Oth.PassToContext)),
      Passes(std::move(Oth.Passes)),
      CurrentContext(Oth.CurrentContext),
      BackBufferIndex(Oth.BackBufferIndex),
      DepthTarget(std::move(Oth.DepthTarget)),
      QuadVertexBuffer(std::move(Oth.QuadVertexBuffer)),
      QuadIndexBuffer(std::move(Oth.QuadIndexBuffer))
{
	ColorTarget = std::move(Oth.ColorTarget);
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
        SetupDispatches = std::move(Oth.SetupDispatches);
        PassToContext = std::move(Oth.PassToContext);
        Passes = std::move(Oth.Passes);
        CurrentContext = Oth.CurrentContext;
		ExecutionContexts = std::move(Oth.ExecutionContexts),

        BackBufferIndex = Oth.BackBufferIndex;

		ColorTarget = std::move(Oth.ColorTarget);
		DepthTarget = std::move(Oth.DepthTarget);

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
			return new vulkan_resource_binder();
#if _WIN32
		case backend_type::directx12:
			return new directx12_resource_binder();
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

    if (Pass->Type == pass_type::raster)
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
    if (Pass->Type == pass_type::raster)
    {
        return Context->SetGraphicsPipelineState(static_cast<render_context*>(CurrentContext));
    }
    else if (Pass->Type == pass_type::compute)
    {
        return Context->SetComputePipelineState(static_cast<compute_context*>(CurrentContext));
    }
}

static bool 
HasWriteDependency(const array<u64>& WriterOutputs, 
		           const array<u64>& OtherReads, 
				   const array<u64>& OtherWrites)
{
    for(u64 OutID : WriterOutputs)
    {
        // If OutID is in 'OtherReads' => read-after-write hazard => dependency
        // If OutID is in 'OtherWrites' => write-after-write hazard => dependency
        for(u64 InID : OtherReads)
        {
            if(InID == OutID) return true;
        }
        for(u64 WID : OtherWrites)
        {
            if(WID == OutID) return true;
        }
    }
    return false;
}

void global_graphics_context::
ExportGraphViz()
{
    std::ofstream out("../render_graph_viz.dot");
    out << "digraph RenderGraph {\n";
    out << "    rankdir=LR;\n";

    for (size_t i = 0; i < Passes.size(); i++)
    {
        shader_pass* Pass = Passes[i];
        out << "    node" << i
            << " [label=\"" << std::string(Pass->Name.data(), Pass->Name.size()) << "\", shape=box];\n";
    }

    for (size_t i = 0; i < Passes.size(); i++)
    {
        for (auto Neighbor : Adjacency[i])
        {
            out << "    node" << i << " -> node" << Neighbor << ";\n";
        }
    }

    out << "}\n";
    out.close();
}

// TODO: Compilation optimization
// TODO: Better memory arena usage
// TODO: Command parallelization maybe
// TODO: Binding static descriptors only once when needed
// TODO: Better resource utilization using resource lifetimes
void global_graphics_context::
Compile()
{
	std::unordered_map<u64, resource_lifetime> Lifetimes;
	std::unordered_map<u64, resource_state> LastKnownResourceStates;
	std::vector<bound_resource> CurrParamsToBind;

	for(u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		shader_pass* Pass = Passes[PassIndex];
		general_context* UseContext = GetOrCreateContext(Pass);
		Pass->IsNewContext = Binder->SetContext(UseContext);

		u32   MemberIdx  = 0;
		u32   StaticOffs = 0;
		bool  HaveStatic = UseContext->ParameterLayout.size() > 1;
		void* ShaderParameters = Pass->ShaderParameters;
		member_definition* Member = nullptr;

		array<binding_packet> Packets = array<binding_packet>(Pass->ShaderReflection->Size);

		// TODO: remove this vectors
		std::vector<bound_resource> ParamsToBind;
		std::vector<u64> InputIDs;
		std::vector<u64> OutputIDs;

        if (Pass->RasterReflection)
        {
            void* RasterParameters = Pass->RasterParameters;

            for (u32 RasterParamIdx = 0; RasterParamIdx < Pass->RasterReflection->Size; ++RasterParamIdx)
            {
                Member = Pass->RasterReflection->Data + RasterParamIdx;

                if (Member->Type == meta_type::gpu_index_buffer)
                {
                    gpu_index_buffer* Data = (gpu_index_buffer*)((uptr)RasterParameters + Member->Offset);
					InputIDs.push_back(Data->ID);
                }
				else if (Member->Type == meta_type::gpu_indirect_buffer)
				{
                    gpu_indirect_buffer* Data = (gpu_indirect_buffer*)((uptr)RasterParameters + Member->Offset);
					InputIDs.push_back(Data->ID);
				}
                else if (Member->Type == meta_type::gpu_color_target)
                {
                    gpu_color_target* Data = (gpu_color_target*)((uptr)RasterParameters + Member->Offset);
                    for (size_t i = 0; i < Data->IDs.size(); i++)
                    {
                        u64 ID = (*Data)[i];
						OutputIDs.push_back(ID);
                    }
                }
                else if (Member->Type == meta_type::gpu_depth_target)
                {
                    gpu_depth_target* Data = (gpu_depth_target*)((uptr)RasterParameters + Member->Offset);
					OutputIDs.push_back(Data->ID);
                }
            }
        }

		// Inputs and outputs
		for(u32 ParamIdx = 0; ParamIdx < UseContext->ParameterLayout[0].size(); ParamIdx++, MemberIdx++, StaticOffs++)
		{
			descriptor_param Parameter = UseContext->ParameterLayout[0][ParamIdx];
			Member = Pass->ShaderReflection->Data + MemberIdx;
			if(Member->Type == meta_type::gpu_buffer)
			{
				gpu_buffer* Data = (gpu_buffer*)((uptr)ShaderParameters + Member->Offset);
				assert(Data->ID != ~0ull);

				binding_packet NewPacket;
				NewPacket.Resource = GpuMemoryHeap->GetBuffer(Data->ID);
				Packets[MemberIdx] = NewPacket;

				ParamsToBind.push_back({Data->ID});
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

				binding_packet NewPacket;
				NewPacket.Resources = array<resource*>(Data->ID != ~0ull);
				NewPacket.Resources[0] = Data->ID == ~0ull ? nullptr : GpuMemoryHeap->GetTexture(Data->ID);
				NewPacket.SubresourceIndex = Data->SubresourceIdx == SUBRESOURCES_ALL ? 0 : Data->SubresourceIdx;
				NewPacket.Mips = Data->SubresourceIdx;
				Packets[MemberIdx] = NewPacket;

				ParamsToBind.push_back({Data->ID, NewPacket.Mips});
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

					NewPacket.Resources[i] = GpuMemoryHeap->GetTexture(ID);

					ParamsToBind.push_back({ID, NewPacket.Mips});
					if(!Parameter.IsWritable)
					{
						InputIDs.push_back(ID);
					}
					else
					{
						OutputIDs.push_back(ID);
					}
				}
				Packets[MemberIdx] = NewPacket;
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
						NewPacket.Resources[i] = GpuMemoryHeap->GetTexture((*Data)[i]);
					}
					Packets[MemberIdx] = NewPacket;
				}
			}
		}

		Pass->ParamsChanged = (ParamsToBind.size() != CurrParamsToBind.size()) ||
							  !std::equal(CurrParamsToBind.begin(), CurrParamsToBind.end(), ParamsToBind.begin());

		CurrParamsToBind = ParamsToBind;

		Pass->Inputs  = array<u64>(InputIDs.data() , InputIDs.size());
		Pass->Outputs = array<u64>(OutputIDs.data(), OutputIDs.size());

		Pass->ParamsChanged ? Pass->Bindings = Packets : NULL;
		if(HaveStatic)
		{
			Binder->AppendStaticStorage(UseContext, Pass->Bindings, StaticOffs);
		}
	}

	InDegree.assign(Passes.size(), 0);
	Adjacency.assign(Passes.size(), {});
	for(u32 i = 0; i < Passes.size(); i++)
	{
		shader_pass* PassI = Passes[i];

		for(u32 j = i; j < Passes.size(); j++)
		{
			if(i == j) continue;
			shader_pass* PassJ = Passes[j];

			if(HasWriteDependency(PassI->Outputs, PassJ->Inputs, PassJ->Outputs))
				Adjacency[i].push_back(j);
		}
	}

	for(u32 i = 0; i < Passes.size(); i++)
	{
		for(int Neighbor : Adjacency[i])
			InDegree[Neighbor]++;
	}

	std::vector<u32> InDegreeCopy = InDegree;
	std::vector<u32> ReadyPasses;
	for(u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		if(InDegreeCopy[PassIndex] == 0)
			ReadyPasses.push_back(PassIndex);
	}

	while(ReadyPasses.size())
	{
		std::vector<u32> NewReady;
		for(u32 PassIndex : ReadyPasses)
		{
			shader_pass* Pass = Passes[PassIndex];
			general_context* UseContext = GetOrCreateContext(Pass);

			void* ShaderParameters = Pass->ShaderParameters;

			// TODO: remove this vectors
			std::vector<buffer_barrier> AttachmentBufferBarriers;
			std::vector<texture_barrier> AttachmentImageBarriers;

			u32 MemberIdx = 0;
			member_definition* Member = nullptr;
			for(u32 ParamIdx = 0; ParamIdx < UseContext->ParameterLayout[0].size(); ParamIdx++, MemberIdx++)
			{
				descriptor_param Parameter = UseContext->ParameterLayout[0][ParamIdx];
				Member = Pass->ShaderReflection->Data + MemberIdx;
				if(Member->Type == meta_type::gpu_buffer)
				{
					gpu_buffer* Data = (gpu_buffer*)((uptr)ShaderParameters + Member->Offset);

					auto& Lifetime = Lifetimes[Data->ID];
					Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
					Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

					resource_state NewState;
					NewState.IsWritable = Parameter.IsWritable;
					NewState.ShaderStageAccess = Parameter.ShaderToUse;
					NewState.ShaderAspect = Parameter.AspectMask;
					NewState.Valid = true;
					auto& LastState = LastKnownResourceStates[Data->ID];

					if(!LastState.Valid || LastState != NewState)
					{
						AttachmentBufferBarriers.push_back({(buffer*)Pass->Bindings[MemberIdx].Resource, Parameter.AspectMask, Parameter.ShaderToUse});
					}
					LastState = NewState;
					ParamIdx += Data->WithCounter;
				}
				else if(Member->Type == meta_type::gpu_texture)
				{
					gpu_texture* Data = (gpu_texture*)((uptr)ShaderParameters + Member->Offset);

					auto& Lifetime = Lifetimes[Data->ID];
					Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
					Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

					resource_state NewState;
					NewState.IsWritable = Parameter.IsWritable;
					NewState.State = Parameter.BarrierState;
					NewState.ShaderStageAccess = Parameter.ShaderToUse;
					NewState.ShaderAspect = Parameter.AspectMask;
					NewState.Valid = true;
					auto& LastState = LastKnownResourceStates[Data->ID];

					if(Data->ID != ~0ull && (!LastState.Valid || LastState != NewState))
					{
						AttachmentImageBarriers.push_back({(texture*)Pass->Bindings[MemberIdx].Resources[0], Parameter.AspectMask, Parameter.BarrierState, Pass->Bindings[MemberIdx].Mips, Parameter.ShaderToUse});
					}
					LastState = NewState;
				}
				else if(Member->Type == meta_type::gpu_texture_array)
				{
					gpu_texture_array* Data = (gpu_texture_array*)((uptr)ShaderParameters + Member->Offset);

					for(size_t i = 0; i < Data->IDs.size(); i++)
					{
						u64 ID = (*Data)[i];
						auto& Lifetime = Lifetimes[ID];
						Lifetime.FirstUsagePassIndex = Min(Lifetime.FirstUsagePassIndex, PassIndex);
						Lifetime.LastUsagePassIndex  = Max(Lifetime.LastUsagePassIndex , PassIndex);

						resource_state NewState;
						NewState.IsWritable = Parameter.IsWritable;
						NewState.State = Parameter.BarrierState;
						NewState.ShaderStageAccess = Parameter.ShaderToUse;
						NewState.ShaderAspect = Parameter.AspectMask;
						NewState.Valid = true;
						auto& LastState = LastKnownResourceStates[ID];

						if(!LastState.Valid || LastState != NewState)
						{
							AttachmentImageBarriers.push_back({(texture*)Pass->Bindings[MemberIdx].Resources[i], Parameter.AspectMask, Parameter.BarrierState, Pass->Bindings[MemberIdx].Mips, Parameter.ShaderToUse});
						}
						LastState = NewState;
					}
				}
				else
				{
					// TODO: Implement buffers for basic types
				}
			}

			Pass->BufferBarriers  = array<buffer_barrier>(AttachmentBufferBarriers.data(), AttachmentBufferBarriers.size());
			Pass->TextureBarriers = array<texture_barrier>(AttachmentImageBarriers.data(), AttachmentImageBarriers.size());

			for(u32 Neighbor : Adjacency[PassIndex])
			{
				InDegreeCopy[Neighbor]--;
				if(InDegreeCopy[Neighbor] == 0)
				{
					NewReady.push_back(Neighbor);
				}
			}
		}

		ReadyPasses = std::move(NewReady);
	}

	Binder->BindStaticStorage(Backend);
	Binder->DestroyObject();
}


// TODO: Better memory arena usage
// TODO: Consider of pushing pass work in different thread if some of them are not dependent and can use different queue type
void global_graphics_context::
ExecuteAsync()
{
	std::vector<u32> InDegreeCopy = InDegree;
	std::vector<u32> ReadyPasses;
	for (u32 PassIndex = 0; PassIndex < Passes.size(); PassIndex++)
	{
		if (InDegreeCopy[PassIndex] == 0)
			ReadyPasses.push_back(PassIndex);
	}

	command_list* MainExecutionContext = ExecutionContexts[BackBufferIndex];
	MainExecutionContext->AcquireNextImage();
	MainExecutionContext->Begin();

	texture* CurrentColorTarget = GpuMemoryHeap->GetTexture(MainExecutionContext, ColorTarget[BackBufferIndex]);
	MainExecutionContext->FillTexture(CurrentColorTarget, vec4(vec3(0), 1.0f));

	buffer*  BoundVertexBuffer = nullptr;
	buffer*  BoundIndexBuffer = nullptr;
	std::vector<texture*> BoundColorTargets;
	texture* BoundDepthTarget = nullptr;

	bool RenderingActive = false;
	while (ReadyPasses.size())
	{
		std::vector<u32> NewReady;
		for (u32 PassIndex : ReadyPasses)
		{
			shader_pass* Pass = Passes[PassIndex];
			bool ShouldRebind = Pass->ParamsChanged || Pass->IsNewContext;

			bool ColorTargetChanged = false;
			bool DepthTargetChanged = false;
			buffer* DesiredIndexBuffer = nullptr;
			std::vector<texture*> DesiredColorTargets; 
			texture* DesiredDepthTarget = nullptr;

			std::vector<buffer_barrier> AttachmentBufferBarriers;
			std::vector<texture_barrier> AttachmentImageBarriers;
			if (Pass->RasterReflection)
			{
				void* RasterParameters = Pass->RasterParameters;

				for (u32 RasterParamIdx = 0; RasterParamIdx < Pass->RasterReflection->Size; ++RasterParamIdx)
				{
					member_definition* Member = Pass->RasterReflection->Data + RasterParamIdx;

					if (Member->Type == meta_type::gpu_index_buffer)
					{
						gpu_index_buffer* Data = (gpu_index_buffer*)((uptr)RasterParameters + Member->Offset);
						DesiredIndexBuffer = GpuMemoryHeap->GetBuffer(MainExecutionContext, Data->ID);

						if (BoundIndexBuffer != DesiredIndexBuffer)
						{
							AttachmentBufferBarriers.push_back({DesiredIndexBuffer, AF_IndexRead, PSF_VertexInput});
						}
					}
					else if (Member->Type == meta_type::gpu_indirect_buffer)
					{
						gpu_indirect_buffer* Data = (gpu_indirect_buffer*)((uptr)RasterParameters + Member->Offset);
						MainExecutionContext->IndirectCommands = GpuMemoryHeap->GetBuffer(MainExecutionContext, Data->ID);
						AttachmentBufferBarriers.push_back({MainExecutionContext->IndirectCommands, AF_IndirectCommandRead, PSF_DrawIndirect});
					}
					else if (Member->Type == meta_type::gpu_color_target)
					{
						gpu_color_target* Data = (gpu_color_target*)((uptr)RasterParameters + Member->Offset);
						DesiredColorTargets.clear();
						DesiredColorTargets.reserve(Data->IDs.size());

						for (size_t i = 0; i < Data->IDs.size(); i++)
						{
							u64 ID = (*Data)[i];
							texture* NewColorTarget = GpuMemoryHeap->GetTexture(MainExecutionContext, ID);
							DesiredColorTargets.push_back(NewColorTarget);
						}
						if(((BoundColorTargets.size() != DesiredColorTargets.size()) ||
						   !std::equal(BoundColorTargets.begin(), BoundColorTargets.end(), DesiredColorTargets.begin())))
						{
							ColorTargetChanged = true;
							for (texture* NewColorTarget : DesiredColorTargets)
							{
								AttachmentImageBarriers.push_back({
									NewColorTarget, 
									AF_ColorAttachmentWrite, 
									barrier_state::color_attachment, 
									SUBRESOURCES_ALL, 
									PSF_ColorAttachment
								});
							}
						}
					}
					else if (Member->Type == meta_type::gpu_depth_target)
					{
						gpu_depth_target* Data = (gpu_depth_target*)((uptr)RasterParameters + Member->Offset);
						DesiredDepthTarget = GpuMemoryHeap->GetTexture(MainExecutionContext, Data->ID);
						if (BoundDepthTarget != DesiredDepthTarget)
						{
							DepthTargetChanged = true;
							AttachmentImageBarriers.push_back({
								DesiredDepthTarget, 
								AF_DepthStencilAttachmentWrite, 
								barrier_state::depth_stencil_attachment, 
								SUBRESOURCES_ALL, 
								PSF_EarlyFragment
							});
						}
					}
				}
			}

			if (RenderingActive && (ColorTargetChanged || DepthTargetChanged || ShouldRebind))
			{
				MainExecutionContext->EndRendering();
				RenderingActive = false;
			}

			SetContext(Pass, MainExecutionContext);

			SetupDispatches[Pass](MainExecutionContext);

			AttachmentBufferBarriers.insert(AttachmentBufferBarriers.end(), Pass->BufferBarriers.begin(), Pass->BufferBarriers.end());
			AttachmentImageBarriers.insert(AttachmentImageBarriers.end(), Pass->TextureBarriers.begin(), Pass->TextureBarriers.end());

			MainExecutionContext->SetBufferBarriers(AttachmentBufferBarriers);
			MainExecutionContext->SetImageBarriers(AttachmentImageBarriers);

			if (DesiredIndexBuffer && BoundIndexBuffer != DesiredIndexBuffer)
			{
				MainExecutionContext->SetIndexBuffer(DesiredIndexBuffer);
				BoundIndexBuffer = DesiredIndexBuffer;
			}

			if (Pass->Type == pass_type::raster && !RenderingActive)
			{
				if (DesiredColorTargets.size())
				{
					if (ColorTargetChanged)
					{
						MainExecutionContext->SetColorTarget(DesiredColorTargets);
						BoundColorTargets = DesiredColorTargets;
					} 
					else
					{
						MainExecutionContext->SetColorTarget(BoundColorTargets);
					}
				}

				if (DesiredDepthTarget)
				{
					if (DepthTargetChanged)
					{
						MainExecutionContext->SetDepthTarget(DesiredDepthTarget);
						BoundDepthTarget = DesiredDepthTarget;
					}
					else
					{
						MainExecutionContext->SetDepthTarget(BoundDepthTarget);
					}
				}

				MainExecutionContext->BeginRendering(Pass->Width, Pass->Height);
				RenderingActive = true;
			}

			MainExecutionContext->BindShaderParameters(Pass->Bindings);

			Dispatches[Pass](MainExecutionContext);

			for (u32 Neighbor : Adjacency[PassIndex])
			{
				InDegreeCopy[Neighbor]--;
				if (InDegreeCopy[Neighbor] == 0)
				{
					NewReady.push_back(Neighbor);
				}
			}
		}

		ReadyPasses = std::move(NewReady);
	}

	if (RenderingActive)
	{
		MainExecutionContext->EndRendering();
		RenderingActive = false;
	}

	MainExecutionContext->DebugGuiBegin(CurrentColorTarget);
	ImGui::Render();
	MainExecutionContext->DebugGuiEnd();

	MainExecutionContext->EmplaceColorTarget(CurrentColorTarget);
	MainExecutionContext->Present();
}

void global_graphics_context::
SwapBuffers()
{
	for(shader_pass* Pass : Passes)
	{
		ContextMap[PassToContext.at(Pass)]->Clear();
	}

	Passes.clear();
	Dispatches.clear();
	SetupDispatches.clear();
	PassToContext.clear();

	BackBufferIndex = (BackBufferIndex + 1) % Backend->ImageCount;
}
