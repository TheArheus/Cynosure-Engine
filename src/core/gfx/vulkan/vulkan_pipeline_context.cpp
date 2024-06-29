
void vulkan_global_pipeline_context::
CreateResource(renderer_backend* Backend)
{
	Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
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
AcquireNextImage()
{
	vkAcquireNextImageKHR(Device, Gfx->Swapchain, ~0ull, AcquireSemaphore, VK_NULL_HANDLE, &BackBufferIndex);
}

void vulkan_global_pipeline_context::
Begin()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->Reset(CommandList);

	SetImageBarriers({
						{Gfx->NullTexture1D, AF_ShaderRead, barrier_state::shader_read, ~0u}, 
						{Gfx->NullTexture2D, AF_ShaderRead, barrier_state::shader_read, ~0u}, 
						{Gfx->NullTexture3D, AF_ShaderRead, barrier_state::shader_read, ~0u},
						{Gfx->NullTextureCube, AF_ShaderRead, barrier_state::shader_read, ~0u}, 
					 }, PSF_TopOfPipe, PSF_Compute);

	TexturesToCommon.insert(Gfx->NullTexture1D);
	TexturesToCommon.insert(Gfx->NullTexture2D);
	TexturesToCommon.insert(Gfx->NullTexture3D);
	TexturesToCommon.insert(Gfx->NullTextureCube);
}

void vulkan_global_pipeline_context::
End()
{
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
}

void vulkan_global_pipeline_context::
DeviceWaitIdle()
{
	vkDeviceWaitIdle(Device);
}

void vulkan_global_pipeline_context::
EndOneTime()
{
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->ExecuteAndRemove(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
}

void vulkan_global_pipeline_context::
EmplaceColorTarget(texture* RenderTexture)
{
	vulkan_texture* Texture = static_cast<vulkan_texture*>(RenderTexture);

	std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
	{
		CreateImageBarrier(Texture->Handle, GetVKAccessMask(Texture->CurrentLayout[0], Texture->PrevShader), GetVKAccessMask(AF_TransferRead, PSF_Transfer), GetVKLayout(Texture->CurrentState[0]), GetVKLayout(barrier_state::transfer_src)),
		CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], 0, GetVKAccessMask(AF_TransferWrite, PSF_Transfer), GetVKLayout(barrier_state::undefined), GetVKLayout(barrier_state::transfer_dst)),
	};
	ImageBarrier(*CommandList, GetVKPipelineStage(PSF_ColorAttachment), GetVKPipelineStage(PSF_Transfer), ImageCopyBarriers);

	VkImageCopy ImageCopyRegion = {};
	ImageCopyRegion.srcSubresource.aspectMask = Texture->Aspect;
	ImageCopyRegion.srcSubresource.layerCount = 1;
	ImageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageCopyRegion.dstSubresource.layerCount = 1;
	ImageCopyRegion.extent = {u32(Texture->Width), u32(Texture->Height), u32(Texture->Depth)};

	vkCmdCopyImage(*CommandList, Texture->Handle, GetVKLayout(barrier_state::transfer_src), Gfx->SwapchainImages[BackBufferIndex], GetVKLayout(barrier_state::transfer_dst), 1, &ImageCopyRegion);
}

void vulkan_global_pipeline_context::
Present()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);

	std::vector<VkImageMemoryBarrier> ImageEndRenderBarriers = 
	{
		CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], GetVKAccessMask(AF_TransferWrite, PSF_Transfer), 0, GetVKLayout(barrier_state::transfer_dst), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	};
	ImageBarrier(*CommandList, GetVKPipelineStage(PSF_Transfer), GetVKPipelineStage(PSF_BottomOfPipe), ImageEndRenderBarriers);

	for(u32 Idx = 0; Idx < BuffersToCommon.size(); ++Idx)
	{
		directx12_buffer* Resource = static_cast<directx12_buffer*>(*std::next(BuffersToCommon.begin(), Idx));

		Resource->CurrentLayout = 0;
		Resource->PrevShader = 0;
	}

	for(u32 Idx = 0; Idx < TexturesToCommon.size(); ++Idx)
	{
		directx12_texture* Resource = static_cast<directx12_texture*>(*std::next(TexturesToCommon.begin(), Idx));

		std::fill(Resource->CurrentLayout.begin(), Resource->CurrentLayout.end(), 0);
		std::fill(Resource->CurrentState.begin(), Resource->CurrentState.end(), barrier_state::undefined);
		Resource->PrevShader = 0;
	}

	BuffersToCommon.clear();
	TexturesToCommon.clear();
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
FillTexture(texture* Texture, vec4 Value)
{
	VkClearColorValue ClearColor = {};
	ClearColor.float32[0] = Value.x;
	ClearColor.float32[1] = Value.y;
	ClearColor.float32[2] = Value.z;
	ClearColor.float32[3] = Value.w;

	VkImageSubresourceRange ClearRange = {};
	ClearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ClearRange.baseMipLevel = 0;
	ClearRange.baseArrayLayer = 0;
	ClearRange.levelCount = VK_REMAINING_MIP_LEVELS;
	ClearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCmdClearColorImage(*CommandList, static_cast<vulkan_texture*>(Texture)->Handle, GetVKLayout(Texture->CurrentState[0]), &ClearColor, 1, &ClearRange);
}

void vulkan_global_pipeline_context::
CopyImage(texture* Dst, texture* Src)
{
	vulkan_texture* SrcTexture = static_cast<vulkan_texture*>(Src);
	vulkan_texture* DstTexture = static_cast<vulkan_texture*>(Dst);

	SetImageBarriers({{SrcTexture, AF_TransferWrite, barrier_state::transfer_src, ~0u}, 
					  {DstTexture, AF_TransferWrite, barrier_state::transfer_dst, ~0u}}, 
					 PSF_ColorAttachment, PSF_Transfer);

	VkImageCopy ImageCopyRegion = {};
	ImageCopyRegion.srcSubresource.aspectMask = SrcTexture->Aspect;
	ImageCopyRegion.srcSubresource.layerCount = 1;
	ImageCopyRegion.dstSubresource.aspectMask = DstTexture->Aspect;
	ImageCopyRegion.dstSubresource.layerCount = 1;
	ImageCopyRegion.extent = {(u32)Src->Width, (u32)Src->Height, 1};

	vkCmdCopyImage(*CommandList, SrcTexture->Handle, GetVKLayout(barrier_state::transfer_src), DstTexture->Handle, GetVKLayout(barrier_state::transfer_dst), 1, &ImageCopyRegion);
}

