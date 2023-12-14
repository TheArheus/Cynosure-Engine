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

struct global_pipeline_context
{
	global_pipeline_context() = default;
	
	void DestroyObject()
	{
		if(AcquireSemaphore)
		{
			vkDestroySemaphore(Device, AcquireSemaphore, nullptr);
			AcquireSemaphore = 0;
		}
		if(ReleaseSemaphore)
		{
			vkDestroySemaphore(Device, ReleaseSemaphore, nullptr);
			ReleaseSemaphore = 0;
		}
	}

	global_pipeline_context(command_queue& CommandQueue)
	{
		CreateResource(CommandQueue);
	}

	void CreateResource(command_queue& CommandQueue)
	{
		Device = CommandQueue.Device;

		VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &AcquireSemaphore));
		VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ReleaseSemaphore));

		CommandList = CommandQueue.AllocateCommandList();
	}

	void Begin(renderer_backend* Backend, command_queue& CommandQueue)
	{
		CommandQueue.Reset(CommandList);

		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		vkAcquireNextImageKHR(Device, Gfx->Swapchain, ~0ull, AcquireSemaphore, VK_NULL_HANDLE, &BackBufferIndex);
	}

	void End(command_queue& CommandQueue)
	{
		CommandQueue.Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
	}

	void DeviceWaitIdle()
	{
		vkDeviceWaitIdle(Device);
	}

	void EndOneTime(command_queue& CommandQueue)
	{
		CommandQueue.ExecuteAndRemove(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
	}

	void EmplaceColorTarget(renderer_backend* Backend, texture& Texture)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
		{
			CreateImageBarrier(Texture.Handle, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
			CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
		};
		ImageBarrier(*CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, ImageCopyBarriers);

		VkImageCopy ImageCopyRegion = {};
		ImageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageCopyRegion.srcSubresource.layerCount = 1;
		ImageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageCopyRegion.dstSubresource.layerCount = 1;
		ImageCopyRegion.extent = {u32(Texture.Width), u32(Texture.Height), u32(Texture.Depth)};

		vkCmdCopyImage(*CommandList, Texture.Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Gfx->SwapchainImages[BackBufferIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageCopyRegion);
	}

	void Present(renderer_backend* Backend, command_queue& CommandQueue)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		std::vector<VkImageMemoryBarrier> ImageEndRenderBarriers = 
		{
			CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		};
		ImageBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ImageEndRenderBarriers);

		CommandQueue.Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);

		// NOTE: It shouldn't be there
		VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores = &ReleaseSemaphore;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &Gfx->Swapchain;
		PresentInfo.pImageIndices = &BackBufferIndex;
		vkQueuePresentKHR(CommandQueue.Handle, &PresentInfo);
	}

	void FillBuffer(buffer& Buffer, u32 Value)
	{
		vkCmdFillBuffer(*CommandList, Buffer.Handle, 0, Buffer.Size, Value);
	}

	void CopyImage(texture& Dst, texture& Src)
	{
		std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
		{
			CreateImageBarrier(Src.Handle, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
			CreateImageBarrier(Dst.Handle, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
		};
		ImageBarrier(*CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, ImageCopyBarriers);

		VkImageCopy ImageCopyRegion = {};
		ImageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageCopyRegion.srcSubresource.layerCount = 1;
		ImageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageCopyRegion.dstSubresource.layerCount = 1;
		ImageCopyRegion.extent = {(u32)Src.Width, (u32)Src.Height, 1};

		vkCmdCopyImage(*CommandList, Src.Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Dst.Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageCopyRegion);
	}

	void SetMemoryBarrier(VkAccessFlags SrcAccess, VkAccessFlags DstAccess, 
						  VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		VkMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
		Barrier.srcAccessMask = SrcAccess;
		Barrier.dstAccessMask = DstAccess;

		vkCmdPipelineBarrier(*CommandList, SrcStageMask, DstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 1, &Barrier, 0, 0, 0, 0);
	}

	void SetBufferBarrier(const std::tuple<buffer&, VkAccessFlags, VkAccessFlags>& BarrierData, 
						  VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		Barrier.buffer = std::get<0>(BarrierData).Handle;
		Barrier.srcAccessMask = std::get<1>(BarrierData);
		Barrier.dstAccessMask = std::get<2>(BarrierData);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.offset = 0;
		Barrier.size = std::get<0>(BarrierData).Size;

		vkCmdPipelineBarrier(*CommandList, SrcStageMask, DstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &Barrier, 0, 0);
	}

	void SetBufferBarriers(const std::vector<std::tuple<buffer&, VkAccessFlags, VkAccessFlags>>& BarrierData, 
						   VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		std::vector<VkBufferMemoryBarrier> Barriers;
		for(const std::tuple<buffer&, VkAccessFlags, VkAccessFlags>& Data : BarrierData)
		{
			VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
			Barrier.buffer = std::get<0>(Data).Handle;
			Barrier.srcAccessMask = std::get<1>(Data);
			Barrier.dstAccessMask = std::get<2>(Data);
			Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.offset = 0;
			Barrier.size = std::get<0>(Data).Size;

			std::get<0>(Data).Layout.Access = std::get<1>(Data);

			Barriers.push_back(Barrier);
		}

		vkCmdPipelineBarrier(*CommandList, SrcStageMask, DstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, Barriers.size(), Barriers.data(), 0, 0);
	}

	VkDevice Device;

	VkPipelineStageFlags CurrentStage;
	VkCommandBuffer* CommandList;
	VkSemaphore AcquireSemaphore, ReleaseSemaphore;

	u32 BackBufferIndex = 0;
};

class render_context
{
public:
	struct input_data
	{
		bool UseColor;
		bool UseDepth;
		bool UseBackFace;
		bool UseOutline;
		bool UseMultiview;
		uint8_t ViewMask;
	};

	render_context() = default;

	render_context(renderer_backend* Backend,
				   shader_input* Signature,
				   std::initializer_list<const std::string> ShaderList, const std::vector<VkFormat>& ColorTargetFormats, const input_data& InputData = {true, true, true, false, false, 0}) 
				 : InputSignature(Signature)
	{
		RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		Device = Gfx->Device;

		for(const std::string Shader : ShaderList)
		{
			VkPipelineShaderStageCreateInfo Stage = {};
			Stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str());
			Stage.pName  = "main";
			if(Shader.find(".vert.") != std::string::npos)
			{
				Stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			}
			if(Shader.find(".doma.") != std::string::npos)
			{
				Stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			}
			if(Shader.find(".hull.") != std::string::npos)
			{
				Stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			}
			if (Shader.find(".geom.") != std::string::npos)
			{
				Stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			}
			if(Shader.find(".frag.") != std::string::npos)
			{
				Stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			ShaderStages.push_back(Stage);
		}

		VkGraphicsPipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

		CreateInfo.layout = Signature->Handle;
		CreateInfo.pStages = ShaderStages.data();
		CreateInfo.stageCount = ShaderStages.size();

		VkPipelineRenderingCreateInfoKHR PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
		PipelineRenderingCreateInfo.colorAttachmentCount    = ColorTargetFormats.size();
		PipelineRenderingCreateInfo.pColorAttachmentFormats = InputData.UseColor ? ColorTargetFormats.data() : nullptr;
		PipelineRenderingCreateInfo.depthAttachmentFormat   = InputData.UseDepth ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_UNDEFINED;
		PipelineRenderingCreateInfo.viewMask = InputData.UseMultiview * InputData.ViewMask;

		VkPipelineVertexInputStateCreateInfo VertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

		VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
		InputAssemblyState.topology = InputData.UseOutline ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		std::vector<VkPipelineColorBlendAttachmentState> ColorAttachmentState(ColorTargetFormats.size());
		for(u32 Idx = 0; Idx < ColorTargetFormats.size(); ++Idx)
		{
			ColorAttachmentState[Idx].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
#if DEBUG_COLOR_BLEND
			ColorAttachmentState[Idx].blendEnable = true;
			ColorAttachmentState[Idx].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ColorAttachmentState[Idx].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
#endif
		}

		VkPipelineColorBlendStateCreateInfo ColorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
		ColorBlendState.pAttachments = ColorAttachmentState.data();
		ColorBlendState.attachmentCount = ColorAttachmentState.size();

		VkPipelineDepthStencilStateCreateInfo DepthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
		DepthStencilState.depthTestEnable = true;
		DepthStencilState.depthWriteEnable = true;
		DepthStencilState.minDepthBounds = 0.0f;
		DepthStencilState.maxDepthBounds = 1.0f;
		DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

		VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
		ViewportState.viewportCount = 1;
		ViewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo RasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
		RasterizationState.lineWidth = 1.0f;
		RasterizationState.cullMode  = InputData.UseBackFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT;
		RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineDynamicStateCreateInfo DynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
		VkDynamicState DynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		DynamicState.pDynamicStates = DynamicStates;
		DynamicState.dynamicStateCount = 2;

		VkPipelineMultisampleStateCreateInfo MultisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
		MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //MsaaQuality;

		VkPipelineTessellationStateCreateInfo TessellationState = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

		CreateInfo.pColorBlendState = &ColorBlendState;
		CreateInfo.pDepthStencilState = &DepthStencilState;
		CreateInfo.pDynamicState = &DynamicState;
		CreateInfo.pInputAssemblyState = &InputAssemblyState;
		CreateInfo.pMultisampleState = &MultisampleState;
		CreateInfo.pRasterizationState = &RasterizationState;
		CreateInfo.pTessellationState = &TessellationState;
		CreateInfo.pVertexInputState = &VertexInputState;
		CreateInfo.pViewportState = &ViewportState;
		CreateInfo.pNext = &PipelineRenderingCreateInfo;

		VK_CHECK(vkCreateGraphicsPipelines(Device, nullptr, 1, &CreateInfo, 0, &Pipeline));
	}

	render_context(const render_context&) = delete;
	render_context& operator=(const render_context&) = delete;

    render_context(render_context&& other) noexcept : 
		ColorTargets(std::move(other.ColorTargets)),
		DepthTarget(std::move(other.DepthTarget)),
		GlobalOffset(std::move(other.GlobalOffset)),
		PushConstantIdx(std::move(other.PushConstantIdx)),
		RenderingInfo(std::move(other.RenderingInfo)),
		SetIndices(std::move(other.SetIndices)),
		ShaderStages(std::move(other.ShaderStages)),
		PushDescriptorBindings(std::move(other.PushDescriptorBindings)),
		StaticDescriptorBindings(std::move(other.StaticDescriptorBindings)),
		BufferInfos(std::move(other.BufferInfos)),
		BufferArrayInfos(std::move(other.BufferArrayInfos)),
		RenderingAttachmentInfos(std::move(other.RenderingAttachmentInfos)),
		RenderingAttachmentInfoArrays(std::move(other.RenderingAttachmentInfoArrays)),
		InputSignature(other.InputSignature),
		PipelineContext(std::move(other.PipelineContext)),
		Device(std::move(other.Device)),
		Pipeline(std::move(other.Pipeline))
	{}

    // Move assignment operator
    render_context& operator=(render_context&& other) noexcept 
	{
        if (this != &other) 
		{
			std::swap(ColorTargets, other.ColorTargets);
			std::swap(DepthTarget, other.DepthTarget);
			std::swap(GlobalOffset, other.GlobalOffset);
			std::swap(PushConstantIdx, other.PushConstantIdx);
			std::swap(RenderingInfo, other.RenderingInfo);
			std::swap(SetIndices, other.SetIndices);
			std::swap(ShaderStages, other.ShaderStages);
			std::swap(PushDescriptorBindings, other.PushDescriptorBindings);
			std::swap(StaticDescriptorBindings, other.StaticDescriptorBindings);
			std::swap(BufferInfos, other.BufferInfos);
			std::swap(BufferArrayInfos, other.BufferArrayInfos);
			std::swap(RenderingAttachmentInfos, other.RenderingAttachmentInfos);
			std::swap(RenderingAttachmentInfoArrays, other.RenderingAttachmentInfoArrays);

			InputSignature = other.InputSignature;
			other.InputSignature = nullptr;

			std::swap(PipelineContext, other.PipelineContext);
			std::swap(Device, other.Device);
			std::swap(Pipeline, other.Pipeline);
        }
        return *this;
    }

	void DestroyObject()
	{
		PipelineContext = {};
		for(VkPipelineShaderStageCreateInfo& Stage : ShaderStages)
		{
			vkDestroyShaderModule(Device, Stage.module, nullptr);
		}
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}

	void Begin(const global_pipeline_context& GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight)
	{
		PipelineContext = GlobalPipelineContext;

		std::vector<VkDescriptorSet> SetsToBind;
		for(const VkDescriptorSet& DescriptorSet : InputSignature->Sets)
		{
			if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
		}

		vkCmdBeginRenderingKHR(*PipelineContext.CommandList, &RenderingInfo);

		vkCmdBindPipeline(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
		if(SetsToBind.size())
			vkCmdBindDescriptorSets(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);

		VkViewport Viewport = {0, (float)RenderHeight, (float)RenderWidth, -(float)RenderHeight, 0, 1};
		VkRect2D Scissor = {{0, 0}, {RenderWidth, RenderHeight}};
		vkCmdSetViewport(*PipelineContext.CommandList, 0, 1, &Viewport);
		vkCmdSetScissor(*PipelineContext.CommandList, 0, 1, &Scissor);
	}

	void End()
	{
		ColorTargets.clear();
		DepthTarget = {};

		GlobalOffset = 0;
		PushConstantIdx = 0;

		SetIndices.clear();
		PushDescriptorBindings.clear();
		StaticDescriptorBindings.clear();
		std::vector<std::unique_ptr<descriptor_info>>().swap(BufferInfos);
		std::vector<std::unique_ptr<descriptor_info[]>>().swap(BufferArrayInfos);
		std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR>>().swap(RenderingAttachmentInfos);
		std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR[]>>().swap(RenderingAttachmentInfoArrays);

		vkCmdEndRenderingKHR(*PipelineContext.CommandList);
	}

	void SetColorTarget(VkAttachmentLoadOp LoadOp, VkAttachmentStoreOp StoreOp, u32 RenderWidth, u32 RenderHeight, const std::vector<texture>& ColorAttachments, VkClearValue Clear, u32 Face = 0, bool EnableMultiview = false)
	{
		ColorTargets = ColorAttachments;

		RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
		RenderingInfo.layerCount = 1;
		RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

		std::unique_ptr<VkRenderingAttachmentInfoKHR[]> ColorInfo((VkRenderingAttachmentInfoKHR*)calloc(sizeof(VkRenderingAttachmentInfoKHR), ColorAttachments.size()));
		for(u32 AttachmentIdx = 0; AttachmentIdx < ColorAttachments.size(); ++AttachmentIdx)
		{
			ColorInfo[AttachmentIdx].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			ColorInfo[AttachmentIdx].imageView = ColorAttachments[AttachmentIdx].Views[0];
			ColorInfo[AttachmentIdx].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			ColorInfo[AttachmentIdx].loadOp = LoadOp;
			ColorInfo[AttachmentIdx].storeOp = StoreOp;
			ColorInfo[AttachmentIdx].clearValue = Clear;
		}
		RenderingInfo.colorAttachmentCount = ColorAttachments.size();
		RenderingInfo.pColorAttachments = ColorInfo.get();

		RenderingAttachmentInfoArrays.push_back(std::move(ColorInfo));
	}

	void SetDepthTarget(VkAttachmentLoadOp LoadOp, VkAttachmentStoreOp StoreOp, u32 RenderWidth, u32 RenderHeight, const texture& DepthAttachment, VkClearValue Clear, u32 Face = 0, bool EnableMultiview = false)
	{
		DepthTarget = DepthAttachment;

		RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
		RenderingInfo.layerCount = 1;
		RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

		std::unique_ptr<VkRenderingAttachmentInfoKHR> DepthInfo = std::make_unique<VkRenderingAttachmentInfoKHR>();
		DepthInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		DepthInfo->imageView = DepthAttachment.Views[0];
		DepthInfo->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthInfo->loadOp = LoadOp;
		DepthInfo->storeOp = StoreOp;
		DepthInfo->clearValue = Clear;
		RenderingInfo.pDepthAttachment = DepthInfo.get();

		RenderingAttachmentInfos.push_back(std::move(DepthInfo));
	}

	void SetStencilTarget(VkAttachmentLoadOp LoadOp, VkAttachmentStoreOp StoreOp, u32 RenderWidth, u32 RenderHeight, const texture& StencilAttachment, VkClearValue Clear, u32 Face = 0, bool EnableMultiview = false)
	{
		RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
		RenderingInfo.layerCount = 1;
		RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

		std::unique_ptr<VkRenderingAttachmentInfoKHR> StencilInfo = std::make_unique<VkRenderingAttachmentInfoKHR>();
		StencilInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		StencilInfo->imageView = StencilAttachment.Views[0];
		StencilInfo->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		StencilInfo->loadOp = LoadOp;
		StencilInfo->storeOp = StoreOp;
		StencilInfo->clearValue = Clear;
		RenderingInfo.pStencilAttachment = StencilInfo.get();

		RenderingAttachmentInfos.push_back(std::move(StencilInfo));
	}

	void StaticUpdate()
	{
		vkUpdateDescriptorSets(Device, StaticDescriptorBindings.size(), StaticDescriptorBindings.data(), 0, nullptr);
	}

	void Draw(const buffer& VertexBuffer, u32 FirstVertex, u32 VertexCount)
	{
		vkCmdPushDescriptorSetKHR(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
		vkCmdDraw(*PipelineContext.CommandList, VertexCount, 1, FirstVertex, 0);
		SetIndices.clear();
		GlobalOffset = 0;
		PushConstantIdx = 0;
	}

	void DrawIndexed(const buffer& IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance = 0, u32 InstanceCount = 1)
	{
		vkCmdPushDescriptorSetKHR(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
		vkCmdBindIndexBuffer(*PipelineContext.CommandList, IndexBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(*PipelineContext.CommandList, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
		SetIndices.clear();
		GlobalOffset = 0;
		PushConstantIdx = 0;
	}

	template<typename command_structure>
	void DrawIndirect(u32 ObjectDrawCount, const buffer& IndexBuffer, const buffer& IndirectCommands)
	{
		vkCmdPushDescriptorSetKHR(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
		vkCmdBindIndexBuffer(*PipelineContext.CommandList, IndexBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexedIndirectCount(*PipelineContext.CommandList, IndirectCommands.Handle, 0, IndirectCommands.Handle, IndirectCommands.CounterOffset, ObjectDrawCount, sizeof(command_structure));
		SetIndices.clear();
		GlobalOffset = 0;
		PushConstantIdx = 0;
	}

	void SetConstant(void* Data, size_t Size)
	{
		vkCmdPushConstants(*PipelineContext.CommandList, InputSignature->Handle, InputSignature->PushConstants[PushConstantIdx++].stageFlags, GlobalOffset, Size, Data);
		GlobalOffset += Size;
	}

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	void SetStorageBufferView(const buffer& Buffer, b32 UseCounter = true, u32 Set = 0)
	{
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
		std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
		BufferInfo->buffer = Buffer.Handle;
		BufferInfo->offset = 0;
		BufferInfo->range  = Buffer.WithCounter ? Buffer.CounterOffset : Buffer.Size;

		VkWriteDescriptorSet CounterDescriptorSet = {};
		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = 1;
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferInfos.push_back(std::move(BufferInfo));

		if(Buffer.WithCounter && UseCounter)
		{
			CounterBufferInfo->buffer = Buffer.Handle;
			CounterBufferInfo->offset = Buffer.CounterOffset;
			CounterBufferInfo->range  = sizeof(u32);

			CounterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			if(!IsPush)
				CounterDescriptorSet.dstSet = InputSignature->Sets[Set];
			CounterDescriptorSet.dstBinding = SetIndices[Set];
			CounterDescriptorSet.descriptorCount = 1;
			CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
			if(IsPush)
				PushDescriptorBindings.push_back(CounterDescriptorSet);
			else
				StaticDescriptorBindings.push_back(CounterDescriptorSet);
			SetIndices[Set] += 1;

			BufferInfos.push_back(std::move(CounterBufferInfo));
		}
	}

	void SetUniformBufferView(const buffer& Buffer, bool UseCounter = true, u32 Set = 0)
	{
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
		std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
		BufferInfo->buffer = Buffer.Handle;
		BufferInfo->offset = 0;
		BufferInfo->range  = Buffer.WithCounter ? Buffer.CounterOffset : Buffer.Size;

		VkWriteDescriptorSet CounterDescriptorSet = {};
		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = 1;
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferInfos.push_back(std::move(BufferInfo));

		if(Buffer.WithCounter && UseCounter)
		{
			CounterBufferInfo->buffer = Buffer.Handle;
			CounterBufferInfo->offset = Buffer.CounterOffset;
			CounterBufferInfo->range  = sizeof(u32);

			CounterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			if(!IsPush)
				CounterDescriptorSet.dstSet = InputSignature->Sets[Set];
			CounterDescriptorSet.dstBinding = SetIndices[Set];
			CounterDescriptorSet.descriptorCount = 1;
			CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
			if(IsPush)
				PushDescriptorBindings.push_back(CounterDescriptorSet);
			else
				StaticDescriptorBindings.push_back(CounterDescriptorSet);
			SetIndices[Set] += 1;

			BufferInfos.push_back(std::move(CounterBufferInfo));
		}
	}

	// TODO: Remove image layouts and move them inside texture structure
	void SetSampledImage(const std::vector<texture>& Textures, VkImageLayout Layout, u32 ViewIdx = 0, u32 Set = 0)
	{
		if(Textures.size() == 0)
		{
			SetIndices[Set] += 1;
			return;
		}
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), Textures.size()));
		for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
		{
			ImageInfo[TextureIdx].imageLayout = Layout;
			ImageInfo[TextureIdx].imageView = Textures[TextureIdx].Views[ViewIdx];
			ImageInfo[TextureIdx].sampler = Textures[TextureIdx].Sampler.Handle;
		}

		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSet.descriptorCount = Textures.size();
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferArrayInfos.push_back(std::move(ImageInfo));
	}

	void SetStorageImage(const std::vector<texture>& Textures, VkImageLayout Layout, u32 ViewIdx = 0, u32 Set = 0)
	{
		if(Textures.size() == 0)
		{
			SetIndices[Set] += 1;
			return;
		}
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), Textures.size()));
		for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
		{
			ImageInfo[TextureIdx].imageLayout = Layout;
			ImageInfo[TextureIdx].imageView = Textures[TextureIdx].Views[ViewIdx];
			ImageInfo[TextureIdx].sampler = Textures[TextureIdx].Sampler.Handle;
		}

		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = Textures.size();
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferArrayInfos.push_back(std::move(ImageInfo));
	}

	void SetImageSampler(const std::vector<texture>& Textures, VkImageLayout Layout, u32 ViewIdx = 0, u32 Set = 0)
	{
		if(Textures.size() == 0)
		{
			SetIndices[Set] += 1;
			return;
		}
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), Textures.size()));
		for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
		{
			ImageInfo[TextureIdx].imageLayout = Layout;
			ImageInfo[TextureIdx].imageView = Textures[TextureIdx].Views[ViewIdx];
			ImageInfo[TextureIdx].sampler = Textures[TextureIdx].Sampler.Handle;
		}

		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = Textures.size();
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferArrayInfos.push_back(std::move(ImageInfo));
	}
	
