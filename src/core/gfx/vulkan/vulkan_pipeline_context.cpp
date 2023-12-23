
void vulkan_global_pipeline_context::
CreateResource(renderer_backend* Backend)
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(static_cast<vulkan_backend*>(Backend)->CommandQueue);
	Device = CommandQueue->Device;

	VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &AcquireSemaphore));
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ReleaseSemaphore));

	CommandList = CommandQueue->AllocateCommandList();
}

void vulkan_global_pipeline_context::
DestroyObject()
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

void vulkan_global_pipeline_context::
Begin(renderer_backend* Backend)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->Reset(CommandList);

	vkAcquireNextImageKHR(Device, Gfx->Swapchain, ~0ull, AcquireSemaphore, VK_NULL_HANDLE, &BackBufferIndex);
}

void vulkan_global_pipeline_context::
End(renderer_backend* Backend)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
}

void vulkan_global_pipeline_context::
DeviceWaitIdle()
{
	vkDeviceWaitIdle(Device);
}

void vulkan_global_pipeline_context::
EndOneTime(renderer_backend* Backend)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->ExecuteAndRemove(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
}

void vulkan_global_pipeline_context::
EmplaceColorTarget(renderer_backend* Backend, texture* RenderTexture)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_texture* Texture = static_cast<vulkan_texture*>(RenderTexture);

	std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
	{
		CreateImageBarrier(Texture->Handle, AF_ColorAttachmentWrite, AF_TransferRead, GetVKImageLayout(image_barrier_state::color_attachment), GetVKImageLayout(image_barrier_state::transfer_src)),
		CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], 0, AF_TransferWrite, GetVKImageLayout(image_barrier_state::undefined), GetVKImageLayout(image_barrier_state::transfer_dst)),
	};
	ImageBarrier(*CommandList, PSF_ColorAttachment, PSF_Transfer, ImageCopyBarriers);

	VkImageCopy ImageCopyRegion = {};
	ImageCopyRegion.srcSubresource.aspectMask = Texture->Aspect;
	ImageCopyRegion.srcSubresource.layerCount = 1;
	ImageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageCopyRegion.dstSubresource.layerCount = 1;
	ImageCopyRegion.extent = {u32(Texture->Width), u32(Texture->Height), u32(Texture->Depth)};

	vkCmdCopyImage(*CommandList, Texture->Handle, GetVKImageLayout(image_barrier_state::transfer_src), Gfx->SwapchainImages[BackBufferIndex], GetVKImageLayout(image_barrier_state::transfer_dst), 1, &ImageCopyRegion);
}

void vulkan_global_pipeline_context::
Present(renderer_backend* Backend)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);

	std::vector<VkImageMemoryBarrier> ImageEndRenderBarriers = 
	{
		CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], AF_TransferWrite, 0, GetVKImageLayout(image_barrier_state::transfer_dst), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	};
	ImageBarrier(*CommandList, PSF_Transfer, PSF_BottomOfPipe, ImageEndRenderBarriers);

	CommandQueue->Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);

	// NOTE: It shouldn't be there
	VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = &ReleaseSemaphore;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &Gfx->Swapchain;
	PresentInfo.pImageIndices = &BackBufferIndex;
	vkQueuePresentKHR(CommandQueue->Handle, &PresentInfo);
}

void vulkan_global_pipeline_context::
FillBuffer(buffer* Buffer, u32 Value)
{
	vulkan_buffer* VulkanBuffer = static_cast<vulkan_buffer*>(Buffer);

	vkCmdFillBuffer(*CommandList, VulkanBuffer->Handle, 0, VulkanBuffer->Size, Value);
}