void vulkan_global_pipeline_context::
GenerateMips(texture* Texture)
{
	vulkan_texture* VulkanTexture = static_cast<vulkan_texture*>(Texture);

	SetImageBarriers({{VulkanTexture, AF_TransferWrite, barrier_state::transfer_dst, ~0u}}, PSF_Transfer, PSF_Transfer);
	SetImageBarriers({{VulkanTexture, AF_TransferRead, barrier_state::transfer_src, 0}}, PSF_Transfer, PSF_Transfer);

	for(u32 MipIdx = 1; MipIdx < VulkanTexture->Info.MipLevels; ++MipIdx)
	{
		VkImageBlit Blit = {};
		Blit.srcSubresource.aspectMask = VulkanTexture->Aspect;
		Blit.srcSubresource.layerCount = 1;
		Blit.srcSubresource.mipLevel   = MipIdx - 1;
		Blit.srcOffsets[1].x = Max(1, s32(Texture->Width  >> (MipIdx - 1)));
		Blit.srcOffsets[1].y = Max(1, s32(Texture->Height >> (MipIdx - 1)));
		Blit.srcOffsets[1].z = Max(1, s32(Texture->Depth  >> (MipIdx - 1)));

		Blit.dstSubresource.aspectMask = VulkanTexture->Aspect;
		Blit.dstSubresource.layerCount = 1;
		Blit.dstSubresource.mipLevel   = MipIdx;
		Blit.dstOffsets[1].x = Max(1, s32(Texture->Width  >> MipIdx));
		Blit.dstOffsets[1].y = Max(1, s32(Texture->Height >> MipIdx));
		Blit.dstOffsets[1].z = Max(1, s32(Texture->Depth  >> MipIdx));

		vkCmdBlitImage(*CommandList, VulkanTexture->Handle, GetVKLayout(barrier_state::transfer_src), VulkanTexture->Handle, GetVKLayout(barrier_state::transfer_dst), 1, &Blit, VK_FILTER_LINEAR);

		SetImageBarriers({{VulkanTexture, AF_TransferRead, barrier_state::transfer_src, MipIdx}}, PSF_Transfer, PSF_Transfer);
	}

	SetImageBarriers({{VulkanTexture, 0, VulkanTexture->CurrentState[0], ~0u}}, PSF_Transfer, PSF_Transfer);
}

void vulkan_global_pipeline_context::
SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	VkMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
	Barrier.srcAccessMask = GetVKAccessMask(SrcAccess, SrcStageMask);
	Barrier.dstAccessMask = GetVKAccessMask(DstAccess, DstStageMask);

	vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 1, &Barrier, 0, 0, 0, 0);
}

void vulkan_global_pipeline_context::
SetBufferBarrier(const std::tuple<buffer*, u32>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	u32 ResourceLayoutNext = std::get<1>(BarrierData);

	vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(std::get<0>(BarrierData));
	VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
	Barrier.buffer = Buffer->Handle;
	Barrier.srcAccessMask = GetVKAccessMask(Buffer->CurrentLayout, DstStageMask);
	Barrier.dstAccessMask = GetVKAccessMask(ResourceLayoutNext, DstStageMask);
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.offset = 0;
	Barrier.size = Buffer->Size;

	Buffer->CurrentLayout = ResourceLayoutNext;
	Buffer->PrevShader = DstStageMask;

	vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &Barrier, 0, 0);
}

void vulkan_global_pipeline_context::
SetBufferBarriers(const std::vector<std::tuple<buffer*, u32>>& BarrierData, 
					   u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<VkBufferMemoryBarrier> Barriers;
	for(const std::tuple<buffer*, u32>& Data : BarrierData)
	{
		u32 ResourceLayoutNext = std::get<1>(Data);

		vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(std::get<0>(Data));
		VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		Barrier.buffer = Buffer->Handle;
		Barrier.srcAccessMask = GetVKAccessMask(Buffer->CurrentLayout, DstStageMask);
		Barrier.dstAccessMask = GetVKAccessMask(ResourceLayoutNext, DstStageMask);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.offset = 0;
		Barrier.size = Buffer->Size;

		Buffer->PrevShader = DstStageMask;
		Buffer->CurrentLayout = ResourceLayoutNext;

		Barriers.push_back(Barrier);
	}

	vkCmdPipelineBarrier(*CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 0, 0, Barriers.size(), Barriers.data(), 0, 0);
}

