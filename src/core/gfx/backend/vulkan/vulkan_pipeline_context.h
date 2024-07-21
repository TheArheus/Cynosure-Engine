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

struct vulkan_command_list : public command_list
{
	vulkan_command_list() = default;

	vulkan_command_list(renderer_backend* Backend)
	{
		CreateResource(Backend);
	}

	~vulkan_command_list() override = default;
	
	void DestroyObject() override;

	void CreateResource(renderer_backend* Backend) override;

	void AcquireNextImage() override;

	void Begin() override;

	void End() override;

	void DeviceWaitIdle() override;

	void EndOneTime() override;

	void SetGraphicsPipelineState(render_context* Context) override;
	void SetComputePipelineState(compute_context* Context) override;

	void SetConstant(void* Data, size_t Size) override;

	void SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight) override;
	void SetIndexBuffer(buffer* Buffer) override;

	void EmplaceColorTarget(texture* RenderTexture) override;

	void Present() override;

	void SetColorTarget(const std::vector<texture*>& Targets, vec4 Clear = {0, 0, 0, 1}) override;
	void SetDepthTarget(texture* Target, vec2 Clear = {1, 0}) override;
	void SetStencilTarget(texture* Target, vec2 Clear = {1, 0}) override;

	void BindShaderParameters(void* Data) override;

	void DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount) override;
	void DrawIndirect(buffer* IndirectCommands, u32 ObjectDrawCount, u32 CommandStructureSize) override;
	void Dispatch(u32 X = 1, u32 Y = 1, u32 Z = 1) override;

	void FillBuffer(buffer* Buffer, u32 Value) override;
	void FillTexture(texture* Texture, vec4 Value) override;

	void CopyImage(texture* Dst, texture* Src) override;

	void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, u32 SrcStageMask, u32 DstStageMask) override;

	void SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData) override;
	void SetImageBarriers(const std::vector<std::tuple<texture*, u32, barrier_state, u32, u32>>& BarrierData) override;

	void DebugGuiBegin(texture* RenderTarget) override;
	void DebugGuiEnd() override;

	VkRenderingInfoKHR RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	VkRenderPassBeginInfo RenderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

	VkDevice Device;

	VkPipelineStageFlags CurrentStage;
	VkCommandBuffer CommandList;
	VkSemaphore AcquireSemaphore, ReleaseSemaphore;

	vulkan_backend* Gfx;

	std::vector<VkImageView>  AttachmentViews;
	std::vector<VkClearValue> RenderTargetClears;
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR>>   RenderingAttachmentInfos;
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR[]>> RenderingAttachmentInfoArrays;
};

struct vulkan_resource_binder;
class vulkan_render_context : public render_context
{
	friend vulkan_command_list;
	friend vulkan_resource_binder;
public:
	vulkan_render_context() = default;

	vulkan_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::vector<std::string> ShaderList, 
						  const std::vector<image_format>& ColorTargetFormats, const utils::render_context::input_data& InputData = {cull_mode::back, true, true, false, false, false, 0}, const std::vector<shader_define>& ShaderDefines = {});

	vulkan_render_context(const vulkan_render_context&) = delete;
	vulkan_render_context& operator=(const vulkan_render_context&) = delete;

    vulkan_render_context(vulkan_render_context&& other) noexcept : 
		Parameters(std::move(other.Parameters)),
		ShaderStages(std::move(other.ShaderStages)),
		Pipeline(std::move(other.Pipeline))
	{}

    vulkan_render_context& operator=(vulkan_render_context&& other) noexcept 
	{
        if (this != &other) 
		{
			std::swap(Parameters, other.Parameters);
			std::swap(ShaderStages, other.ShaderStages);
			std::swap(Pipeline, other.Pipeline);
        }
        return *this;
    }

	void Clear() override 
	{
		FrameBufferIdx = 0;
	};
	
private:
	
	u32 FrameBufferIdx  = 0;

	load_op  LoadOp;
	store_op StoreOp;

	std::unordered_map<u32, VkFramebuffer> FrameBuffers;
	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;

	std::vector<bool> PushDescriptors;
	std::vector<u32>  DescriptorSizes;
	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	std::vector<VkDescriptorSet> Sets;

	VkPipeline Pipeline;
	VkDescriptorPool Pool;
	VkPipelineLayout RootSignatureHandle;
	VkPushConstantRange ConstantRange = {};

	VkSubpassDescription Subpass = {};
	VkRenderPass RenderPass;
};