void vulkan_global_pipeline_context::
CopyImage(texture* Dst, texture* Src)
{
	vulkan_texture* SrcTexture = static_cast<vulkan_texture*>(Src);
	vulkan_texture* DstTexture = static_cast<vulkan_texture*>(Dst);

	std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
	{
		CreateImageBarrier(SrcTexture->Handle, 0, AF_TransferWrite, GetVKImageLayout(image_barrier_state::undefined), GetVKImageLayout(image_barrier_state::transfer_src)),
		CreateImageBarrier(DstTexture->Handle, 0, AF_TransferWrite, GetVKImageLayout(image_barrier_state::undefined), GetVKImageLayout(image_barrier_state::transfer_dst)),
	};
	ImageBarrier(*CommandList, PSF_ColorAttachment, PSF_Transfer, ImageCopyBarriers);

	VkImageCopy ImageCopyRegion = {};
	ImageCopyRegion.srcSubresource.aspectMask = SrcTexture->Aspect;
	ImageCopyRegion.srcSubresource.layerCount = 1;
	ImageCopyRegion.dstSubresource.aspectMask = DstTexture->Aspect;
	ImageCopyRegion.dstSubresource.layerCount = 1;
	ImageCopyRegion.extent = {(u32)Src->Width, (u32)Src->Height, 1};

	vkCmdCopyImage(*CommandList, SrcTexture->Handle, GetVKImageLayout(image_barrier_state::transfer_src), DstTexture->Handle, GetVKImageLayout(image_barrier_state::transfer_dst), 1, &ImageCopyRegion);
}

void vulkan_global_pipeline_context::
SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	VkMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
	Barrier.srcAccessMask = GetVKAccessMask(SrcAccess);
	Barrier.dstAccessMask = GetVKAccessMask(DstAccess);

	vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 1, &Barrier, 0, 0, 0, 0);
}

void vulkan_global_pipeline_context::
SetBufferBarrier(const std::tuple<buffer*, u32, u32>& BarrierData, 
					  u32 SrcStageMask, u32 DstStageMask)
{
	vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(std::get<0>(BarrierData));
	VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	Barrier.buffer = Buffer->Handle;
	Barrier.srcAccessMask = GetVKAccessMask(std::get<1>(BarrierData));
	Barrier.dstAccessMask = GetVKAccessMask(std::get<2>(BarrierData));
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.offset = 0;
	Barrier.size = Buffer->Size;

	vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &Barrier, 0, 0);
}

void vulkan_global_pipeline_context::
SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData, 
					   u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<VkBufferMemoryBarrier> Barriers;
	for(const std::tuple<buffer*, u32, u32>& Data : BarrierData)
	{
		vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(std::get<0>(Data));
		VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		Barrier.buffer = Buffer->Handle;
		Barrier.srcAccessMask = GetVKAccessMask(std::get<1>(Data));
		Barrier.dstAccessMask = GetVKAccessMask(std::get<2>(Data));
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.offset = 0;
		Barrier.size = Buffer->Size;

		Barriers.push_back(Barrier);
	}

	vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 0, 0, Barriers.size(), Barriers.data(), 0, 0);
}

void vulkan_global_pipeline_context::
SetImageBarriers(const std::vector<std::tuple<texture*, u32, u32, image_barrier_state, image_barrier_state>>& BarrierData, 
					  u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<VkImageMemoryBarrier> Barriers;
	for(const std::tuple<texture*, u32, u32, image_barrier_state, image_barrier_state>& Data : BarrierData)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(std::get<0>(Data));

		VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		Barrier.srcAccessMask = GetVKAccessMask(std::get<1>(Data));
		Barrier.dstAccessMask = GetVKAccessMask(std::get<2>(Data));
		Barrier.oldLayout = GetVKImageLayout(std::get<3>(Data));
		Barrier.newLayout = GetVKImageLayout(std::get<4>(Data));
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.image = Texture->Handle;
		Barrier.subresourceRange.aspectMask = Texture->Aspect;
		Barrier.subresourceRange.baseMipLevel   = 0;
		Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		Barriers.push_back(Barrier);
	}

	if(Barriers.size())
	{
		vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), 
							 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 
							 (u32)Barriers.size(), Barriers.data());
	}
}