void vulkan_global_pipeline_context::
SetImageBarriers(const std::vector<std::tuple<texture*, u32, barrier_state, u32>>& BarrierData, 
					  u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<VkImageMemoryBarrier> Barriers;
	for(const std::tuple<texture*, u32, barrier_state, u32>& Data : BarrierData)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(std::get<0>(Data));

		u32 ResourceLayoutNext = std::get<1>(Data);
		barrier_state ResourceStateNext = std::get<2>(Data);

		u32 MipIdx = std::get<3>(Data);
		VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		Barrier.dstAccessMask = GetVKAccessMask(ResourceLayoutNext, DstStageMask);
		Barrier.newLayout = GetVKLayout(ResourceStateNext);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.image = Texture->Handle;
		Barrier.subresourceRange.aspectMask = Texture->Aspect;
		if(MipIdx == ~0u)
		{
			Barrier.srcAccessMask = GetVKAccessMask(Texture->CurrentLayout[0], DstStageMask);
			Barrier.oldLayout = GetVKLayout(Texture->CurrentState[0]);

			Barrier.subresourceRange.baseMipLevel   = 0;
			Barrier.subresourceRange.baseArrayLayer = 0;
			Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), ResourceLayoutNext);
			std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), ResourceStateNext);
		}
		else
		{
			Barrier.srcAccessMask = GetVKAccessMask(Texture->CurrentLayout[MipIdx], DstStageMask);
			Barrier.oldLayout = GetVKLayout(Texture->CurrentState[MipIdx]);

			Barrier.subresourceRange.baseMipLevel   = MipIdx;
			Barrier.subresourceRange.baseArrayLayer = 0;
			Barrier.subresourceRange.levelCount = 1;
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			Texture->CurrentLayout[MipIdx] = ResourceLayoutNext;
			Texture->CurrentState[MipIdx] = ResourceStateNext;
		}

		Texture->PrevShader = DstStageMask;
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
SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, barrier_state, u32>>& BarrierData, 
					  u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<VkImageMemoryBarrier> Barriers;
	for(const std::tuple<std::vector<texture*>, u32, barrier_state, u32>& Data : BarrierData)
	{
		const std::vector<texture*>& Textures = std::get<0>(Data);
		if(!Textures.size()) continue;
		u32 MipIdx = std::get<3>(Data);
		for(texture* TextureData : Textures)
		{
			vulkan_texture* Texture = static_cast<vulkan_texture*>(TextureData);

			u32 ResourceLayoutNext = std::get<1>(Data);
			barrier_state ResourceStateNext = std::get<2>(Data);

			VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			Barrier.dstAccessMask = GetVKAccessMask(ResourceLayoutNext, DstStageMask);
			Barrier.newLayout = GetVKLayout(ResourceStateNext);
			Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.image = Texture->Handle;
			Barrier.subresourceRange.aspectMask = Texture->Aspect;
			if(MipIdx == ~0u)
			{
				Barrier.srcAccessMask = GetVKAccessMask(Texture->CurrentLayout[0], DstStageMask);
				Barrier.oldLayout = GetVKLayout(Texture->CurrentState[0]);

				Barrier.subresourceRange.baseMipLevel   = 0;
				Barrier.subresourceRange.baseArrayLayer = 0;
				Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

				std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), ResourceLayoutNext);
				std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), ResourceStateNext);
			}
			else
			{
				Barrier.srcAccessMask = GetVKAccessMask(Texture->CurrentLayout[MipIdx], DstStageMask);
				Barrier.oldLayout = GetVKLayout(Texture->CurrentState[MipIdx]);

				Barrier.subresourceRange.baseMipLevel   = MipIdx;
				Barrier.subresourceRange.baseArrayLayer = 0;
				Barrier.subresourceRange.levelCount = 1;
				Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

				Texture->CurrentLayout[MipIdx] = ResourceLayoutNext;
				Texture->CurrentState[MipIdx] = ResourceStateNext;
			}

			Texture->PrevShader = DstStageMask;
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

void vulkan_global_pipeline_context::
DebugGuiBegin(texture* RenderTarget)
{
	vulkan_texture* Clr = static_cast<vulkan_texture*>(RenderTarget);

	ImGui_ImplVulkan_NewFrame();

	VkRenderingInfoKHR RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	RenderingInfo.renderArea = {{}, {u32(Gfx->Width), u32(Gfx->Height)}};
	RenderingInfo.layerCount = 1;
	RenderingInfo.viewMask   = 0;

	VkRenderingAttachmentInfoKHR ColorInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR};
	ColorInfo.imageView = Clr->Views[0];
	ColorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	ColorInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	ColorInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorInfo.clearValue = {0, 0, 0, 0};
	RenderingInfo.colorAttachmentCount = 1;
	RenderingInfo.pColorAttachments = &ColorInfo;

	std::fill(Clr->CurrentLayout.begin(), Clr->CurrentLayout.end(), AF_ColorAttachmentWrite);
	std::fill(Clr->CurrentState.begin(), Clr->CurrentState.end(), barrier_state::color_attachment);
	Clr->PrevShader = PSF_ColorAttachment;

	if(Gfx->Features13.dynamicRendering)
		vkCmdBeginRenderingKHR(*CommandList, &RenderingInfo);
}

void vulkan_global_pipeline_context::
DebugGuiEnd()
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *CommandList);

	if(Gfx->Features13.dynamicRendering)
		vkCmdEndRenderingKHR(*CommandList);
}