private:
	
	std::vector<texture> ColorTargets;
	texture DepthTarget;

	u32 GlobalOffset = 0;
	u32 PushConstantIdx = 0;

	VkRenderingInfoKHR RenderingInfo;
	std::map<u32, u32> SetIndices;

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	std::vector<VkWriteDescriptorSet> PushDescriptorBindings;
	std::vector<VkWriteDescriptorSet> StaticDescriptorBindings;
	std::vector<std::unique_ptr<descriptor_info>>   BufferInfos;
	std::vector<std::unique_ptr<descriptor_info[]>> BufferArrayInfos;
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR>>   RenderingAttachmentInfos;
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR[]>> RenderingAttachmentInfoArrays;

	const shader_input* InputSignature;
	global_pipeline_context PipelineContext;

	VkDevice Device;
	VkPipeline Pipeline;
};

class compute_context
{
public:
	compute_context() = default;

	compute_context(renderer_backend* Backend, shader_input* Signature, const std::string& Shader) :
		InputSignature(Signature)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		Device = Gfx->Device;

		ComputeStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		ComputeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
		ComputeStage.module = Gfx->LoadShaderModule(Shader.c_str());
		ComputeStage.pName  = "main";

		VkComputePipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
		CreateInfo.stage  = ComputeStage;
		CreateInfo.layout = Signature->Handle;

