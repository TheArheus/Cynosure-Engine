#pragma once

union descriptor_info
{
	struct // NOTE: VkDescriptorBufferInfo
	{
		VkBuffer        buffer;
		VkDeviceSize    offset;
		VkDeviceSize    range;
	};
	struct // NOTE: VkDescriptorImageInfo
	{
		VkSampler        sampler;
		VkImageView      imageView;
		VkImageLayout    imageLayout;
	};
};

struct vulkan_global_pipeline_context : public global_pipeline_context
{
	vulkan_global_pipeline_context() = default;

	vulkan_global_pipeline_context(renderer_backend* Backend)
	{
		CreateResource(Backend);
	}

	void CreateResource(renderer_backend* Backend) override;
	
	void DestroyObject() override;

	void AcquireNextImage() override;

	void Begin() override;

	void End() override;

	void DeviceWaitIdle() override;

	void EndOneTime() override;

	void EmplaceColorTarget(texture* RenderTexture) override;

	void Present() override;

	void FillBuffer(buffer* Buffer, u32 Value) override;
	void FillTexture(texture* Texture, barrier_state CurrentState, vec4 Value) override;
	void GenerateMips(texture* Texture, barrier_state CurrentState) override;

	void CopyImage(texture* Dst, texture* Src) override;

	void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void SetBufferBarrier(const std::tuple<buffer*, u32, u32>& BarrierData, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData, 
						   u32 SrcStageMask, u32 DstStageMask) override;

	void SetImageBarriers(const std::vector<std::tuple<texture*, u32, u32, barrier_state, barrier_state, u32>>& BarrierData, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, u32, barrier_state, barrier_state, u32>>& BarrierData, 
						  u32 SrcStageMask, u32 DstStageMask) override;

	void DebugGuiBegin(texture* RenderTarget) override;
	void DebugGuiEnd() override;

	VkDevice Device;

	VkPipelineStageFlags CurrentStage;
	VkCommandBuffer* CommandList;
	VkSemaphore AcquireSemaphore, ReleaseSemaphore;

	vulkan_backend* Gfx;
};

class vulkan_general_context : public general_context
{
};

// TODO: This is highly unefficient so I need to refactor this
class vulkan_render_context : public render_context
{
public:
	vulkan_render_context() = default;

	vulkan_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::initializer_list<const std::string> ShaderList, 
						  const std::vector<texture*>& ColorTargets, const utils::render_context::input_data& InputData = {cull_mode::back, true, true, false, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {});

	vulkan_render_context(const vulkan_render_context&) = delete;
	vulkan_render_context& operator=(const vulkan_render_context&) = delete;

    vulkan_render_context(vulkan_render_context&& other) noexcept : 
		RenderingInfo(std::move(other.RenderingInfo)),
		SetIndices(std::move(other.SetIndices)),
		Parameters(std::move(other.Parameters)),
		ShaderStages(std::move(other.ShaderStages)),
		StaticDescriptorBindings(std::move(other.StaticDescriptorBindings)),
		BufferInfos(std::move(other.BufferInfos)),
		BufferArrayInfos(std::move(other.BufferArrayInfos)),
		RenderingAttachmentInfos(std::move(other.RenderingAttachmentInfos)),
		RenderingAttachmentInfoArrays(std::move(other.RenderingAttachmentInfoArrays)),
		PipelineContext(std::move(other.PipelineContext)),
		Device(std::move(other.Device)),
		Pipeline(std::move(other.Pipeline))
	{
		NullTexture1D = other.NullTexture1D;
		other.NullTexture1D = nullptr;

		NullTexture2D = other.NullTexture2D;
		other.NullTexture2D = nullptr;

		NullTexture3D = other.NullTexture3D;
		other.NullTexture3D = nullptr;

		NullTextureCube = other.NullTextureCube;
		other.NullTextureCube = nullptr;
	}