vulkan_render_context::
vulkan_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::initializer_list<const std::string> ShaderList, 
					  const std::vector<texture*>& ColorTargets, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines)
	: LoadOp(NewLoadOp), StoreOp(NewStoreOp)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	NullTexture1D = static_cast<vulkan_texture*>(Gfx->NullTexture1D);
	NullTexture2D = static_cast<vulkan_texture*>(Gfx->NullTexture2D);
	NullTexture3D = static_cast<vulkan_texture*>(Gfx->NullTexture3D);
	NullTextureCube = static_cast<vulkan_texture*>(Gfx->NullTextureCube);
	Device = Gfx->Device;

	RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	RenderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	UseFramebuffer = !Gfx->Features13.dynamicRendering;

	u32 PushConstantStage = 0;
	std::map<VkDescriptorType, u32> DescriptorTypeCounts;
	std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> ShaderRootLayout;

	for(const std::string Shader : ShaderList)
	{
		VkPipelineShaderStageCreateInfo Stage = {};
		Stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Stage.pName  = "main";
		if(Shader.find(".vert.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::vertex, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_VERTEX_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_control, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_eval, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::geometry, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_GEOMETRY_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::fragment, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_FRAGMENT_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		ShaderStages.push_back(Stage);
	}

	std::vector<u32> DescriptorsSizes(ShaderRootLayout.size());
	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			auto Parameter = ShaderRootLayout[LayoutIdx][BindingIdx];
			DescriptorsSizes[LayoutIdx] += Parameter.descriptorCount;
			Parameters[LayoutIdx].push_back(Parameter);
		}
	}

	// TODO: Check if binding partially bound
	// TODO: Binding flags
	std::vector<VkDescriptorSetLayout> Layouts(ShaderRootLayout.size());
	Sets.resize(ShaderRootLayout.size(), VK_NULL_HANDLE);
	PushDescriptors.resize(ShaderRootLayout.size());
	for(u32 SpaceIdx = 0; SpaceIdx < ShaderRootLayout.size(); ++SpaceIdx)
	{
		PushDescriptors[SpaceIdx] = (DescriptorsSizes[SpaceIdx] < 32);
		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
		DescriptorSetLayoutCreateInfo.flags = PushDescriptors[SpaceIdx] * VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		DescriptorSetLayoutCreateInfo.bindingCount = Parameters[SpaceIdx].size();
		DescriptorSetLayoutCreateInfo.pBindings = Parameters[SpaceIdx].data();

		VkDescriptorSetLayout DescriptorSetLayout;
		VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
		Layouts[SpaceIdx] = DescriptorSetLayout;
	}


	std::vector<VkDescriptorPoolSize> PoolSizes;
	for (const auto& DescriptorType : DescriptorTypeCounts) {
		VkDescriptorPoolSize PoolSize = {};
		PoolSize.type = DescriptorType.first;
		PoolSize.descriptorCount = DescriptorType.second;
		PoolSizes.push_back(PoolSize);
	}

	VkDescriptorPoolCreateInfo PoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	PoolInfo.poolSizeCount = PoolSizes.size();
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = ShaderRootLayout.size();

	VK_CHECK(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &Pool));

	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		if(PushDescriptors[LayoutIdx]) continue;
		VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		AllocInfo.descriptorPool = Pool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &Layouts[LayoutIdx];

		VkDescriptorSet DescriptorSet;
		VK_CHECK(vkAllocateDescriptorSets(Device, &AllocInfo, &DescriptorSet));
		Sets[LayoutIdx] = DescriptorSet;
	}

	VkPipelineLayoutCreateInfo RootSignatureCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	RootSignatureCreateInfo.pSetLayouts = Layouts.data();
	RootSignatureCreateInfo.setLayoutCount = Layouts.size();

	if(HavePushConstant)
	{
		ConstantRange.stageFlags = PushConstantStage;
		ConstantRange.size       = PushConstantSize;
		RootSignatureCreateInfo.pushConstantRangeCount = 1;
		RootSignatureCreateInfo.pPushConstantRanges    = &ConstantRange;
	}

	VK_CHECK(vkCreatePipelineLayout(Device, &RootSignatureCreateInfo, nullptr, &RootSignatureHandle));

	VkGraphicsPipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

	CreateInfo.layout = RootSignatureHandle;
	CreateInfo.pStages = ShaderStages.data();
	CreateInfo.stageCount = ShaderStages.size();

	std::vector<VkFormat> ColorTargetFormats;
	std::vector<VkAttachmentReference> AttachmentReferences;
	std::vector<VkAttachmentDescription> AttachmentDescriptions;
	for(u32 FormatIdx = 0; FormatIdx < ColorTargets.size(); ++FormatIdx)
	{
		VkAttachmentReference AttachRef = {};
		VkAttachmentDescription AttachDesc = {};
		AttachDesc.format        = GetVKFormat(ColorTargets[FormatIdx]->Info.Format);
		AttachDesc.samples       = VK_SAMPLE_COUNT_1_BIT;
		AttachDesc.loadOp        = GetVKLoadOp(NewLoadOp);
		AttachDesc.storeOp       = GetVKStoreOp(NewStoreOp);
		AttachDesc.initialLayout = AttachDesc.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		AttachDesc.finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		AttachRef.attachment = FormatIdx;
		AttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		ColorTargetFormats.push_back(AttachDesc.format);
		AttachmentDescriptions.push_back(AttachDesc);
		AttachmentReferences.push_back(AttachRef);
	}

	VkAttachmentReference DepthAttachRef = {};
	DepthAttachRef.attachment = ColorTargets.size();
	DepthAttachRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::vector<VkSubpassDependency> Dependencies;
#if 0
	VkSubpassDependency ColorDependency = {};
	ColorDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	ColorDependency.dstSubpass    = 0;
	ColorDependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	ColorDependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	ColorDependency.srcAccessMask = 0;
	ColorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	if(InputData.UseColor)
		Dependencies.push_back(ColorDependency);

	VkSubpassDependency DepthDependency = {};
	DepthDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	DepthDependency.dstSubpass    = 0;
	DepthDependency.srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	DepthDependency.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	DepthDependency.srcAccessMask = 0;
	DepthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	if(InputData.UseDepth)
		Dependencies.push_back(DepthDependency);
#endif

	VkAttachmentDescription DepthAttachDesc = {};
	DepthAttachDesc.format        = InputData.UseDepth ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_UNDEFINED;
	DepthAttachDesc.samples       = VK_SAMPLE_COUNT_1_BIT;
	DepthAttachDesc.loadOp        = GetVKLoadOp(NewLoadOp);
	DepthAttachDesc.storeOp       = GetVKStoreOp(NewStoreOp);
	DepthAttachDesc.initialLayout = DepthAttachDesc.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	DepthAttachDesc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	if(InputData.UseDepth)
		AttachmentDescriptions.push_back(DepthAttachDesc);

	VkRenderPassMultiviewCreateInfo MultiviewCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO};
	MultiviewCreateInfo.subpassCount         = 1;
	MultiviewCreateInfo.dependencyCount      = Dependencies.size();
	MultiviewCreateInfo.pViewMasks           = &InputData.ViewMask;
	MultiviewCreateInfo.correlationMaskCount = 0;
	MultiviewCreateInfo.pCorrelationMasks    = nullptr;

	Subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount    = InputData.UseColor ? AttachmentReferences.size() : 0;
	Subpass.pColorAttachments       = InputData.UseColor ? AttachmentReferences.data() : nullptr;
	Subpass.pDepthStencilAttachment = InputData.UseDepth ? &DepthAttachRef : nullptr;

	VkRenderPassCreateInfo RenderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	RenderPassCreateInfo.pNext           = InputData.UseMultiview ? &MultiviewCreateInfo : nullptr;
	RenderPassCreateInfo.attachmentCount = AttachmentDescriptions.size();
	RenderPassCreateInfo.pAttachments    = AttachmentDescriptions.data();
	RenderPassCreateInfo.subpassCount    = 1;
	RenderPassCreateInfo.pSubpasses      = &Subpass;
	RenderPassCreateInfo.dependencyCount = Dependencies.size();
	RenderPassCreateInfo.pDependencies   = Dependencies.data();
	VK_CHECK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &RenderPass));

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
	DepthStencilState.depthTestEnable  = InputData.UseDepth;
	DepthStencilState.depthWriteEnable = true;
	DepthStencilState.minDepthBounds = 0.0f;
	DepthStencilState.maxDepthBounds = 1.0f;
	DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	ViewportState.viewportCount = 1; //int(InputData.UseColor || InputData.UseDepth);
	ViewportState.scissorCount  = 1; //int(InputData.UseColor || InputData.UseDepth);

	VkPipelineRasterizationConservativeStateCreateInfoEXT ConservativeRasterState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT};
	ConservativeRasterState.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
	ConservativeRasterState.extraPrimitiveOverestimationSize = Gfx->ConservativeRasterProps.maxExtraPrimitiveOverestimationSize;

	VkPipelineRasterizationStateCreateInfo RasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	RasterizationState.lineWidth = 1.0f;
	RasterizationState.cullMode  = GetVKCullMode(InputData.CullMode);
	RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	RasterizationState.pNext = InputData.UseConservativeRaster ? &ConservativeRasterState : nullptr;

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
	if(Gfx->Features13.dynamicRendering)
		CreateInfo.pNext = &PipelineRenderingCreateInfo;
	else
		CreateInfo.renderPass = RenderPass;

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
	FramebufferCreateInfo.renderPass = RenderPass;
	FramebufferCreateInfo.width  = RenderWidth;
	FramebufferCreateInfo.height = RenderHeight;
	FramebufferCreateInfo.layers = 1; //EnableMultiview * (1 << Face);
	RenderPassInfo.renderPass = RenderPass;
	RenderPassInfo.renderArea.extent.width = RenderWidth;
	RenderPassInfo.renderArea.extent.height = RenderHeight;

	std::vector<VkDescriptorSet> SetsToBind;
	for(const VkDescriptorSet& DescriptorSet : Sets)
	{
		if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
	}

	vkCmdBindPipeline(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
	if(SetsToBind.size())
		vkCmdBindDescriptorSets(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, RootSignatureHandle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);

	VkViewport Viewport = {0, (float)RenderHeight, (float)RenderWidth, -(float)RenderHeight, 0, 1};
	VkRect2D Scissor = {{0, 0}, {RenderWidth, RenderHeight}};
	vkCmdSetViewport(*PipelineContext->CommandList, 0, 1, &Viewport);
	vkCmdSetScissor(*PipelineContext->CommandList, 0, 1, &Scissor);
}

