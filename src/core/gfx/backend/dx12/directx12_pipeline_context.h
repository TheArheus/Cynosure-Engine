#pragma once

struct descriptor_binding
{
	dx12_descriptor_type Type;
	D3D12_GPU_DESCRIPTOR_HANDLE TableBegin;
	D3D12_GPU_VIRTUAL_ADDRESS ResourceBegin;
	u32 Idx;
	u32 Count;
};

struct directx12_command_list : public command_list
{
	directx12_command_list() = default;
	~directx12_command_list() override = default;

	void Begin() override;
	void Reset() override;
	
	void PlaceEndOfFrameBarriers() override;

	void Update(buffer* BufferToUpdate, void* Data) override;
	void UpdateSize(buffer* BufferToUpdate, void* Data, u32 UpdateByteSize) override;
	void ReadBack(buffer* BufferToRead, void* Data) override;
	void ReadBackSize(buffer* BufferToRead, void* Data, u32 UpdateByteSize) override;

	void Update(texture* TextureToUpdate, void* Data) override;
	void ReadBack(texture* TextureToRead, void* Data) override;

	bool SetGraphicsPipelineState(render_context* Context) override;
	bool SetComputePipelineState(compute_context* Context) override;

	void SetConstant(void* Data, size_t Size) override;

	void SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight) override;
	void SetIndexBuffer(buffer* Buffer) override;

	void EmplaceColorTarget(texture* RenderTexture) override;

	void SetColorTarget(const std::vector<texture*>& Targets, vec4 Clear = {0, 0, 0, 1}) override;
	void SetDepthTarget(texture* Target, vec2 Clear = {1, 0}) override;
	void SetStencilTarget(texture* Target, vec2 Clear = {1, 0}) override;

	void BindShaderParameters(const array<binding_packet>& Data) override;

	void BeginRendering(u32 RenderWidth, u32 RenderHeight) override;
	void EndRendering() override;

	void DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount) override;
	void DrawIndirect(u32 ObjectDrawCount, u32 CommandStructureSize) override;
	void Dispatch(u32 X = 1, u32 Y = 1, u32 Z = 1) override;

	void FillBuffer(buffer* Buffer, u32 Value) override;
	void FillTexture(texture* Texture, vec4 Value) override;
	void FillTexture(texture* Texture, float Depth, u32 Stencil) override;

	void CopyImage(texture* Dst, texture* Src) override;

	void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, u32 SrcStageMask, u32 DstStageMask) override {};

	void SetBufferBarriers(const std::vector<buffer_barrier>& BarrierData) override;
	void SetImageBarriers(const std::vector<texture_barrier>& BarrierData) override;

	void DebugGuiBegin(texture* RenderTarget) override;
	void DebugGuiEnd() override;

	directx12_backend* Gfx = nullptr;

	ID3D12Device6* Device = nullptr;
	ID3D12GraphicsCommandList* Handle = nullptr;
	ID3D12CommandAllocator* CommandAlloc;

	std::vector<texture*> ColorAttachmentsToBind;
	texture* DepthStencilAttachmentToBind;

	D3D12_COMMAND_LIST_TYPE Type;

	vec4 ColorClear;
	vec2 DepthClear;
};

class directx12_resource_binder;
class directx12_render_context : public render_context
{
	ID3D12Device6* Device = nullptr;
	friend directx12_command_list;
	friend directx12_resource_binder;

public:
	directx12_render_context() = default;
	~directx12_render_context() override { DestroyObject(); };

	directx12_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::vector<std::string> ShaderList, 
			const std::vector<image_format>& ColorTargetFormats, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines);

	directx12_render_context(const directx12_render_context&) = delete;
	directx12_render_context& operator=(const directx12_render_context&) = delete;

	void Clear() override
	{
		ResourceBindingIdx = 0;
		SamplersBindingIdx = 0;
	}

	void DestroyObject() override 
	{
		if(ResourceHeap)
		{
			delete ResourceHeap;
			ResourceHeap = nullptr;
		}
		if(SamplersHeap)
		{
			delete SamplersHeap;
			SamplersHeap = nullptr;
		}
		if(IndirectSignatureHandle)
		{
			IndirectSignatureHandle.Reset();
		}
		if(RootSignatureHandle)
		{
			RootSignatureHandle.Reset();
		}
		if(Pipeline)
		{
			Pipeline.Reset();
		}
	}