		VK_CHECK(vkCreateComputePipelines(Device, nullptr, 1, &CreateInfo, 0, &Pipeline));
	}

	compute_context(const compute_context&) = delete;
	compute_context& operator=(const compute_context&) = delete;

    compute_context(compute_context&& other) noexcept : 
		ComputeStage(std::move(other.ComputeStage)),
		GlobalOffset(std::move(other.GlobalOffset)),
		PushConstantIdx(std::move(other.PushConstantIdx)),
		PushDescriptorBindings(std::move(other.PushDescriptorBindings)),
		StaticDescriptorBindings(std::move(other.StaticDescriptorBindings)),
		BufferInfos(std::move(other.BufferInfos)),
		BufferArrayInfos(std::move(other.BufferArrayInfos)),
		SetIndices(std::move(other.SetIndices)),
		InputSignature(other.InputSignature),
		PipelineContext(std::move(other.PipelineContext)),
		Device(std::move(other.Device)),
		Pipeline(other.Pipeline)
    {
    }

    // Move assignment operator
    compute_context& operator=(compute_context&& other) noexcept 
	{
        if (this != &other) 
		{
			std::swap(ComputeStage, other.ComputeStage);

			std::swap(GlobalOffset, other.GlobalOffset);
			std::swap(PushConstantIdx, other.PushConstantIdx);

			std::swap(PushDescriptorBindings, other.PushDescriptorBindings);
			std::swap(StaticDescriptorBindings, other.StaticDescriptorBindings);
			std::swap(BufferInfos, other.BufferInfos);
			std::swap(BufferArrayInfos, other.BufferArrayInfos);

			std::swap(SetIndices, other.SetIndices);

			InputSignature = other.InputSignature;
			other.InputSignature = nullptr;

			std::swap(PipelineContext, other.PipelineContext);

			std::swap(Device, other.Device);
			std::swap(Pipeline, other.Pipeline);
        }
        return *this;
    }

	void DestroyObject()
	{
		PipelineContext = {};
		vkDestroyShaderModule(Device, ComputeStage.module, nullptr);
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}

	void Begin(const global_pipeline_context& GlobalPipelineContext)
	{
		PipelineContext = GlobalPipelineContext;

		std::vector<VkDescriptorSet> SetsToBind;
		for(const VkDescriptorSet& DescriptorSet : InputSignature->Sets)
		{
			if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
		}

		vkCmdBindPipeline(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
		if(SetsToBind.size())
			vkCmdBindDescriptorSets(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, InputSignature->Handle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);
	}

	void End()
	{
		GlobalOffset = 0;
		PushConstantIdx = 0;

		SetIndices.clear();
		PushDescriptorBindings.clear();
		StaticDescriptorBindings.clear();
		std::vector<std::unique_ptr<descriptor_info>>().swap(BufferInfos);
		std::vector<std::unique_ptr<descriptor_info[]>>().swap(BufferArrayInfos);
	}

	void StaticUpdate()
	{
		vkUpdateDescriptorSets(Device, StaticDescriptorBindings.size(), StaticDescriptorBindings.data(), 0, nullptr);
	}

	void Execute(u32 X = 1, u32 Y = 1, u32 Z = 1)
	{
		vkCmdPushDescriptorSetKHR(*PipelineContext.CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
		vkCmdDispatch(*PipelineContext.CommandList, (X + 31) / 32, (Y + 31) / 32, (Z + 31) / 32);
		SetIndices.clear();
		GlobalOffset = 0;
		PushConstantIdx = 0;
	}

	void SetConstant(void* Data, size_t Size)
	{
		vkCmdPushConstants(*PipelineContext.CommandList, InputSignature->Handle, InputSignature->PushConstants[PushConstantIdx++].stageFlags, GlobalOffset, Size, Data);
		GlobalOffset += Size;
	}

	void SetStorageBufferView(const buffer& Buffer, bool UseCounter = true, u32 Set = 0)
	{
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
		std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
		BufferInfo->buffer = Buffer.Handle;
		BufferInfo->offset = 0;
		BufferInfo->range  = Buffer.WithCounter ? Buffer.CounterOffset : Buffer.Size;

		VkWriteDescriptorSet CounterDescriptorSet = {};
		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = 1;
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferInfos.push_back(std::move(BufferInfo));

		if(Buffer.WithCounter && UseCounter)
		{
			CounterBufferInfo->buffer = Buffer.Handle;
			CounterBufferInfo->offset = Buffer.CounterOffset;
			CounterBufferInfo->range  = sizeof(u32);

			CounterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			if(!IsPush)
				CounterDescriptorSet.dstSet = InputSignature->Sets[Set];
			CounterDescriptorSet.dstBinding = SetIndices[Set];
			CounterDescriptorSet.descriptorCount = 1;
			CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
			if(IsPush)
				PushDescriptorBindings.push_back(CounterDescriptorSet);
			else
				StaticDescriptorBindings.push_back(CounterDescriptorSet);
			SetIndices[Set] += 1;

			BufferInfos.push_back(std::move(CounterBufferInfo));
		}
	}

	void SetUniformBufferView(const buffer& Buffer, u32 UseCounter = true, u32 Set = 0)
	{
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
		std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
		BufferInfo->buffer = Buffer.Handle;
		BufferInfo->offset = 0;
		BufferInfo->range  = Buffer.WithCounter ? Buffer.CounterOffset : Buffer.Size;

		VkWriteDescriptorSet CounterDescriptorSet = {};
		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = 1;
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferInfos.push_back(std::move(BufferInfo));

		if(Buffer.WithCounter && UseCounter)
		{
			CounterBufferInfo->buffer = Buffer.Handle;
			CounterBufferInfo->offset = Buffer.CounterOffset;
			CounterBufferInfo->range  = sizeof(u32);

			CounterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			if(!IsPush)
				CounterDescriptorSet.dstSet = InputSignature->Sets[Set];
			CounterDescriptorSet.dstBinding = SetIndices[Set];
			CounterDescriptorSet.descriptorCount = 1;
			CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
			if(IsPush)
				PushDescriptorBindings.push_back(CounterDescriptorSet);
			else
				StaticDescriptorBindings.push_back(CounterDescriptorSet);
			SetIndices[Set] += 1;

			BufferInfos.push_back(std::move(CounterBufferInfo));
		}
	}

	// TODO: Remove image layouts and move them inside texture structure
	void SetSampledImage(const std::vector<texture>& Textures, VkImageLayout Layout, u32 ViewIdx = 0, u32 Set = 0)
	{
		if(Textures.size() == 0)
		{
			SetIndices[Set] += 1;
			return;
		}
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), Textures.size()));
		for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
		{
			ImageInfo[TextureIdx].imageLayout = Layout;
			ImageInfo[TextureIdx].imageView = Textures[TextureIdx].Views[ViewIdx];
			ImageInfo[TextureIdx].sampler = Textures[TextureIdx].Sampler.Handle;
		}

		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = Textures.size();
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferArrayInfos.push_back(std::move(ImageInfo));
	}

	void SetStorageImage(const std::vector<texture>& Textures, VkImageLayout Layout, u32 ViewIdx = 0, u32 Set = 0)
	{
		if(Textures.size() == 0)
		{
			SetIndices[Set] += 1;
			return;
		}
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), Textures.size()));
		for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
		{
			ImageInfo[TextureIdx].imageLayout = Layout;
			ImageInfo[TextureIdx].imageView = Textures[TextureIdx].Views[ViewIdx];
			ImageInfo[TextureIdx].sampler = Textures[TextureIdx].Sampler.Handle;
		}

		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = Textures.size();
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferArrayInfos.push_back(std::move(ImageInfo));
	}

	void SetImageSampler(const std::vector<texture>& Textures, VkImageLayout Layout, u32 ViewIdx = 0, u32 Set = 0)
	{
		if(Textures.size() == 0)
		{
			SetIndices[Set] += 1;
			return;
		}
		bool IsPush = InputSignature->IsSetPush.at(Set);
		std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), Textures.size()));
		for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
		{
			ImageInfo[TextureIdx].imageLayout = Layout;
			ImageInfo[TextureIdx].imageView = Textures[TextureIdx].Views[ViewIdx];
			ImageInfo[TextureIdx].sampler = Textures[TextureIdx].Sampler.Handle;
		}

		VkWriteDescriptorSet DescriptorSet = {};
		DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!IsPush)
			DescriptorSet.dstSet = InputSignature->Sets[Set];
		DescriptorSet.dstBinding = SetIndices[Set];
		DescriptorSet.descriptorCount = Textures.size();
		DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
		if(IsPush)
			PushDescriptorBindings.push_back(DescriptorSet);
		else
			StaticDescriptorBindings.push_back(DescriptorSet);
		SetIndices[Set] += 1;

		BufferArrayInfos.push_back(std::move(ImageInfo));
	}

private:

	VkPipelineShaderStageCreateInfo ComputeStage;

	u32 GlobalOffset = 0;
	u32 PushConstantIdx = 0;

	std::vector<VkWriteDescriptorSet> PushDescriptorBindings;
	std::vector<VkWriteDescriptorSet> StaticDescriptorBindings;
	std::vector<std::unique_ptr<descriptor_info>> BufferInfos;
	std::vector<std::unique_ptr<descriptor_info[]>> BufferArrayInfos;

	std::map<u32, u32> SetIndices;

	const shader_input* InputSignature;
	global_pipeline_context PipelineContext;

	VkDevice Device;
	VkPipeline Pipeline;
};