void vulkan_render_context::
End()
{
	SetIndices.clear();
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	PushDescriptorBindings.clear();
}

void vulkan_render_context::
Clear()
{
	FrameBufferIdx = 0;
	AttachmentViews.clear();
	RenderTargetClears.clear();
	StaticDescriptorBindings.clear();
	std::vector<std::unique_ptr<descriptor_info>>().swap(BufferInfos);
	std::vector<std::unique_ptr<descriptor_info[]>>().swap(BufferArrayInfos);
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR>>().swap(RenderingAttachmentInfos);
	std::vector<std::unique_ptr<VkRenderingAttachmentInfoKHR[]>>().swap(RenderingAttachmentInfoArrays);
}

void vulkan_render_context::
SetColorTarget(u32 RenderWidth, u32 RenderHeight, const std::vector<texture*>& ColorAttachments, vec4 Clear, u32 Face, bool EnableMultiview)
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
		ColorInfo[AttachmentIdx].imageLayout = GetVKLayout(barrier_state::color_attachment);
		ColorInfo[AttachmentIdx].loadOp = GetVKLoadOp(LoadOp);
		ColorInfo[AttachmentIdx].storeOp = GetVKStoreOp(StoreOp);
		memcpy(ColorInfo[AttachmentIdx].clearValue.color.float32, Clear.E, 4 * sizeof(float));

		AttachmentViews.push_back(Attachment->Views[0]);
		RenderTargetClears.push_back(ColorInfo[AttachmentIdx].clearValue);
	}
	RenderingInfo.colorAttachmentCount = ColorAttachments.size();
	RenderingInfo.pColorAttachments = ColorInfo.get();

	RenderingAttachmentInfoArrays.push_back(std::move(ColorInfo));
}

void vulkan_render_context::
SetDepthTarget(u32 RenderWidth, u32 RenderHeight, texture* DepthAttachment, vec2 Clear, u32 Face, bool EnableMultiview)
{
	vulkan_texture* Attachment = static_cast<vulkan_texture*>(DepthAttachment);

	RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
	RenderingInfo.layerCount = 1;
	RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

	std::unique_ptr<VkRenderingAttachmentInfoKHR> DepthInfo = std::make_unique<VkRenderingAttachmentInfoKHR>();
	DepthInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	DepthInfo->imageView = Attachment->Views[0];
	DepthInfo->imageLayout = GetVKLayout(barrier_state::depth_stencil_attachment);
	DepthInfo->loadOp = GetVKLoadOp(LoadOp);
	DepthInfo->storeOp = GetVKStoreOp(StoreOp);
	DepthInfo->clearValue.depthStencil.depth   = Clear.E[0];
	DepthInfo->clearValue.depthStencil.stencil = Clear.E[1];
	RenderingInfo.pDepthAttachment = DepthInfo.get();

	AttachmentViews.push_back(Attachment->Views[0]);
	RenderTargetClears.push_back(DepthInfo->clearValue);
	RenderingAttachmentInfos.push_back(std::move(DepthInfo));
}

void vulkan_render_context::
SetStencilTarget(u32 RenderWidth, u32 RenderHeight, texture* StencilAttachment, vec2 Clear, u32 Face, bool EnableMultiview)
{
	vulkan_texture* Attachment = static_cast<vulkan_texture*>(StencilAttachment);

	RenderingInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
	RenderingInfo.layerCount = 1;
	RenderingInfo.viewMask   = EnableMultiview * (1 << Face);

	std::unique_ptr<VkRenderingAttachmentInfoKHR> StencilInfo = std::make_unique<VkRenderingAttachmentInfoKHR>();
	StencilInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	StencilInfo->imageView = Attachment->Views[0];
	StencilInfo->imageLayout = GetVKLayout(barrier_state::depth_stencil_attachment);
	StencilInfo->loadOp = GetVKLoadOp(LoadOp);
	StencilInfo->storeOp = GetVKStoreOp(StoreOp);
	StencilInfo->clearValue.depthStencil.depth   = Clear.E[0];
	StencilInfo->clearValue.depthStencil.stencil = Clear.E[1];
	RenderingInfo.pStencilAttachment = StencilInfo.get();

	AttachmentViews.push_back(Attachment->Views[0]);
	RenderTargetClears.push_back(StencilInfo->clearValue);
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

	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, RootSignatureHandle, 0, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	if(UseFramebuffer)
	{
		FramebufferCreateInfo.attachmentCount = AttachmentViews.size();
		FramebufferCreateInfo.pAttachments    = AttachmentViews.data();
		RenderPassInfo.clearValueCount        = RenderTargetClears.size();
		RenderPassInfo.pClearValues           = RenderTargetClears.data();

		VkFramebuffer& FrameBuffer = FrameBuffers[FrameBufferIdx];
		if(!FrameBuffer)
			vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &FrameBuffer);
		FrameBufferIdx++;

		RenderPassInfo.framebuffer = FrameBuffer;
		vkCmdBeginRenderPass(*PipelineContext->CommandList, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdDraw(*PipelineContext->CommandList, VertexCount, 1, FirstVertex, 0);
		vkCmdEndRenderPass(*PipelineContext->CommandList);
	}
	else
	{
		vkCmdBeginRenderingKHR(*PipelineContext->CommandList, &RenderingInfo);
		vkCmdDraw(*PipelineContext->CommandList, VertexCount, 1, FirstVertex, 0);
		vkCmdEndRenderingKHR(*PipelineContext->CommandList);
	}
}