private:
	ComPtr<ID3D12PipelineState> Pipeline;
	ComPtr<ID3D12RootSignature> RootSignatureHandle;
	ComPtr<ID3D12CommandSignature> IndirectSignatureHandle;
	D3D12_PRIMITIVE_TOPOLOGY PipelineTopology;

	u32 PushConstantOffset = 0;
	u32 ResourceBindingIdx = 0;
	u32 SamplersBindingIdx = 0;

	u32  PushConstantSize	  = 0;
	bool HaveDrawID           = false;
	bool HavePushConstant     = false;
	bool IsResourceHeapInited = false;
	bool IsSamplersHeapInited = false;

	load_op  LoadOp;
	store_op StoreOp;

	descriptor_heap* ResourceHeap = nullptr;
	descriptor_heap* SamplersHeap = nullptr;

	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
};

class directx12_compute_context : public compute_context
{
	ID3D12Device6* Device;
	friend directx12_command_list;
	friend directx12_resource_binder;

public:
	directx12_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});
	~directx12_compute_context() override { DestroyObject(); };

	directx12_compute_context(const directx12_compute_context&) = delete;
	directx12_compute_context& operator=(const directx12_compute_context&) = delete;

	void Clear() override
	{
		ResourceBindingIdx = 0;
		SamplersBindingIdx = 0;
	}

	void DestroyObject() override 
	{
		if(ResourceHeap)
		{
			delete ResourceHeap;
			ResourceHeap = nullptr;
		}
		if(SamplersHeap)
		{
			delete SamplersHeap;
			SamplersHeap = nullptr;
		}
		if(RootSignatureHandle)
		{
			RootSignatureHandle.Reset();
		}
		if(Pipeline)
		{
			Pipeline.Reset();
		}
	}

private:
	ComPtr<ID3D12PipelineState> Pipeline;
	ComPtr<ID3D12RootSignature> RootSignatureHandle;

	u32 PushConstantOffset = 0;
	u32 ResourceBindingIdx = 0;
	u32 SamplersBindingIdx = 0;

	u32  PushConstantSize = 0;
	bool HavePushConstant = false;
	bool IsResourceHeapInited = false;
	bool IsSamplersHeapInited = false;

	descriptor_heap* ResourceHeap = nullptr;
	descriptor_heap* SamplersHeap = nullptr;

	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
};

class directx12_resource_binder : public resource_binder
{
public:
	directx12_resource_binder() = default;

	directx12_resource_binder(general_context* ContextToUse)
	{
		SetContext(ContextToUse);
	}

	directx12_resource_binder(const directx12_resource_binder&) = delete;
	directx12_resource_binder operator=(const directx12_resource_binder&) = delete;

	void DestroyObject() override 
	{
		CurrContext = nullptr;
	}

	bool SetContext(general_context* ContextToUse) override
	{
		bool Result = CurrContext != ContextToUse;
		CurrContext = ContextToUse;

		if(ContextToUse->Type == pass_type::raster)
		{
			directx12_render_context* ContextToBind = static_cast<directx12_render_context*>(ContextToUse);
			ResourceBindingIdx = ContextToBind->ResourceBindingIdx;
			SamplersBindingIdx = ContextToBind->SamplersBindingIdx;
			ShaderRootLayout = ContextToBind->ShaderRootLayout;
			ResourceHeap = ContextToBind->ResourceHeap;
			SamplersHeap = ContextToBind->SamplersHeap;
			Device = ContextToBind->Device;
		}
		else if(ContextToUse->Type == pass_type::compute)
		{
			directx12_compute_context* ContextToBind = static_cast<directx12_compute_context*>(ContextToUse);
			ResourceBindingIdx = ContextToBind->ResourceBindingIdx;
			SamplersBindingIdx = ContextToBind->SamplersBindingIdx;
			ShaderRootLayout = ContextToBind->ShaderRootLayout;
			ResourceHeap = ContextToBind->ResourceHeap;
			SamplersHeap = ContextToBind->SamplersHeap;
			Device = ContextToBind->Device;
		}

		return Result;
	}

	void AppendStaticStorage(general_context* Context, const array<binding_packet>& Data, u32 Offset) override {};
	void BindStaticStorage(renderer_backend* GeneralBackend) override {};

	void SetBufferView(resource* Buffer, u32 Set = 0) override;

	void SetSampledImage(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;

	ID3D12Device6* Device = nullptr;
	std::map<u32, u32> SetIndices;
	std::vector<descriptor_binding> BindingDescriptions;
	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;

	u32 ResourceBindingIdx = 0;
	u32 SamplersBindingIdx = 0;
	u32 RootResourceBindingIdx = 0;
	u32 RootSamplersBindingIdx = 0;

	descriptor_heap* ResourceHeap = nullptr;
	descriptor_heap* SamplersHeap = nullptr;
};
