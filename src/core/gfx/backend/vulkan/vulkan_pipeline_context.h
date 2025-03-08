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
	~vulkan_command_list() override = default;

	void Reset() override;
	void Begin() override;

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

	void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, u32 SrcStageMask, u32 DstStageMask) override;

	void SetBufferBarriers(const std::vector<buffer_barrier>& BarrierData) override;
	void SetImageBarriers(const std::vector<texture_barrier>& BarrierData) override;

	void DebugGuiBegin(texture* RenderTarget) override;
	void DebugGuiEnd() override;

	VkRenderingInfoKHR RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	VkRenderPassBeginInfo RenderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

	VkDevice Device;
	u32 LayerCount = 1;
	u32 QueueFlags = 0;
	bool SwapChainWillBeUsed = false;

	VkPipelineStageFlags CurrentStage = 0;
	VkCommandBuffer Handle;

	vulkan_backend* Gfx = nullptr;

	std::vector<VkImageView>  AttachmentViews;
	std::vector<VkClearValue> RenderTargetClears;
	std::vector<VkRenderingAttachmentInfoKHR*> RenderingAttachmentInfos;
};

struct vulkan_resource_binder;
class vulkan_render_context : public render_context
{
	friend vulkan_command_list;
	friend vulkan_resource_binder;

public:
	vulkan_render_context() = default;
	~vulkan_render_context() override { DestroyObject(); }

	vulkan_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::vector<std::string> ShaderList, 
						  const std::vector<image_format>& ColorTargetFormats, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines);

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

	void DestroyObject() override
	{
		for (auto& [Key, Framebuffer] : FrameBuffers)
		{
			if (Framebuffer != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(Device, Framebuffer, nullptr);
				Framebuffer = VK_NULL_HANDLE;
			}
		}
		FrameBuffers.clear();

		for (auto& Layout : Layouts)
		{
			if(Layout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(Device, Layout, nullptr);
				Layout = VK_NULL_HANDLE;
			}
		}
		Layouts.clear();

		if (RenderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(Device, RenderPass, nullptr);
		}

		if (Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(Device, Pipeline, nullptr);
			Pipeline = VK_NULL_HANDLE;
		}

		if (Pool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(Device, Pool, nullptr);
			Pool = VK_NULL_HANDLE;
		}

		if (RootSignatureHandle != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(Device, RootSignatureHandle, nullptr);
			RootSignatureHandle = VK_NULL_HANDLE;
		}
	}
	
private:
	
	u32 FrameBufferIdx  = 0;

	load_op  LoadOp;
	store_op StoreOp;

	std::unordered_map<u32, VkFramebuffer> FrameBuffers;
	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;
	std::vector<VkDescriptorSetLayout> Layouts;

	std::vector<bool> PushDescriptors;
	std::vector<u32>  DescriptorSizes;
	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	std::vector<VkDescriptorSet> Sets;

	VkDevice Device;
	VkPipeline Pipeline;
	VkDescriptorPool Pool;
	VkPipelineLayout RootSignatureHandle;
	VkPushConstantRange ConstantRange = {};

	VkSubpassDescription Subpass = {};
	VkRenderPass RenderPass;
};

class vulkan_compute_context : public compute_context
{
	friend vulkan_command_list;
	friend vulkan_resource_binder;

public:
	vulkan_compute_context() = default;
	~vulkan_compute_context() override { DestroyObject(); };

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

	void DestroyObject() override
	{
		for (auto& Layout : Layouts)
		{
			if(Layout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(Device, Layout, nullptr);
				Layout = VK_NULL_HANDLE;
			}
		}
		Layouts.clear();

		if (Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(Device, Pipeline, nullptr);
			Pipeline = VK_NULL_HANDLE;
		}

		if (Pool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(Device, Pool, nullptr);
			Pool = VK_NULL_HANDLE;
		}

		if (RootSignatureHandle != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(Device, RootSignatureHandle, nullptr);
			RootSignatureHandle = VK_NULL_HANDLE;
		}
	}

private:

	VkPipelineShaderStageCreateInfo ComputeStage;

	std::vector<bool> PushDescriptors;
	std::vector<u32> DescriptorSizes;
	std::vector<VkDescriptorSet> Sets;

	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;
	std::vector<VkDescriptorSetLayout> Layouts;

	VkDevice Device;
	VkPipeline Pipeline;
	VkDescriptorPool Pool;
	VkPipelineLayout RootSignatureHandle;
	VkPushConstantRange ConstantRange = {};
};

struct vulkan_resource_binder : public resource_binder
{
	vulkan_resource_binder() = default;

	vulkan_resource_binder(general_context* ContextToUse)
	{
		SetContext(ContextToUse);
	}

	vulkan_resource_binder(const vulkan_resource_binder&) = delete;
	vulkan_resource_binder operator=(const vulkan_resource_binder&) = delete;

	~vulkan_resource_binder() override
	{
		DestroyObject();
	}

	bool SetContext(general_context* ContextToUse) override
	{
		bool Result = CurrContext != ContextToUse;
		CurrContext = ContextToUse;

		SetIndices.clear();
		if(ContextToUse->Type == pass_type::raster)
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

		return Result;
	}

	void DestroyObject() override
	{
		CurrContext = nullptr;
		Sets.clear();
		PushDescriptors.clear();
		DescriptorInfos.clear();
		PushDescriptorBindings.clear();
		StaticDescriptorBindings.clear();
	};

	void AppendStaticStorage(general_context* Context, const array<binding_packet>& Data, u32 Offset) override;
	void BindStaticStorage(renderer_backend* GeneralBackend) override;

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	void SetBufferView(resource* Buffer, u32 Set = 0) override;

	void SetSampledImage(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetStorageImage(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;
	void SetImageSampler(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) override;

	std::map<u32, u32> SetIndices;
	std::vector<VkWriteDescriptorSet> PushDescriptorBindings;
	std::vector<VkWriteDescriptorSet> StaticDescriptorBindings;

	std::vector<descriptor_info*> DescriptorInfos;

	std::vector<bool> PushDescriptors;
	std::vector<VkDescriptorSet> Sets;
};