void vulkan_render_context::
DrawIndexed(buffer* IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount)
{
	vulkan_buffer* IndexAttachment = static_cast<vulkan_buffer*>(IndexBuffer);

	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, RootSignatureHandle, 0, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	if(UseFramebuffer)
	{
		FramebufferCreateInfo.attachmentCount = AttachmentViews.size();
		FramebufferCreateInfo.pAttachments    = AttachmentViews.data();
		RenderPassInfo.clearValueCount        = RenderTargetClears.size();
		RenderPassInfo.pClearValues           = RenderTargetClears.data();

		VkFramebuffer& FrameBuffer = FrameBuffers[FrameBufferIdx];
		if(!FrameBuffer)
			vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &FrameBuffer);
		FrameBufferIdx++;

		RenderPassInfo.framebuffer = FrameBuffer;
		vkCmdBeginRenderPass(*PipelineContext->CommandList, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindIndexBuffer(*PipelineContext->CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(*PipelineContext->CommandList, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
		vkCmdEndRenderPass(*PipelineContext->CommandList);
	}
	else
	{
		vkCmdBeginRenderingKHR(*PipelineContext->CommandList, &RenderingInfo);
		vkCmdBindIndexBuffer(*PipelineContext->CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(*PipelineContext->CommandList, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
		vkCmdEndRenderingKHR(*PipelineContext->CommandList);
	}
}

void vulkan_render_context::
DrawIndirect(u32 ObjectDrawCount, buffer* IndexBuffer, buffer* IndirectCommands, u32 CommandStructureSize)
{
	vulkan_buffer* IndexAttachment = static_cast<vulkan_buffer*>(IndexBuffer);
	vulkan_buffer* IndirectCommandsAttachment = static_cast<vulkan_buffer*>(IndirectCommands);

	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, RootSignatureHandle, 0, PushDescriptorBindings.size(), PushDescriptorBindings.data());
	if(UseFramebuffer)
	{
		FramebufferCreateInfo.attachmentCount = AttachmentViews.size();
		FramebufferCreateInfo.pAttachments    = AttachmentViews.data();
		RenderPassInfo.clearValueCount        = RenderTargetClears.size();
		RenderPassInfo.pClearValues           = RenderTargetClears.data();

		VkFramebuffer& FrameBuffer = FrameBuffers[FrameBufferIdx];
		if(!FrameBuffer)
			vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &FrameBuffer);
		FrameBufferIdx++;

		RenderPassInfo.framebuffer = FrameBuffer;
		vkCmdBeginRenderPass(*PipelineContext->CommandList, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindIndexBuffer(*PipelineContext->CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexedIndirectCount(*PipelineContext->CommandList, IndirectCommandsAttachment->Handle, 4, IndirectCommandsAttachment->Handle, IndirectCommandsAttachment->CounterOffset, ObjectDrawCount, CommandStructureSize);
		vkCmdEndRenderPass(*PipelineContext->CommandList);
	}
	else
	{
		vkCmdBeginRenderingKHR(*PipelineContext->CommandList, &RenderingInfo);
		vkCmdBindIndexBuffer(*PipelineContext->CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexedIndirectCount(*PipelineContext->CommandList, IndirectCommandsAttachment->Handle, 4, IndirectCommandsAttachment->Handle, IndirectCommandsAttachment->CounterOffset, ObjectDrawCount, CommandStructureSize);
		vkCmdEndRenderingKHR(*PipelineContext->CommandList);
	}

	PipelineContext->TexturesToCommon.insert(TexturesToCommon.begin(), TexturesToCommon.end());
	PipelineContext->BuffersToCommon.insert(BuffersToCommon.begin(), BuffersToCommon.end());
}

void vulkan_render_context::
SetConstant(void* Data, size_t Size)
{
	vkCmdPushConstants(*PipelineContext->CommandList, RootSignatureHandle, ConstantRange.stageFlags, 0, Size, Data);
}

// NOTE: If with counter, then it is using 2 bindings instead of 1
void vulkan_render_context::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);

	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

	VkWriteDescriptorSet CounterDescriptorSet = {};
	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = 1;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
	if(PushDescriptors[Set])
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
		if(!PushDescriptors[Set])
			CounterDescriptorSet.dstSet = Sets[Set];
		CounterDescriptorSet.dstBinding = SetIndices[Set];
		CounterDescriptorSet.descriptorCount = 1;
		CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
		if(PushDescriptors[Set])
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
	BuffersToCommon.insert(Buffer);

	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

	VkWriteDescriptorSet CounterDescriptorSet = {};
	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = 1;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
	if(PushDescriptors[Set])
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
		if(!PushDescriptors[Set])
			CounterDescriptorSet.dstSet = Sets[Set];
		CounterDescriptorSet.dstBinding = SetIndices[Set];
		CounterDescriptorSet.descriptorCount = 1;
		CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
		if(PushDescriptors[Set])
			PushDescriptorBindings.push_back(CounterDescriptorSet);
		else
			StaticDescriptorBindings.push_back(CounterDescriptorSet);
		SetIndices[Set] += 1;

		BufferInfos.push_back(std::move(CounterBufferInfo));
	}
}

// TODO: Remove image layouts and move them inside texture structure
// TODO: Need to know image type ahead of time so that I wouldn't have image_type input parameter
void vulkan_render_context::
SetSampledImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 DescriptorCount = Parameters[Set][SetIndices[Set]].descriptorCount;
	std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), DescriptorCount));
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		TexturesToCommon.insert(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	vulkan_texture* NullData = nullptr;
	if(Type == image_type::Texture1D)
		NullData = static_cast<vulkan_texture*>(NullTexture1D);
	else if(Type == image_type::Texture2D)
		NullData = static_cast<vulkan_texture*>(NullTexture2D);
	else if(Type == image_type::Texture3D)
		NullData = static_cast<vulkan_texture*>(NullTexture3D);
	else if(Type == image_type::TextureCube)
		NullData = static_cast<vulkan_texture*>(NullTextureCube);
	for(u32 TextureIdx = Textures.size(); TextureIdx < DescriptorCount; TextureIdx++)
	{
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = NullData->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = NullData->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSet.descriptorCount = DescriptorCount;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	BufferArrayInfos.push_back(std::move(ImageInfo));
}

void vulkan_render_context::
SetStorageImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 DescriptorCount = Parameters[Set][SetIndices[Set]].descriptorCount;
	std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), DescriptorCount));
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		TexturesToCommon.insert(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	vulkan_texture* NullData = nullptr;
	if(Type == image_type::Texture1D)
		NullData = static_cast<vulkan_texture*>(NullTexture1D);
	else if(Type == image_type::Texture2D)
		NullData = static_cast<vulkan_texture*>(NullTexture2D);
	else if(Type == image_type::Texture3D)
		NullData = static_cast<vulkan_texture*>(NullTexture3D);
	else if(Type == image_type::TextureCube)
		NullData = static_cast<vulkan_texture*>(NullTextureCube);
	for(u32 TextureIdx = Textures.size(); TextureIdx < DescriptorCount; TextureIdx++)
	{
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = NullData->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = NullData->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = DescriptorCount;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	BufferArrayInfos.push_back(std::move(ImageInfo));
}

void vulkan_render_context::
SetImageSampler(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 DescriptorCount = Parameters[Set][SetIndices[Set]].descriptorCount;
	std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), DescriptorCount));
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		TexturesToCommon.insert(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	vulkan_texture* NullData = nullptr;
	if(Type == image_type::Texture1D)
		NullData = static_cast<vulkan_texture*>(NullTexture1D);
	else if(Type == image_type::Texture2D)
		NullData = static_cast<vulkan_texture*>(NullTexture2D);
	else if(Type == image_type::Texture3D)
		NullData = static_cast<vulkan_texture*>(NullTexture3D);
	else if(Type == image_type::TextureCube)
		NullData = static_cast<vulkan_texture*>(NullTextureCube);
	for(u32 TextureIdx = Textures.size(); TextureIdx < DescriptorCount; TextureIdx++)
	{
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = NullData->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = NullData->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = DescriptorCount;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	BufferArrayInfos.push_back(std::move(ImageInfo));
}

vulkan_compute_context::
vulkan_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	NullTexture1D = static_cast<vulkan_texture*>(Gfx->NullTexture1D);
	NullTexture2D = static_cast<vulkan_texture*>(Gfx->NullTexture2D);
	NullTexture3D = static_cast<vulkan_texture*>(Gfx->NullTexture3D);
	NullTextureCube = static_cast<vulkan_texture*>(Gfx->NullTextureCube);
	Device = Gfx->Device;

	std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> ShaderRootLayout;
	std::map<VkDescriptorType, u32> DescriptorTypeCounts;

	ComputeStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	ComputeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	ComputeStage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::compute, ShaderRootLayout, DescriptorTypeCounts, HavePushConstant, PushConstantSize, ShaderDefines, &BlockSizeX, &BlockSizeY, &BlockSizeZ);
	ComputeStage.pName  = "main";

	std::vector<u32> DescriptorsSizes(ShaderRootLayout.size());
	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			auto Parameter = ShaderRootLayout[LayoutIdx][BindingIdx];
			DescriptorsSizes[LayoutIdx] += Parameter.descriptorCount;
			Parameters[LayoutIdx].push_back(Parameter);
		}
	}

	// TODO: Check if binding partially bound
	std::vector<VkDescriptorSetLayout> Layouts(ShaderRootLayout.size());
	Sets.resize(ShaderRootLayout.size(), VK_NULL_HANDLE);
	PushDescriptors.resize(ShaderRootLayout.size());
	for(u32 SpaceIdx = 0; SpaceIdx < ShaderRootLayout.size(); ++SpaceIdx)
	{
		PushDescriptors[SpaceIdx] = (DescriptorsSizes[SpaceIdx] < 32);
		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
		DescriptorSetLayoutCreateInfo.flags = PushDescriptors[SpaceIdx] * VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		DescriptorSetLayoutCreateInfo.bindingCount = Parameters[SpaceIdx].size();
		DescriptorSetLayoutCreateInfo.pBindings = Parameters[SpaceIdx].data();

		VkDescriptorSetLayout DescriptorSetLayout;
		VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
		Layouts[SpaceIdx] = DescriptorSetLayout;
	}


	std::vector<VkDescriptorPoolSize> PoolSizes;
	for (const auto& DescriptorType : DescriptorTypeCounts) {
		VkDescriptorPoolSize PoolSize = {};
		PoolSize.type = DescriptorType.first;
		PoolSize.descriptorCount = DescriptorType.second;
		PoolSizes.push_back(PoolSize);
	}

	VkDescriptorPoolCreateInfo PoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	PoolInfo.poolSizeCount = PoolSizes.size();
	PoolInfo.pPoolSizes = PoolSizes.data();
	PoolInfo.maxSets = ShaderRootLayout.size();

	VK_CHECK(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &Pool));

	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		if(PushDescriptors[LayoutIdx]) continue;
		VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		AllocInfo.descriptorPool = Pool;
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &Layouts[LayoutIdx];

		VkDescriptorSet DescriptorSet;
		VK_CHECK(vkAllocateDescriptorSets(Device, &AllocInfo, &DescriptorSet));
		Sets[LayoutIdx] = DescriptorSet;
	}

	VkPipelineLayoutCreateInfo RootSignatureCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	RootSignatureCreateInfo.pSetLayouts = Layouts.data();
	RootSignatureCreateInfo.setLayoutCount = Layouts.size();

	if(HavePushConstant)
	{
		ConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		ConstantRange.size       = PushConstantSize;
		RootSignatureCreateInfo.pushConstantRangeCount = 1;
		RootSignatureCreateInfo.pPushConstantRanges    = &ConstantRange;
	}

	VK_CHECK(vkCreatePipelineLayout(Device, &RootSignatureCreateInfo, nullptr, &RootSignatureHandle));

	VkComputePipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	CreateInfo.stage  = ComputeStage;
	CreateInfo.layout = RootSignatureHandle;

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
	for(const VkDescriptorSet& DescriptorSet : Sets)
	{
		if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
	}

	vkCmdBindPipeline(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
	if(SetsToBind.size())
		vkCmdBindDescriptorSets(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, RootSignatureHandle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);
}