    vulkan_render_context& operator=(vulkan_render_context&& other) noexcept 
	{
        if (this != &other) 
		{
			std::swap(RenderingInfo, other.RenderingInfo);
			std::swap(SetIndices, other.SetIndices);
			std::swap(Parameters, other.Parameters);
			std::swap(ShaderStages, other.ShaderStages);
			std::swap(StaticDescriptorBindings, other.StaticDescriptorBindings);
			std::swap(BufferInfos, other.BufferInfos);
			std::swap(BufferArrayInfos, other.BufferArrayInfos);
			std::swap(RenderingAttachmentInfos, other.RenderingAttachmentInfos);
			std::swap(RenderingAttachmentInfoArrays, other.RenderingAttachmentInfoArrays);

			std::swap(PipelineContext, other.PipelineContext);
			std::swap(Device, other.Device);
			std::swap(Pipeline, other.Pipeline);

			NullTexture1D = other.NullTexture1D;
			other.NullTexture1D = nullptr;

			NullTexture2D = other.NullTexture2D;
			other.NullTexture2D = nullptr;

			NullTexture3D = other.NullTexture3D;
			other.NullTexture3D = nullptr;

			NullTextureCube = other.NullTextureCube;
			other.NullTextureCube = nullptr;
        }
        return *this;
    }

	void DestroyObject() override;

	void Begin(global_pipeline_context* GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight) override;
	void End()   override;
	void Clear() override;

	void SetColorTarget(u32 RenderWidth, u32 RenderHeight, const std::vector<texture*>& ColorAttachments, vec4 Clear, u32 Face = 0, bool EnableMultiview = false) override;
	void SetDepthTarget(u32 RenderWidth, u32 RenderHeight, texture* DepthAttachment, vec2 Clear, u32 Face = 0, bool EnableMultiview = false) override;
	void SetStencilTarget(u32 RenderWidth, u32 RenderHeight, texture* StencilAttachment, vec2 Clear, u32 Face = 0, bool EnableMultiview = false) override;

	void StaticUpdate() override;

	void Draw(buffer* VertexBuffer, u32 FirstVertex, u32 VertexCount);
	void DrawIndexed(buffer* IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance = 0, u32 InstanceCount = 1) override;

	void DrawIndirect(u32 ObjectDrawCount, buffer* IndexBuffer, buffer* IndirectCommands, u32 CommandStructureSize) override;

	void SetConstant(void* Data, size_t Size) override;

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	void SetStorageBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;
	void SetUniformBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;

	// TODO: Remove image layouts and move them inside texture structure
	void SetSampledImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	
private:
	
	u32 GlobalOffset    = 0;
	u32 PushConstantIdx = 0;
	u32 FrameBufferIdx  = 0;

	u32  PushConstantSize = 0;
	bool HavePushConstant = false;
	bool UseFramebuffer   = false;

	load_op  LoadOp;
	store_op StoreOp;

	VkRenderingInfoKHR RenderingInfo;
	VkRenderPassBeginInfo RenderPassInfo;

	std::map<u32, u32> SetIndices;
	std::unordered_map<u32, VkFramebuffer> FrameBuffers;
	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;

	std::vector<u32> DescriptorSizes;
	std::vector<VkImageView> AttachmentViews;
	std::vector<VkClearValue> RenderTargetClears;
	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	std::vector<VkWriteDescriptorSet> StaticDescriptorBindings;
	std::vector<VkDescriptorSet> Sets;
	std::vector<std::unique_ptr<descriptor_info>>   BufferInfos;
	std::vector<std::unique_ptr<descriptor_info[]>> BufferArrayInfos;
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR>>   RenderingAttachmentInfos;
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR[]>> RenderingAttachmentInfoArrays;

	vulkan_global_pipeline_context* PipelineContext;
	texture* NullTexture1D;
	texture* NullTexture2D;
	texture* NullTexture3D;
	texture* NullTextureCube;

	VkDevice Device;
	VkPipeline Pipeline;
	VkDescriptorPool Pool;
	VkPipelineLayout RootSignatureHandle;
	VkFramebufferCreateInfo FramebufferCreateInfo;
	VkPushConstantRange ConstantRange = {};

