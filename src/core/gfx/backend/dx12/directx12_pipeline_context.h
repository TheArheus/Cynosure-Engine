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

	directx12_command_list(renderer_backend* Backend)
	{
		CreateResource(Backend);
	}

	~directx12_command_list() override = default;
	
	void DestroyObject() override {}

	void CreateResource(renderer_backend* Backend) override;

	void AcquireNextImage() override;

	void Begin() override;

	void End() override;

	void DeviceWaitIdle() override 
	{
		Fence.Wait();
	}

	void EndOneTime() override;

	void SetGraphicsPipelineState(render_context* Context) override;
	void SetComputePipelineState(compute_context* Context) override;

	void SetConstant(void* Data, size_t Size) override;

	void SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight) override;
	void SetIndexBuffer(buffer* Buffer) override;

	void EmplaceColorTarget(texture* RenderTexture) override;

	void Present() override;

	void SetColorTarget(const std::vector<texture*>& Targets, vec4 Clear = {0, 0, 0, 1}) override {};
	void SetDepthTarget(texture* Target, vec2 Clear = {1, 0}) override {};
	void SetStencilTarget(texture* Target, vec2 Clear = {1, 0}) override {};

	void BindShaderParameters(void* Data) override;

	void DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount) override;
	void DrawIndirect(buffer* IndirectCommands, u32 ObjectDrawCount, u32 CommandStructureSize) override;
	void Dispatch(u32 X = 1, u32 Y = 1, u32 Z = 1) override;

	void FillBuffer(buffer* Buffer, u32 Value) override;
	void FillTexture(texture* Texture, vec4 Value) override;

	void CopyImage(texture* Dst, texture* Src) override;

	void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, u32 SrcStageMask, u32 DstStageMask) override {};

	void SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData) override;
	void SetImageBarriers(const std::vector<std::tuple<texture*, u32, barrier_state, u32, u32>>& BarrierData) override;
	void SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, barrier_state, u32, u32>>& BarrierData) override;

	void DebugGuiBegin(texture* RenderTarget) override;
	void DebugGuiEnd() override;

	directx12_fence Fence;
	directx12_backend* Gfx;

	ID3D12Device6* Device;
	ID3D12GraphicsCommandList* CommandList;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;
};

class directx12_render_context : public render_context
{
	ID3D12Device6* Device;
	friend directx12_command_list;

public:
	directx12_render_context() = default;

	directx12_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::vector<std::string> ShaderList, 
			const std::vector<image_format>& ColorTargetFormats, const utils::render_context::input_data& InputData = {cull_mode::back, true, true, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {});

	directx12_render_context(const directx12_render_context&) = delete;
	directx12_render_context& operator=(const directx12_render_context&) = delete;

	~directx12_render_context() override = default;

private:
	ComPtr<ID3D12PipelineState> Pipeline;
	ComPtr<ID3D12RootSignature> RootSignatureHandle;
	ComPtr<ID3D12CommandSignature> IndirectSignatureHandle;
	D3D12_PRIMITIVE_TOPOLOGY PipelineTopology;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ColorTargets;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilTarget;

	std::map<u32, std::map<u32, u32>> BindingOffsets;
	std::map<u32, u32> SetIndices;
	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
	std::vector<descriptor_binding> BindingDescriptions;

	u32 RepeatedResourceExec = 0;
	u32 RepeatedSamplerExec  = 0;

	u32 ResourceBindingIdx = 0;
	u32 SamplersBindingIdx = 0;
	u32 RootResourceBindingIdx = 0;
	u32 RootSamplersBindingIdx = 0;

	u32  PushConstantSize	  = 0;
	bool HaveDrawID           = false;
	bool HavePushConstant     = false;
	bool IsResourceHeapInited = false;
	bool IsSamplersHeapInited = false;

	load_op  LoadOp;
	store_op StoreOp;

	directx12_command_list* PipelineContext = nullptr;
	descriptor_heap ResourceHeap;
	descriptor_heap SamplersHeap;
};

class directx12_compute_context : public compute_context
{
	ID3D12Device6* Device;
	friend directx12_command_list;

public:
	directx12_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});

	directx12_compute_context(const directx12_compute_context&) = delete;
	directx12_compute_context& operator=(const directx12_compute_context&) = delete;

	~directx12_compute_context() override = default;

private:
	ComPtr<ID3D12PipelineState> Pipeline;
	ComPtr<ID3D12RootSignature> RootSignatureHandle;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;

	std::map<u32, std::map<u32, u32>> BindingOffsets;
	std::map<u32, u32> SetIndices;
	std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
	std::vector<descriptor_binding> BindingDescriptions;

	u32 RepeatedResourceExec = 0;
	u32 RepeatedSamplerExec  = 0;

	u32 ResourceBindingIdx = 0;
	u32 SamplersBindingIdx = 0;
	u32 RootResourceBindingIdx = 0;
	u32 RootSamplersBindingIdx = 0;

	u32  PushConstantSize = 0;
	bool HavePushConstant = false;
	bool IsResourceHeapInited = false;
	bool IsSamplersHeapInited = false;

	directx12_command_list* PipelineContext = nullptr;
	descriptor_heap ResourceHeap;
	descriptor_heap SamplersHeap;
};

class directx12_resource_binder : public resource_binder
{
public:
	directx12_resource_binder(renderer_backend* GeneralBackend);

	directx12_resource_binder(renderer_backend* GeneralBackend, general_context* ContextToUse)
	{
		if(ContextToUse->Type == pass_type::graphics)
		{
			directx12_render_context* ContextToBind = static_cast<directx12_render_context*>(ContextToUse);
		}
		else if(ContextToUse->Type == pass_type::compute)
		{
			directx12_compute_context* ContextToBind = static_cast<directx12_compute_context*>(ContextToUse);
		}
	}

	directx12_resource_binder(const directx12_resource_binder&) = delete;
	directx12_resource_binder operator=(const directx12_resource_binder&) = delete;

	void DestroyObject() {};

	void AppendStaticStorage(general_context* Context, void* Data) override;
	void BindStaticStorage(renderer_backend* GeneralBackend) override;

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	void SetStorageBufferView(buffer* Buffer, u32 Set = 0) override;
	void SetUniformBufferView(buffer* Buffer, u32 Set = 0) override;

	// TODO: Remove image layouts and move them inside texture structure
	void SetSampledImage(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
};