void vulkan_compute_context::
End()
{
	SetIndices.clear();
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	PushDescriptorBindings.clear();
}

void vulkan_compute_context::
Clear()
{
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
	vkCmdPushDescriptorSetKHR(*PipelineContext->CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, RootSignatureHandle, 0, PushDescriptorBindings.size(), PushDescriptorBindings.data());

	vkCmdDispatch(*PipelineContext->CommandList, (X + BlockSizeX - 1) / BlockSizeX, (Y + BlockSizeY - 1) / BlockSizeY, (Z + BlockSizeZ - 1) / BlockSizeZ);

	PipelineContext->TexturesToCommon.insert(TexturesToCommon.begin(), TexturesToCommon.end());
	PipelineContext->BuffersToCommon.insert(BuffersToCommon.begin(), BuffersToCommon.end());
}

void vulkan_compute_context::
SetConstant(void* Data, size_t Size)
{
	vkCmdPushConstants(*PipelineContext->CommandList, RootSignatureHandle, ConstantRange.stageFlags, 0, Size, Data);
}

void vulkan_compute_context::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);

	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

	VkWriteDescriptorSet CounterDescriptorSet = {};
	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = 1;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
	if(PushDescriptors[Set])
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
		if(!PushDescriptors[Set])
			CounterDescriptorSet.dstSet = Sets[Set];
		CounterDescriptorSet.dstBinding = SetIndices[Set];
		CounterDescriptorSet.descriptorCount = 1;
		CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
		if(PushDescriptors[Set])
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
	BuffersToCommon.insert(Buffer);

	std::unique_ptr<descriptor_info> CounterBufferInfo = std::make_unique<descriptor_info>();
	std::unique_ptr<descriptor_info> BufferInfo = std::make_unique<descriptor_info>();
	BufferInfo->buffer = Attachment->Handle;
	BufferInfo->offset = 0;
	BufferInfo->range  = Attachment->WithCounter ? Attachment->CounterOffset : Attachment->Size;

	VkWriteDescriptorSet CounterDescriptorSet = {};
	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = 1;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo.get());
	if(PushDescriptors[Set])
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
		if(!PushDescriptors[Set])
			CounterDescriptorSet.dstSet = Sets[Set];
		CounterDescriptorSet.dstBinding = SetIndices[Set];
		CounterDescriptorSet.descriptorCount = 1;
		CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo.get());
		if(PushDescriptors[Set])
			PushDescriptorBindings.push_back(CounterDescriptorSet);
		else
			StaticDescriptorBindings.push_back(CounterDescriptorSet);
		SetIndices[Set] += 1;

		BufferInfos.push_back(std::move(CounterBufferInfo));
	}
}

