#pragma once

struct descriptor_binding
{
	dx12_descriptor_type Type;
	D3D12_GPU_DESCRIPTOR_HANDLE TableBegin;
	D3D12_GPU_VIRTUAL_ADDRESS ResourceBegin;
	u32 Idx;
	u32 Count;
};

struct directx12_global_pipeline_context : public global_pipeline_context
{
	directx12_global_pipeline_context() = default;

	directx12_global_pipeline_context(renderer_backend* Backend)
	{
		CreateResource(Backend);
	}

	~directx12_global_pipeline_context() override = default;
	
	void DestroyObject() override {}

	void CreateResource(renderer_backend* Backend) override;

	void Begin(renderer_backend* Backend) override;

	void End(renderer_backend* Backend) override;

	void DeviceWaitIdle() override 
	{
		Fence.Wait();
	}

	void EndOneTime(renderer_backend* Backend) override;

	void EmplaceColorTarget(renderer_backend* Backend, texture* RenderTexture) override;

	void Present(renderer_backend* Backend) override;

	void FillBuffer(buffer* Buffer, u32 Value) override;

	void CopyImage(texture* Dst, texture* Src) override;

	void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, u32 SrcStageMask, u32 DstStageMask) override;

	void SetBufferBarrier(const std::tuple<buffer*, u32, u32>& BarrierData, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData, 
						   u32 SrcStageMask, u32 DstStageMask) override;

	void SetImageBarriers(const std::vector<std::tuple<texture*, u32, u32, barrier_state, barrier_state>>& BarrierData, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, u32, barrier_state, barrier_state>>& BarrierData, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void DebugGuiBegin(renderer_backend* Backend, texture* RenderTarget) override;
	void DebugGuiEnd(renderer_backend* Backend) override;

	directx12_fence Fence;

	ID3D12Device6* Device;
	ID3D12GraphicsCommandList* CommandList;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;
};

class directx12_render_context : public render_context
{
	ID3D12Device6* Device;

public:
	directx12_render_context() = default;

	directx12_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::initializer_list<const std::string> ShaderList, 
			const std::vector<texture*>& ColorTargets, const utils::render_context::input_data& InputData = {true, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {});

	directx12_render_context(const directx12_render_context&) = delete;
	directx12_render_context& operator=(const directx12_render_context&) = delete;

	~directx12_render_context() override = default;

	void DestroyObject() override {}

	void Begin(global_pipeline_context* GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight) override;
	void End()   override;
	void Clear() override;

	void SetColorTarget(u32 RenderWidth, u32 RenderHeight, const std::vector<texture*>& ColorAttachments, vec4 Clear, u32 Face = 0, bool EnableMultiview = false) override;
	void SetDepthTarget(u32 RenderWidth, u32 RenderHeight, texture* DepthAttachment, vec2 Clear, u32 Face = 0, bool EnableMultiview = false) override;

	// NOTE: Binding depth/stencil targets or combined target is possible only via SetDepthTarget function
	void SetStencilTarget(u32 RenderWidth, u32 RenderHeight, texture* StencilAttachment, vec2 Clear, u32 Face = 0, bool EnableMultiview = false) override;

	void StaticUpdate() override {}

	void Draw(buffer* VertexBuffer, u32 FirstVertex, u32 VertexCount) override;

	void DrawIndexed(buffer* IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance = 0, u32 InstanceCount = 1) override;

	void DrawIndirect(u32 ObjectDrawCount, buffer* IndexBuffer, buffer* IndirectCommands, u32 CommandStructureSize) override;

	void SetConstant(void* Data, size_t Size) override;

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	void SetStorageBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;
	void SetUniformBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;
	void SetSampledImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;

private:
	ComPtr<ID3D12PipelineState> Pipeline;
	ComPtr<ID3D12RootSignature> RootSignatureHandle;
	ComPtr<ID3D12CommandSignature> IndirectSignatureHandle;
	D3D12_PRIMITIVE_TOPOLOGY PipelineTopology;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ColorTargets;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilTarget;

	std::map<u32, u32> SetIndices;
	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
	std::vector<descriptor_binding> BindingDescriptions;

	u32 PushConstantSize       = 0;
	u32 ResourceBindingIdx     = 0;
	u32 SamplersBindingIdx     = 0;
	u32 RootResourceBindingIdx = 0;
	u32 RootSamplersBindingIdx = 0;

	bool HaveDrawID           = false;
	bool HavePushConstant     = false;
	bool IsResourceHeapInited = false;
	bool IsSamplersHeapInited = false;

	load_op  LoadOp;
	store_op StoreOp;

	directx12_global_pipeline_context* PipelineContext = nullptr;
	descriptor_heap ResourceHeap;
	descriptor_heap SamplersHeap;
};

class directx12_compute_context : public compute_context
{
	ID3D12Device6* Device;

public:
	directx12_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});

	directx12_compute_context(const directx12_compute_context&) = delete;
	directx12_compute_context& operator=(const directx12_compute_context&) = delete;

	~directx12_compute_context() override = default;

	void DestroyObject() override {}

	void Begin(global_pipeline_context* GlobalPipelineContext) override;
	void End()   override;
	void Clear() override;

	void StaticUpdate() override {};
	void Execute(u32 X = 1, u32 Y = 1, u32 Z = 1) override;
	void SetConstant(void* Data, size_t Size) override;
	void SetStorageBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;
	void SetUniformBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;
	void SetSampledImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;

private:
	ComPtr<ID3D12PipelineState> Pipeline;
	ComPtr<ID3D12RootSignature> RootSignatureHandle;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;

	std::map<u32, u32> SetIndices;
	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
	std::vector<descriptor_binding> BindingDescriptions;

	u32 ResourceBindingIdx = 0;
	u32 SamplersBindingIdx = 0;
	u32 RootResourceBindingIdx = 0;
	u32 RootSamplersBindingIdx = 0;

	u32 PushConstantSize  = 0;
	bool HavePushConstant = false;
	bool IsResourceHeapInited = false;
	bool IsSamplersHeapInited = false;

	directx12_global_pipeline_context* PipelineContext = nullptr;
	descriptor_heap ResourceHeap;
	descriptor_heap SamplersHeap;
};