void vulkan_global_pipeline_context::
SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, u32, image_barrier_state, image_barrier_state>>& BarrierData, 
					  u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<VkImageMemoryBarrier> Barriers;
	for(const std::tuple<std::vector<texture*>, u32, u32, image_barrier_state, image_barrier_state>& Data : BarrierData)
	{
		const std::vector<texture*>& Textures = std::get<0>(Data);
		if(!Textures.size()) continue;
		for(texture* TextureData : Textures)
		{
			vulkan_texture* Texture = static_cast<vulkan_texture*>(TextureData);

			VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			Barrier.srcAccessMask = GetVKAccessMask(std::get<1>(Data));
			Barrier.dstAccessMask = GetVKAccessMask(std::get<2>(Data));
			Barrier.oldLayout = GetVKImageLayout(std::get<3>(Data));
			Barrier.newLayout = GetVKImageLayout(std::get<4>(Data));
			Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.image = Texture->Handle;
			Barrier.subresourceRange.aspectMask = Texture->Aspect;
			Barrier.subresourceRange.baseMipLevel   = 0;
			Barrier.subresourceRange.baseArrayLayer = 0;
			Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			Barriers.push_back(Barrier);
		}
	}

	if(Barriers.size())
	{
		vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), 
							 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 
							 (u32)Barriers.size(), Barriers.data());
	}
}

vulkan_render_context::
vulkan_render_context(renderer_backend* Backend,
			   shader_input* Signature,
			   std::initializer_list<const std::string> ShaderList, const std::vector<texture*>& ColorTargets, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines) 
			 : InputSignature(static_cast<vulkan_shader_input*>(Signature))
{
	RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	Device = Gfx->Device;

	for(const std::string Shader : ShaderList)
	{
		VkPipelineShaderStageCreateInfo Stage = {};
		Stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Stage.pName  = "main";
		if(Shader.find(".vert.") != std::string::npos)
		{
			Stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::vertex, ShaderDefines);
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			Stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_control, ShaderDefines);
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			Stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_eval, ShaderDefines);
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			Stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::geometry, ShaderDefines);
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			Stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::fragment, ShaderDefines);
		}
		ShaderStages.push_back(Stage);
	}

	VkGraphicsPipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

	CreateInfo.layout = InputSignature->Handle;
	CreateInfo.pStages = ShaderStages.data();
	CreateInfo.stageCount = ShaderStages.size();

	std::vector<VkFormat> ColorTargetFormats;
	for(u32 FormatIdx = 0; FormatIdx < ColorTargets.size(); ++FormatIdx) ColorTargetFormats.push_back(GetVKFormat(ColorTargets[FormatIdx]->Info.Format));

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

void vulkan_render_context::
DestroyObject()
{
	PipelineContext = {};
	for(VkPipelineShaderStageCreateInfo& Stage : ShaderStages)
	{
		vkDestroyShaderModule(Device, Stage.module, nullptr);
	}
	vkDestroyPipeline(Device, Pipeline, nullptr);
}