// TODO: This is highly unefficient so I need to refactor this
class vulkan_compute_context : public compute_context
{
	friend vulkan_command_list;
	friend vulkan_resource_binder;
public:
	vulkan_compute_context() = default;
	~vulkan_compute_context() override = default;

	vulkan_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines = {});

	vulkan_compute_context(const vulkan_compute_context&) = delete;
	vulkan_compute_context& operator=(const vulkan_compute_context&) = delete;

    vulkan_compute_context(vulkan_compute_context&& other) noexcept : 
		ComputeStage(std::move(other.ComputeStage)),
		Parameters(std::move(other.Parameters)),
		Pipeline(other.Pipeline)
    {}

    vulkan_compute_context& operator=(vulkan_compute_context&& other) noexcept 
	{
        if (this != &other) 
		{
			std::swap(ComputeStage, other.ComputeStage);
			std::swap(Parameters, other.Parameters);
			std::swap(Pipeline, other.Pipeline);
        }
        return *this;
    }


	void Clear() override {};

private:

	VkPipelineShaderStageCreateInfo ComputeStage;

	std::vector<bool> PushDescriptors;
	std::vector<u32> DescriptorSizes;
	std::vector<VkDescriptorSet> Sets;

	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;

	VkPipeline Pipeline;
	VkDescriptorPool Pool;
	VkPipelineLayout RootSignatureHandle;
	VkPushConstantRange ConstantRange = {};
};

struct vulkan_resource_binder : public resource_binder
{
	vulkan_resource_binder(renderer_backend* GeneralBackend)
	{
		vulkan_backend* Backend = static_cast<vulkan_backend*>(GeneralBackend);
		NullTexture1D = Backend->NullTexture1D;
		NullTexture2D = Backend->NullTexture2D;
		NullTexture3D = Backend->NullTexture3D;
		NullTextureCube = Backend->NullTextureCube;
	}

	vulkan_resource_binder(renderer_backend* GeneralBackend, general_context* ContextToUse)
	{
		vulkan_backend* Backend = static_cast<vulkan_backend*>(GeneralBackend);
		NullTexture1D = Backend->NullTexture1D;
		NullTexture2D = Backend->NullTexture2D;
		NullTexture3D = Backend->NullTexture3D;
		NullTextureCube = Backend->NullTextureCube;

		if(ContextToUse->Type == pass_type::graphics)
		{
			vulkan_render_context* ContextToBind = static_cast<vulkan_render_context*>(ContextToUse);
			PushDescriptors = ContextToBind->PushDescriptors;
			Sets = ContextToBind->Sets;
		}
		else if(ContextToUse->Type == pass_type::compute)
		{
			vulkan_compute_context* ContextToBind = static_cast<vulkan_compute_context*>(ContextToUse);
			PushDescriptors = ContextToBind->PushDescriptors;
			Sets = ContextToBind->Sets;
		}
	}

	vulkan_resource_binder(const vulkan_resource_binder&) = delete;
	vulkan_resource_binder operator=(const vulkan_resource_binder&) = delete;

	~vulkan_resource_binder() override
	{
		DestroyObject();
	}

	void DestroyObject()
	{
		DescriptorInfos.clear();
		StaticDescriptorBindings.clear();
	};

	void AppendStaticStorage(general_context* Context, void* Data) override;
	void BindStaticStorage(renderer_backend* GeneralBackend) override;

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	void SetStorageBufferView(buffer* Buffer, u32 Set = 0) override;
	void SetUniformBufferView(buffer* Buffer, u32 Set = 0) override;

	// TODO: Remove image layouts and move them inside texture structure
	void SetSampledImage(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;

	texture* NullTexture1D;
	texture* NullTexture2D;
	texture* NullTexture3D;
	texture* NullTextureCube;

	std::map<u32, u32> SetIndices;
	std::vector<VkWriteDescriptorSet> PushDescriptorBindings;
	std::vector<VkWriteDescriptorSet> StaticDescriptorBindings;

	std::vector<std::unique_ptr<descriptor_info>>   DescriptorInfos;
	std::vector<std::unique_ptr<descriptor_info[]>> DescriptorArrayInfos;

	std::vector<bool> PushDescriptors;
	std::vector<VkDescriptorSet> Sets;
};