// TODO: Remove image layouts and move them inside texture structure
// TODO: Need to know image type ahead of time so that I wouldn't have image_type input parameter
void vulkan_compute_context::
SetSampledImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 DescriptorCount = Parameters[Set][SetIndices[Set]].descriptorCount;
	std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), DescriptorCount));
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		TexturesToCommon.insert(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	vulkan_texture* NullData = nullptr;
	if(Type == image_type::Texture1D)
		NullData = static_cast<vulkan_texture*>(NullTexture1D);
	else if(Type == image_type::Texture2D)
		NullData = static_cast<vulkan_texture*>(NullTexture2D);
	else if(Type == image_type::Texture3D)
		NullData = static_cast<vulkan_texture*>(NullTexture3D);
	else if(Type == image_type::TextureCube)
		NullData = static_cast<vulkan_texture*>(NullTextureCube);
	for(u32 TextureIdx = Textures.size(); TextureIdx < DescriptorCount; TextureIdx++)
	{
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = NullData->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = NullData->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = DescriptorCount;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	BufferArrayInfos.push_back(std::move(ImageInfo));
}

void vulkan_compute_context::
SetStorageImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 DescriptorCount = Parameters[Set][SetIndices[Set]].descriptorCount;
	std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), DescriptorCount));
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		TexturesToCommon.insert(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	vulkan_texture* NullData = nullptr;
	if(Type == image_type::Texture1D)
		NullData = static_cast<vulkan_texture*>(NullTexture1D);
	else if(Type == image_type::Texture2D)
		NullData = static_cast<vulkan_texture*>(NullTexture2D);
	else if(Type == image_type::Texture3D)
		NullData = static_cast<vulkan_texture*>(NullTexture3D);
	else if(Type == image_type::TextureCube)
		NullData = static_cast<vulkan_texture*>(NullTextureCube);
	for(u32 TextureIdx = Textures.size(); TextureIdx < DescriptorCount; TextureIdx++)
	{
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = NullData->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = NullData->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = DescriptorCount;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	BufferArrayInfos.push_back(std::move(ImageInfo));
}

void vulkan_compute_context::
SetImageSampler(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 DescriptorCount = Parameters[Set][SetIndices[Set]].descriptorCount;
	std::unique_ptr<descriptor_info[]> ImageInfo((descriptor_info*)calloc(sizeof(descriptor_info), DescriptorCount));
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		TexturesToCommon.insert(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	vulkan_texture* NullData = nullptr;
	if(Type == image_type::Texture1D)
		NullData = static_cast<vulkan_texture*>(NullTexture1D);
	else if(Type == image_type::Texture2D)
		NullData = static_cast<vulkan_texture*>(NullTexture2D);
	else if(Type == image_type::Texture3D)
		NullData = static_cast<vulkan_texture*>(NullTexture3D);
	else if(Type == image_type::TextureCube)
		NullData = static_cast<vulkan_texture*>(NullTextureCube);
	for(u32 TextureIdx = Textures.size(); TextureIdx < DescriptorCount; TextureIdx++)
	{
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = NullData->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = NullData->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = DescriptorCount;
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo.get());
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	BufferArrayInfos.push_back(std::move(ImageInfo));
}