void vulkan_render_context::
Begin(global_pipeline_context* GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight)
{
	PipelineContext = static_cast<vulkan_global_pipeline_context*>(GlobalPipelineContext);

	std::vector<VkDescriptorSet> SetsToBind;
	for(const VkDescriptorSet& DescriptorSet : InputSignature->Sets)
	{
		if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
	}

	vkCmdBeginRenderingKHR(*PipelineContext->CommandList, &RenderingInfo);

	vkCmdBindPipeline(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
	if(SetsToBind.size())
		vkCmdBindDescriptorSets(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);

	VkViewport Viewport = {0, (float)RenderHeight, (float)RenderWidth, -(float)RenderHeight, 0, 1};
	VkRect2D Scissor = {{0, 0}, {RenderWidth, RenderHeight}};
	vkCmdSetViewport(*PipelineContext->CommandList, 0, 1, &Viewport);
	vkCmdSetScissor(*PipelineContext->CommandList, 0, 1, &Scissor);
}

void vulkan_render_context::
End()
{
	GlobalOffset = 0;
	PushConstantIdx = 0;

	SetIndices.clear();
	PushDescriptorBindings.clear();
	StaticDescriptorBindings.clear();
	std::vector<std::unique_ptr<descriptor_info>>().swap(BufferInfos);
	std::vector<std::unique_ptr<descriptor_info[]>>().swap(BufferArrayInfos);
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR>>().swap(RenderingAttachmentInfos);
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR[]>>().swap(RenderingAttachmentInfoArrays);

	vkCmdEndRenderingKHR(*PipelineContext->CommandList);
}

void vulkan_render_context::
SetColorTarget(load_op LoadOp, store_op StoreOp, u32 RenderWidth, u32 RenderHeight, const std::vector<texture*>& ColorAttachments, vec4 Clear, u32 Face, bool EnableMultiview)
{
	RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
	RenderingInfo.layerCount = 1;
	RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

	std::unique_ptr<VkRenderingAttachmentInfoKHR[]> ColorInfo((VkRenderingAttachmentInfoKHR*)calloc(sizeof(VkRenderingAttachmentInfoKHR), ColorAttachments.size()));
	for(u32 AttachmentIdx = 0; AttachmentIdx < ColorAttachments.size(); ++AttachmentIdx)
	{
		vulkan_texture* Attachment = static_cast<vulkan_texture*>(ColorAttachments[AttachmentIdx]);

		ColorInfo[AttachmentIdx].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		ColorInfo[AttachmentIdx].imageView = Attachment->Views[0];
		ColorInfo[AttachmentIdx].imageLayout = GetVKImageLayout(image_barrier_state::color_attachment);
		ColorInfo[AttachmentIdx].loadOp = GetVKLoadOp(LoadOp);
		ColorInfo[AttachmentIdx].storeOp = GetVKStoreOp(StoreOp);
		memcpy(ColorInfo[AttachmentIdx].clearValue.color.float32, Clear.E, 4 * sizeof(float));
	}
	RenderingInfo.colorAttachmentCount = ColorAttachments.size();
	RenderingInfo.pColorAttachments = ColorInfo.get();

	RenderingAttachmentInfoArrays.push_back(std::move(ColorInfo));
}

void vulkan_render_context::
SetDepthTarget(load_op LoadOp, store_op StoreOp, u32 RenderWidth, u32 RenderHeight, texture* DepthAttachment, vec2 Clear, u32 Face, bool EnableMultiview)
{
	vulkan_texture* Attachment = static_cast<vulkan_texture*>(DepthAttachment);

	RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
	RenderingInfo.layerCount = 1;
	RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

	std::unique_ptr<VkRenderingAttachmentInfoKHR> DepthInfo = std::make_unique<VkRenderingAttachmentInfoKHR>();
	DepthInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	DepthInfo->imageView = Attachment->Views[0];
	DepthInfo->imageLayout = GetVKImageLayout(image_barrier_state::depth_stencil_attachment);
	DepthInfo->loadOp = GetVKLoadOp(LoadOp);
	DepthInfo->storeOp = GetVKStoreOp(StoreOp);
	DepthInfo->clearValue.depthStencil.depth   = Clear.E[0];
	DepthInfo->clearValue.depthStencil.stencil = Clear.E[1];
	RenderingInfo.pDepthAttachment = DepthInfo.get();

	RenderingAttachmentInfos.push_back(std::move(DepthInfo));
}

void vulkan_render_context::
SetStencilTarget(load_op LoadOp, store_op StoreOp, u32 RenderWidth, u32 RenderHeight, texture* StencilAttachment, vec2 Clear, u32 Face, bool EnableMultiview)
{
	vulkan_texture* Attachment = static_cast<vulkan_texture*>(StencilAttachment);

	RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
	RenderingInfo.layerCount = 1;
	RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

	std::unique_ptr<VkRenderingAttachmentInfoKHR> StencilInfo = std::make_unique<VkRenderingAttachmentInfoKHR>();
	StencilInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	StencilInfo->imageView = Attachment->Views[0];
	StencilInfo->imageLayout = GetVKImageLayout(image_barrier_state::depth_stencil_attachment);
	StencilInfo->loadOp = GetVKLoadOp(LoadOp);
	StencilInfo->storeOp = GetVKStoreOp(StoreOp);
	StencilInfo->clearValue.depthStencil.depth   = Clear.E[0];
	StencilInfo->clearValue.depthStencil.stencil = Clear.E[1];
	RenderingInfo.pStencilAttachment = StencilInfo.get();

	RenderingAttachmentInfos.push_back(std::move(StencilInfo));
}

void vulkan_render_context::
StaticUpdate()
{
	vkUpdateDescriptorSets(Device, StaticDescriptorBindings.size(), StaticDescriptorBindings.data(), 0, nullptr);
}

void vulkan_render_context::
Draw(buffer* VertexBuffer, u32 FirstVertex, u32 VertexCount)
{
	vulkan_buffer* VertexAttachment = static_cast<vulkan_buffer*>(VertexBuffer);

	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	vkCmdDraw(*PipelineContext->CommandList, VertexCount, 1, FirstVertex, 0);
	SetIndices.clear();
	GlobalOffset = 0;
	PushConstantIdx = 0;
}

void vulkan_render_context::
DrawIndexed(buffer* IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount)
{
	vulkan_buffer* IndexAttachment = static_cast<vulkan_buffer*>(IndexBuffer);

	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	vkCmdBindIndexBuffer(*PipelineContext->CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(*PipelineContext->CommandList, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
	SetIndices.clear();
	GlobalOffset = 0;
	PushConstantIdx = 0;
}

void vulkan_render_context::
DrawIndirect(u32 ObjectDrawCount, buffer* IndexBuffer, buffer* IndirectCommands, u32 CommandStructureSize)
{
	vulkan_buffer* IndexAttachment = static_cast<vulkan_buffer*>(IndexBuffer);
	vulkan_buffer* IndirectCommandsAttachment = static_cast<vulkan_buffer*>(IndirectCommands);

	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	vkCmdBindIndexBuffer(*PipelineContext->CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexedIndirectCount(*PipelineContext->CommandList, IndirectCommandsAttachment->Handle, 0, IndirectCommandsAttachment->Handle, IndirectCommandsAttachment->CounterOffset, ObjectDrawCount, CommandStructureSize);
	SetIndices.clear();
	GlobalOffset = 0;
	PushConstantIdx = 0;
}

void vulkan_render_context::
SetConstant(void* Data, size_t Size)
{
	vkCmdPushConstants(*PipelineContext->CommandList, InputSignature->Handle, InputSignature->PushConstants[PushConstantIdx++].stageFlags, GlobalOffset, Size, Data);
	GlobalOffset += Size;
}

// NOTE: If with counter, then it is using 2 bindings instead of 1
void vulkan_render_context::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);

	bool IsPush = InputSignature->IsSetPush.at(Set);
	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

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

	if(Buffer->WithCounter && UseCounter)
	{
		CounterBufferInfo->buffer = Attachment->Handle;
		CounterBufferInfo->offset = Attachment->CounterOffset;
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

void vulkan_render_context::
SetUniformBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);

	bool IsPush = InputSignature->IsSetPush.at(Set);
	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

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

	if(Buffer->WithCounter && UseCounter)
	{
		CounterBufferInfo->buffer = Attachment->Handle;
		CounterBufferInfo->offset = Attachment->CounterOffset;
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
void vulkan_render_context::
SetSampledImage(const std::vector<texture*>& Textures, image_barrier_state State, u32 ViewIdx, u32 Set)
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
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKImageLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
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

void vulkan_render_context::
SetStorageImage(const std::vector<texture*>& Textures, image_barrier_state State, u32 ViewIdx, u32 Set)
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
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKImageLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
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

void vulkan_render_context::
SetImageSampler(const std::vector<texture*>& Textures, image_barrier_state State, u32 ViewIdx, u32 Set)
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
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKImageLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
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

vulkan_compute_context::
vulkan_compute_context(renderer_backend* Backend, shader_input* Signature, const std::string& Shader, const std::vector<shader_define>& ShaderDefines) :
	InputSignature(static_cast<vulkan_shader_input*>(Signature))
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	Device = Gfx->Device;

	ComputeStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	ComputeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	ComputeStage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::compute, ShaderDefines);
	ComputeStage.pName  = "main";

	VkComputePipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	CreateInfo.stage  = ComputeStage;
	CreateInfo.layout = InputSignature->Handle;

	VK_CHECK(vkCreateComputePipelines(Device, nullptr, 1, &CreateInfo, 0, &Pipeline));
}

void vulkan_compute_context::
DestroyObject()
{
	PipelineContext = {};
	vkDestroyShaderModule(Device, ComputeStage.module, nullptr);
	vkDestroyPipeline(Device, Pipeline, nullptr);
}

void vulkan_compute_context::
Begin(global_pipeline_context* GlobalPipelineContext)
{
	PipelineContext = static_cast<vulkan_global_pipeline_context*>(GlobalPipelineContext);

	std::vector<VkDescriptorSet> SetsToBind;
	for(const VkDescriptorSet& DescriptorSet : InputSignature->Sets)
	{
		if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
	}

	vkCmdBindPipeline(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
	if(SetsToBind.size())
		vkCmdBindDescriptorSets(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, InputSignature->Handle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);
}

void vulkan_compute_context::
End()
{
	GlobalOffset = 0;
	PushConstantIdx = 0;

	SetIndices.clear();
	PushDescriptorBindings.clear();
	StaticDescriptorBindings.clear();
	std::vector<std::unique_ptr<descriptor_info>>().swap(BufferInfos);
	std::vector<std::unique_ptr<descriptor_info[]>>().swap(BufferArrayInfos);
}

void vulkan_compute_context::
StaticUpdate()
{
	vkUpdateDescriptorSets(Device, StaticDescriptorBindings.size(), StaticDescriptorBindings.data(), 0, nullptr);
}

void vulkan_compute_context::
Execute(u32 X, u32 Y, u32 Z)
{
	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, InputSignature->Handle, InputSignature->PushDescriptorSetIdx, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	vkCmdDispatch(*PipelineContext->CommandList, (X + 31) / 32, (Y + 31) / 32, (Z + 31) / 32);
	SetIndices.clear();
	GlobalOffset = 0;
	PushConstantIdx = 0;
}

void vulkan_compute_context::
SetConstant(void* Data, size_t Size)
{
	vkCmdPushConstants(*PipelineContext->CommandList, InputSignature->Handle, InputSignature->PushConstants[PushConstantIdx++].stageFlags, GlobalOffset, Size, Data);
	GlobalOffset += Size;
}

void vulkan_compute_context::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);

	bool IsPush = InputSignature->IsSetPush.at(Set);
	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

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

	if(Buffer->WithCounter && UseCounter)
	{
		CounterBufferInfo->buffer = Attachment->Handle;
		CounterBufferInfo->offset = Attachment->CounterOffset;
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

void vulkan_compute_context::
SetUniformBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);

	bool IsPush = InputSignature->IsSetPush.at(Set);
	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

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

	if(Buffer->WithCounter && UseCounter)
	{
		CounterBufferInfo->buffer = Attachment->Handle;
		CounterBufferInfo->offset = Attachment->CounterOffset;
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
void vulkan_compute_context::
SetSampledImage(const std::vector<texture*>& Textures, image_barrier_state State, u32 ViewIdx, u32 Set)
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
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKImageLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
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

void vulkan_compute_context::
SetStorageImage(const std::vector<texture*>& Textures, image_barrier_state State, u32 ViewIdx, u32 Set)
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
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKImageLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
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

void vulkan_compute_context::
SetImageSampler(const std::vector<texture*>& Textures, image_barrier_state State, u32 ViewIdx, u32 Set)
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
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKImageLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
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