	VkSubpassDescription Subpass = {};
	VkRenderPass RenderPass;
};

// TODO: This is highly unefficient so I need to refactor this
class vulkan_compute_context : public compute_context
{
public:
	vulkan_compute_context() = default;
	~vulkan_compute_context() override = default;

	vulkan_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});

	vulkan_compute_context(const vulkan_compute_context&) = delete;
	vulkan_compute_context& operator=(const vulkan_compute_context&) = delete;

    vulkan_compute_context(vulkan_compute_context&& other) noexcept : 
		ComputeStage(std::move(other.ComputeStage)),
		StaticDescriptorBindings(std::move(other.StaticDescriptorBindings)),
		BufferInfos(std::move(other.BufferInfos)),
		BufferArrayInfos(std::move(other.BufferArrayInfos)),
		SetIndices(std::move(other.SetIndices)),
		Parameters(std::move(other.Parameters)),
		PipelineContext(std::move(other.PipelineContext)),
		Device(std::move(other.Device)),
		Pipeline(other.Pipeline)
    {
		NullTexture1D = other.NullTexture1D;
		other.NullTexture1D = nullptr;

		NullTexture2D = other.NullTexture2D;
		other.NullTexture2D = nullptr;

		NullTexture3D = other.NullTexture3D;
		other.NullTexture3D = nullptr;

		NullTextureCube = other.NullTextureCube;
		other.NullTextureCube = nullptr;
    }

    vulkan_compute_context& operator=(vulkan_compute_context&& other) noexcept 
	{
        if (this != &other) 
		{
			std::swap(ComputeStage, other.ComputeStage);

			std::swap(StaticDescriptorBindings, other.StaticDescriptorBindings);
			std::swap(BufferInfos, other.BufferInfos);
			std::swap(BufferArrayInfos, other.BufferArrayInfos);

			std::swap(SetIndices, other.SetIndices);
			std::swap(Parameters, other.Parameters);

			std::swap(PipelineContext, other.PipelineContext);

			std::swap(Device, other.Device);
			std::swap(Pipeline, other.Pipeline);

			NullTexture1D = other.NullTexture1D;
			other.NullTexture1D = nullptr;

			NullTexture2D = other.NullTexture2D;
			other.NullTexture2D = nullptr;

			NullTexture3D = other.NullTexture3D;
			other.NullTexture3D = nullptr;

			NullTextureCube = other.NullTextureCube;
			other.NullTextureCube = nullptr;
        }
        return *this;
    }

	void DestroyObject() override;

	void Begin(global_pipeline_context* GlobalPipelineContext) override;
	void End()   override;
	void Clear() override;

	void StaticUpdate() override;

	void Execute(u32 X = 1, u32 Y = 1, u32 Z = 1) override;

	void SetConstant(void* Data, size_t Size) override;

	void SetStorageBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;
	void SetUniformBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) override;

	// TODO: Remove image layouts and move them inside texture structure
	void SetSampledImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;

private:

	VkPipelineShaderStageCreateInfo ComputeStage;

	u32 PushConstantSize = 0;
	bool HavePushConstant = false;

	std::vector<u32> DescriptorSizes;
	std::vector<VkWriteDescriptorSet> StaticDescriptorBindings;
	std::vector<VkDescriptorSet> Sets;
	std::vector<std::unique_ptr<descriptor_info>> BufferInfos;
	std::vector<std::unique_ptr<descriptor_info[]>> BufferArrayInfos;

	std::map<u32, u32> SetIndices;
	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;

	vulkan_global_pipeline_context* PipelineContext;
	texture* NullTexture1D;
	texture* NullTexture2D;
	texture* NullTexture3D;
	texture* NullTextureCube;

	VkDevice Device;
	VkPipeline Pipeline;
	VkDescriptorPool Pool;
	VkPipelineLayout RootSignatureHandle;
	VkPushConstantRange ConstantRange = {};
};
