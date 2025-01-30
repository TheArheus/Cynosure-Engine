
#define USE_BOTTOM_OF_PIPE_BARRIERS 0

void vulkan_resource_binder::
AppendStaticStorage(general_context* ContextToUse, const array<binding_packet>& Data, u32 Offset)
{
	if(!Data.size()) return;
	SetIndices.clear();

	for(u32 LayoutIdx = 1; LayoutIdx < ContextToUse->ParameterLayout.size(); ++LayoutIdx)
	{
		for(u32 ParamIdx = 0; ParamIdx < ContextToUse->ParameterLayout[LayoutIdx].size(); ++ParamIdx, ++Offset)
		{
			descriptor_param Parameter = ContextToUse->ParameterLayout[LayoutIdx][ParamIdx];
			if(Parameter.Type == resource_type::buffer_storage || Parameter.Type == resource_type::buffer_uniform)
			{
				assert(false && "Buffer in static storage. Currently is not available, use buffers in the inputs");
			}
			else if(Parameter.Type == resource_type::texture_sampler)
			{
				SetImageSampler(Parameter.Count, Data[Offset].Resources, Parameter.ImageType, Parameter.BarrierState, Data[Offset].SubresourceIndex, LayoutIdx);
			}
			else if(Parameter.Type == resource_type::texture_storage)
			{
				assert(false && "Storage image in static storage. Check the shader bindings. Could be image sampler or buffer");
			}
		}
	}
}

void vulkan_resource_binder::
BindStaticStorage(renderer_backend* GeneralBackend)
{
	vulkan_backend* Backend = static_cast<vulkan_backend*>(GeneralBackend);
	vkUpdateDescriptorSets(Backend->Device, StaticDescriptorBindings.size(), StaticDescriptorBindings.data(), 0, nullptr);
}

void vulkan_resource_binder::
SetBufferView(resource* Buffer, u32 Set)
{
	vulkan_buffer* Attachment = static_cast<vulkan_buffer*>(Buffer);

	descriptor_info* BufferInfo = PushStruct(descriptor_info);
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
	DescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(BufferInfo);
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;

	if(Attachment->WithCounter)
	{
		descriptor_info* CounterBufferInfo = PushStruct(descriptor_info);
		CounterBufferInfo->buffer = Attachment->Handle;
		CounterBufferInfo->offset = Attachment->CounterOffset;
		CounterBufferInfo->range  = sizeof(u32);

		CounterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		if(!PushDescriptors[Set])
			CounterDescriptorSet.dstSet = Sets[Set];
		CounterDescriptorSet.dstBinding = SetIndices[Set];
		CounterDescriptorSet.descriptorCount = 1;
		CounterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		CounterDescriptorSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(CounterBufferInfo);
		if(PushDescriptors[Set])
			PushDescriptorBindings.push_back(CounterDescriptorSet);
		else
			StaticDescriptorBindings.push_back(CounterDescriptorSet);
		SetIndices[Set] += 1;
	}
}

void vulkan_resource_binder::
SetSampledImage(u32 DescriptorCount, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	if(!Textures.size())
	{
		SetIndices[Set] += 1;
		return;
	}

	descriptor_info* ImageInfo = PushArray(descriptor_info, Textures.size());
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = Textures.size();
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo);
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;
}

void vulkan_resource_binder::
SetStorageImage(u32 DescriptorCount, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	if(!Textures.size())
	{
		SetIndices[Set] += 1;
		return;
	}

	descriptor_info* ImageInfo = PushArray(descriptor_info, Textures.size());
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = Textures.size();
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo);
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;
}

void vulkan_resource_binder::
SetImageSampler(u32 DescriptorCount, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	if(!Textures.size())
	{
		SetIndices[Set] += 1;
		return;
	}

	descriptor_info* ImageInfo = PushArray(descriptor_info, Textures.size());
	for(u32 TextureIdx = 0; TextureIdx < Textures.size(); TextureIdx++)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Textures[TextureIdx]);
		ImageInfo[TextureIdx].imageLayout = GetVKLayout(State);
		ImageInfo[TextureIdx].imageView = Texture->Views[ViewIdx];
		ImageInfo[TextureIdx].sampler = Texture->SamplerHandle;
	}

	VkWriteDescriptorSet DescriptorSet = {};
	DescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	if(!PushDescriptors[Set])
		DescriptorSet.dstSet = Sets[Set];
	DescriptorSet.dstBinding = SetIndices[Set];
	DescriptorSet.descriptorCount = Textures.size();
	DescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	DescriptorSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(ImageInfo);
	if(PushDescriptors[Set])
		PushDescriptorBindings.push_back(DescriptorSet);
	else
		StaticDescriptorBindings.push_back(DescriptorSet);
	SetIndices[Set] += 1;
}


void vulkan_command_list::
CreateResource(renderer_backend* Backend)
{
	Gfx = static_cast<vulkan_backend*>(Backend);
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	Device = CommandQueue->Device;

	VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &AcquireSemaphore));
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ReleaseSemaphore));

	VkFenceCreateInfo FenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK(vkCreateFence(Device, &FenceCreateInfo, nullptr, &RenderFence));

	CommandList = CommandQueue->AllocateCommandList();
}

void vulkan_command_list::
DestroyObject()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	vkWaitForFences(Device, 1, &RenderFence, VK_TRUE, UINT64_MAX);
	vkResetFences(Device, 1, &RenderFence);

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
	if(RenderFence)
	{
		vkDestroyFence(Device, RenderFence, nullptr);
		RenderFence = 0;
	}
	if(CommandList)
	{
		CommandQueue->Remove(&CommandList);
		CommandList = nullptr;
	}
}

void vulkan_command_list::
AcquireNextImage()
{
	vkWaitForFences(Device, 1, &RenderFence, VK_TRUE, UINT64_MAX);
	vkResetFences(Device, 1, &RenderFence);
	BackBufferIndex = Gfx->GetCurrentBackBufferIndex(this);
}

void vulkan_command_list::
Begin()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->Reset(&CommandList);
}

void vulkan_command_list::
DeviceWaitIdle()
{
	vkDeviceWaitIdle(Device);
}

void vulkan_command_list::
PlaceEndOfFrameBarriers()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	std::vector<VkImageMemoryBarrier> ImageEndRenderBarriers = 
	{
		CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], GetVKAccessMask(AF_TransferWrite, PSF_Transfer), 0, GetVKLayout(barrier_state::transfer_dst), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	};

	std::vector<buffer_barrier> AttachmentBufferBarriers;
	std::vector<texture_barrier> AttachmentImageBarriers;
	for(u32 Idx = 0; Idx < BuffersToCommon.size(); ++Idx)
	{
#if USE_BOTTOM_OF_PIPE_BARRIERS
		buffer* Resource = *std::next(BuffersToCommon.begin(), Idx);
		AttachmentBufferBarriers.push_back({Resource, AF_ShaderRead, PSF_BottomOfPipe});
#else
		vulkan_buffer* Resource = static_cast<vulkan_buffer*>(*std::next(BuffersToCommon.begin(), Idx));
		Resource->CurrentLayout = 0;
		Resource->PrevShader = PSF_BottomOfPipe;
#endif
	}

	for(u32 Idx = 0; Idx < TexturesToCommon.size(); ++Idx)
	{
#if USE_BOTTOM_OF_PIPE_BARRIERS
		texture* Resource = *std::next(TexturesToCommon.begin(), Idx);
		AttachmentImageBarriers.push_back({Resource, AF_ShaderRead, barrier_state::shader_read, SUBRESOURCES_ALL, PSF_BottomOfPipe});
#else
		vulkan_texture* Resource = static_cast<vulkan_texture*>(*std::next(TexturesToCommon.begin(), Idx));
		std::fill(Resource->CurrentLayout.begin(), Resource->CurrentLayout.end(), 0);
		std::fill(Resource->CurrentState.begin(), Resource->CurrentState.end(), barrier_state::undefined);
		Resource->PrevShader = PSF_BottomOfPipe;
#endif
	}

	ImageBarrier(CommandList, GetVKPipelineStage(PSF_Transfer), GetVKPipelineStage(PSF_BottomOfPipe), ImageEndRenderBarriers);
#if USE_BOTTOM_OF_PIPE_BARRIERS
	SetImageBarriers(AttachmentImageBarriers);
	SetBufferBarriers(AttachmentBufferBarriers);
#endif

	BuffersToCommon.clear();
	TexturesToCommon.clear();
	RenderingAttachmentInfos.clear();
}

void vulkan_command_list::
End()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->Execute(&CommandList);

	PrevContext = nullptr;
	CurrContext = nullptr;
}

void vulkan_command_list::
EndOneTime()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	CommandQueue->ExecuteAndRemove(&CommandList);

	PrevContext = nullptr;
	CurrContext = nullptr;
}

void vulkan_command_list::
Present()
{
	vulkan_command_queue* CommandQueue = static_cast<vulkan_command_queue*>(Gfx->CommandQueue);
	PlaceEndOfFrameBarriers();
	CommandQueue->Execute(&CommandList, &ReleaseSemaphore, &AcquireSemaphore, &RenderFence);

	VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = &ReleaseSemaphore;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &Gfx->Swapchain;
	PresentInfo.pImageIndices = &BackBufferIndex;
	vkQueuePresentKHR(CommandQueue->Handle, &PresentInfo);

	PrevContext = nullptr;
	CurrContext = nullptr;
}


void vulkan_command_list::
EmplaceColorTarget(texture* RenderTexture)
{
	vulkan_texture* Texture = static_cast<vulkan_texture*>(RenderTexture);

	std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
	{
		CreateImageBarrier(Texture->Handle, GetVKAccessMask(Texture->CurrentLayout[0], Texture->PrevShader), GetVKAccessMask(AF_TransferRead, PSF_Transfer), GetVKLayout(Texture->CurrentState[0]), GetVKLayout(barrier_state::transfer_src)),
		CreateImageBarrier(Gfx->SwapchainImages[BackBufferIndex], 0, GetVKAccessMask(AF_TransferWrite, PSF_Transfer), GetVKLayout(barrier_state::undefined), GetVKLayout(barrier_state::transfer_dst)),
	};
	ImageBarrier(CommandList, GetVKPipelineStage(Texture->PrevShader), GetVKPipelineStage(PSF_Transfer), ImageCopyBarriers);

	VkImageCopy ImageCopyRegion = {};
	ImageCopyRegion.srcSubresource.aspectMask = Texture->Aspect;
	ImageCopyRegion.srcSubresource.layerCount = 1;
	ImageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageCopyRegion.dstSubresource.layerCount = 1;
	ImageCopyRegion.extent = {u32(Texture->Width), u32(Texture->Height), u32(Texture->Depth)};

	vkCmdCopyImage(CommandList, Texture->Handle, GetVKLayout(barrier_state::transfer_src), Gfx->SwapchainImages[BackBufferIndex], GetVKLayout(barrier_state::transfer_dst), 1, &ImageCopyRegion);
}

void vulkan_command_list::
Update(buffer* BufferToUpdate, void* Data)
{
	if(!Data) return;
	vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(BufferToUpdate);
	vulkan_buffer* Staging = static_cast<vulkan_buffer*>(BufferToUpdate->UpdateBuffer);
	SetBufferBarriers({{BufferToUpdate, AF_TransferWrite, PSF_Transfer}, {Staging, AF_TransferRead, PSF_Transfer}});

	void* CpuPtr;
	vmaMapMemory(Gfx->AllocatorHandle, Staging->Allocation, &CpuPtr);
	memcpy(CpuPtr, Data, Buffer->Size);
	vmaUnmapMemory(Gfx->AllocatorHandle, Staging->Allocation);

	VkBufferCopy Region = {0, 0, VkDeviceSize(Buffer->Size)};
	vkCmdCopyBuffer(CommandList, Staging->Handle, Buffer->Handle, 1, &Region);
}

void vulkan_command_list::
UpdateSize(buffer* BufferToUpdate, void* Data, u32 UpdateByteSize)
{
	if(!Data) return;
	vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(BufferToUpdate);
	vulkan_buffer* Staging = static_cast<vulkan_buffer*>(BufferToUpdate->UpdateBuffer);

	if(UpdateByteSize == 0) return;
	assert(UpdateByteSize <= Buffer->Size);

	SetBufferBarriers({{BufferToUpdate, AF_TransferWrite, PSF_Transfer}, {Staging, AF_TransferRead, PSF_Transfer}});

	void* CpuPtr;
	vmaMapMemory(Gfx->AllocatorHandle, Staging->Allocation, &CpuPtr);
	memcpy(CpuPtr, Data, UpdateByteSize);
	vmaUnmapMemory(Gfx->AllocatorHandle, Staging->Allocation);

	VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
	vkCmdCopyBuffer(CommandList, Staging->Handle, Buffer->Handle, 1, &Region);
}

void vulkan_command_list::
ReadBack(buffer* BufferToRead, void* Data)
{
	if(!Data) return;
	vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(BufferToRead);
	vulkan_buffer* Staging = static_cast<vulkan_buffer*>(BufferToRead->UploadBuffer);

	SetBufferBarriers({{BufferToRead, AF_TransferRead, PSF_Transfer}, {Staging, AF_TransferRead, PSF_Transfer}});

	VkBufferCopy Region = {0, 0, VkDeviceSize(Buffer->Size)};
	vkCmdCopyBuffer(CommandList, Buffer->Handle, Staging->Handle, 1, &Region);

	void* CpuPtr;
	vmaMapMemory(Gfx->AllocatorHandle, Staging->Allocation, &CpuPtr);
	memcpy(Data, CpuPtr, Buffer->Size);
	vmaUnmapMemory(Gfx->AllocatorHandle, Staging->Allocation);
}

void vulkan_command_list::
ReadBackSize(buffer* BufferToRead, void* Data, u32 UpdateByteSize)
{
	if(!Data) return;
	vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(BufferToRead);
	vulkan_buffer* Staging = static_cast<vulkan_buffer*>(BufferToRead->UploadBuffer);

	if (UpdateByteSize == 0) return;
	assert(UpdateByteSize <= Buffer->Size);

	SetBufferBarriers({{BufferToRead, AF_TransferRead, PSF_Transfer}, {Staging, AF_TransferRead, PSF_Transfer}});

	VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
	vkCmdCopyBuffer(CommandList, Buffer->Handle, Staging->Handle, 1, &Region);

	void* CpuPtr;
	vmaMapMemory(Gfx->AllocatorHandle, Staging->Allocation, &CpuPtr);
	memcpy(Data, CpuPtr, UpdateByteSize);
	vmaUnmapMemory(Gfx->AllocatorHandle, Staging->Allocation);
}

void vulkan_command_list::
Update(texture* TextureToUpdate, void* Data)
{
	if(!Data) return;
	vulkan_texture* Texture = static_cast<vulkan_texture*>(TextureToUpdate);
	vulkan_buffer* Staging = static_cast<vulkan_buffer*>(TextureToUpdate->UpdateBuffer);
	SetImageBarriers({{TextureToUpdate, AF_TransferWrite, barrier_state::transfer_dst, SUBRESOURCES_ALL, PSF_Transfer}});
	SetBufferBarriers({{Staging, AF_TransferRead, PSF_Transfer}});

	void* CpuPtr;
	vmaMapMemory(Gfx->AllocatorHandle, Staging->Allocation, &CpuPtr);
	memcpy(CpuPtr, Data, Texture->Width * Texture->Height * Texture->Depth * GetPixelSize(Texture->Info.Format));
	vmaUnmapMemory(Gfx->AllocatorHandle, Staging->Allocation);

	VkBufferImageCopy Region = {};
	Region.bufferOffset = 0;
	Region.bufferRowLength = 0;
	Region.bufferImageHeight = 0;
	Region.imageSubresource.aspectMask = Texture->Aspect;
	Region.imageSubresource.mipLevel = 0;
	Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = Texture->Info.Type != image_type::Texture3D ? Texture->Depth : 1;
	Region.imageOffset = {0, 0, 0};
	Region.imageExtent = {u32(Texture->Width), u32(Texture->Height), u32(Texture->Depth)};
	vkCmdCopyBufferToImage(CommandList, Staging->Handle, Texture->Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
}

void vulkan_command_list::
ReadBack(texture* TextureToRead, void* Data)
{
	if(!Data) return;
	vulkan_texture* Texture = static_cast<vulkan_texture*>(TextureToRead);
	vulkan_buffer* Staging = static_cast<vulkan_buffer*>(TextureToRead->UploadBuffer);
	SetImageBarriers({{TextureToRead, AF_TransferRead, barrier_state::transfer_src, SUBRESOURCES_ALL, PSF_Transfer}});
	SetBufferBarriers({{Staging, AF_TransferWrite, PSF_Transfer}});

	VkBufferImageCopy Region = {};
	Region.bufferOffset = 0;
	Region.bufferRowLength = 0;
	Region.bufferImageHeight = 0;
	Region.imageSubresource.aspectMask = Texture->Aspect;
	Region.imageSubresource.mipLevel = 0;
	Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = Texture->Info.Type != image_type::Texture3D ? Texture->Depth : 1;
	Region.imageOffset = {0, 0, 0};
	Region.imageExtent = {u32(Texture->Width), u32(Texture->Height), u32(Texture->Depth)};

	vkCmdCopyImageToBuffer(CommandList, Texture->Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Staging->Handle, 1, &Region);

	void* CpuPtr;
	vmaMapMemory(Gfx->AllocatorHandle, Staging->Allocation, &CpuPtr);
	memcpy(Data, CpuPtr, Texture->Width * Texture->Height * Texture->Depth * GetPixelSize(Texture->Info.Format));
	vmaUnmapMemory(Gfx->AllocatorHandle, Staging->Allocation);
}

void vulkan_command_list::
SetColorTarget(const std::vector<texture*>& ColorTargets, vec4 Clear)
{
	assert(CurrContext->Type == pass_type::raster);

	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);

	VkRenderingAttachmentInfoKHR* ColorInfo = PushArray(VkRenderingAttachmentInfoKHR, ColorTargets.size());
	for(u32 AttachmentIdx = 0; AttachmentIdx < ColorTargets.size(); ++AttachmentIdx)
	{
		vulkan_texture* Attachment = static_cast<vulkan_texture*>(ColorTargets[AttachmentIdx]);

		LayerCount = Attachment->Info.Type != image_type::Texture3D ? Attachment->Depth : 1;
		ColorInfo[AttachmentIdx].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		ColorInfo[AttachmentIdx].pNext = nullptr;
		ColorInfo[AttachmentIdx].imageView = Attachment->Views[0];
		ColorInfo[AttachmentIdx].imageLayout = GetVKLayout(barrier_state::color_attachment);
		ColorInfo[AttachmentIdx].loadOp = GetVKLoadOp(Context->LoadOp);
		ColorInfo[AttachmentIdx].storeOp = GetVKStoreOp(Context->StoreOp);
		memcpy(ColorInfo[AttachmentIdx].clearValue.color.float32, Clear.E, 4 * sizeof(float));

		AttachmentViews.push_back(Attachment->Views[0]);
		RenderTargetClears.push_back(ColorInfo[AttachmentIdx].clearValue);
	}

	RenderingInfo.colorAttachmentCount = ColorTargets.size();
	RenderingInfo.pColorAttachments = ColorInfo;

	RenderingAttachmentInfos.push_back(ColorInfo);
}

void vulkan_command_list::
SetDepthTarget(texture* Target, vec2 Clear)
{
	assert(CurrContext->Type == pass_type::raster);

	LayerCount = Target->Info.Type != image_type::Texture3D ? Target->Depth : 1;
	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);
	vulkan_texture* Attachment = static_cast<vulkan_texture*>(Target);

	VkRenderingAttachmentInfoKHR* DepthInfo = PushStruct(VkRenderingAttachmentInfoKHR);
	DepthInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	DepthInfo->pNext = nullptr;
	DepthInfo->imageView = Attachment->Views[0];
	DepthInfo->imageLayout = GetVKLayout(barrier_state::depth_stencil_attachment);
	DepthInfo->loadOp = GetVKLoadOp(Context->LoadOp);
	DepthInfo->storeOp = GetVKStoreOp(Context->StoreOp);
	DepthInfo->clearValue.depthStencil.depth   = Clear.E[0];
	DepthInfo->clearValue.depthStencil.stencil = Clear.E[1];
	RenderingInfo.pDepthAttachment = DepthInfo;

	AttachmentViews.push_back(Attachment->Views[0]);
	RenderTargetClears.push_back(DepthInfo->clearValue);
	RenderingAttachmentInfos.push_back(DepthInfo);
}

void vulkan_command_list::
SetStencilTarget(texture* Target, vec2 Clear)
{
	assert(CurrContext->Type == pass_type::raster);

	//AttachmentImageBarriers.push_back({Target, AF_DepthStencilAttachmentWrite, barrier_state::depth_stencil_attachment, SUBRESOURCES_ALL, PSF_EarlyFragment});

	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);
	vulkan_texture* Attachment = static_cast<vulkan_texture*>(Target);

	VkRenderingAttachmentInfoKHR* StencilInfo = PushStruct(VkRenderingAttachmentInfoKHR);
	StencilInfo->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	StencilInfo->pNext = nullptr;
	StencilInfo->imageView = Attachment->Views[0];
	StencilInfo->imageLayout = GetVKLayout(barrier_state::depth_stencil_attachment);
	StencilInfo->loadOp = GetVKLoadOp(Context->LoadOp);
	StencilInfo->storeOp = GetVKStoreOp(Context->StoreOp);
	StencilInfo->clearValue.depthStencil.depth   = Clear.E[0];
	StencilInfo->clearValue.depthStencil.stencil = Clear.E[1];
	RenderingInfo.pStencilAttachment = StencilInfo;

	AttachmentViews.push_back(Attachment->Views[0]);
	RenderTargetClears.push_back(StencilInfo->clearValue);
	RenderingAttachmentInfos.push_back(StencilInfo);
}

void vulkan_command_list::
BindShaderParameters(const array<binding_packet>& Data)
{
	if(!Data.size()) return;
	vulkan_resource_binder Binder(CurrContext);

	VkPipelineLayout Layout = {};
	VkPipelineBindPoint BindPoint = {};
	if(CurrContext->Type == pass_type::raster)
	{
		vulkan_render_context* ContextToBind = static_cast<vulkan_render_context*>(CurrContext);

		BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Layout = ContextToBind->RootSignatureHandle;
	}
	else if(CurrContext->Type == pass_type::compute)
	{
		vulkan_compute_context* ContextToBind = static_cast<vulkan_compute_context*>(CurrContext);

		BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		Layout = ContextToBind->RootSignatureHandle;
	}

	u32 Offset = 0;
	u32 ParamCount = CurrContext->ParameterLayout[0].size();
	for(u32 ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx, ++Offset)
	{
		descriptor_param Parameter = CurrContext->ParameterLayout[0][ParamIdx];
		if(Parameter.Type == resource_type::buffer_storage || Parameter.Type == resource_type::buffer_uniform)
		{
			buffer* BufferToBind = (buffer*)Data[Offset].Resource;
			Binder.SetBufferView(BufferToBind);
			ParamIdx += BufferToBind->WithCounter;
		}
		else if(Parameter.Type == resource_type::texture_sampler)
		{
			Binder.SetImageSampler(Parameter.Count, Data[Offset].Resources, Parameter.ImageType, Parameter.BarrierState, Data[Offset].SubresourceIndex);
		}
		else if(Parameter.Type == resource_type::texture_storage)
		{
			Binder.SetStorageImage(Parameter.Count, Data[Offset].Resources, Parameter.ImageType, Parameter.BarrierState, Data[Offset].SubresourceIndex);
		}
	}

	vkCmdPushDescriptorSetKHR(CommandList, BindPoint, Layout, 0, Binder.PushDescriptorBindings.size(), Binder.PushDescriptorBindings.data());
}

void vulkan_command_list::
BeginRendering(u32 RenderWidth, u32 RenderHeight)
{
	if(!CurrContext) return;
	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);
	if(Context->Type != pass_type::raster) return;

	GfxWidth  = RenderWidth;
	GfxHeight = RenderHeight;

	RenderingInfo.renderArea = RenderPassInfo.renderArea = {{}, {RenderWidth, RenderHeight}};
	RenderingInfo.layerCount = LayerCount;
	FramebufferCreateInfo.renderPass = Context->RenderPass;
	FramebufferCreateInfo.width  = RenderWidth;
	FramebufferCreateInfo.height = RenderHeight;
	FramebufferCreateInfo.layers = LayerCount;
	RenderPassInfo.renderPass = Context->RenderPass;

	if(!Gfx->Features13.dynamicRendering)
	{
		FramebufferCreateInfo.attachmentCount = AttachmentViews.size();
		FramebufferCreateInfo.pAttachments    = AttachmentViews.data();
		RenderPassInfo.clearValueCount        = RenderTargetClears.size();
		RenderPassInfo.pClearValues           = RenderTargetClears.data();

		VkFramebuffer& FrameBuffer = Context->FrameBuffers[Context->FrameBufferIdx];
		if(!FrameBuffer)
			vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &FrameBuffer);
		Context->FrameBufferIdx++;

		RenderPassInfo.framebuffer = FrameBuffer;
		vkCmdBeginRenderPass(CommandList, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	else
	{
		vkCmdBeginRenderingKHR(CommandList, &RenderingInfo);
	}
}

void vulkan_command_list::
EndRendering()
{
	if(!CurrContext) return;

	LayerCount = 1;
	PrevContext = CurrContext;

	if(CurrContext->Type != pass_type::raster) return;

	if(PrevContext && PrevContext->Type == pass_type::raster)
	{
		if(!Gfx->Features13.dynamicRendering)
		{
			vkCmdEndRenderPass(CommandList);
		}
		else
		{
			vkCmdEndRenderingKHR(CommandList);
		}
	}

	AttachmentViews.clear();
	RenderTargetClears.clear();
	RenderingAttachmentInfos.clear();

	RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	RenderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
}

#if 0
void vulkan_render_context::
Draw(u32 FirstVertex, u32 VertexCount, u32 FirstInstance, u32 InstanceCount)
{
	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);
	assert(CurrContext->Type == pass_type::raster);

	vkCmdDraw(*PipelineContext->CommandList, VertexCount, InstanceCount, FirstVertex, FirstInstance);
}
#endif

void vulkan_command_list::
DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount)
{
	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);
	assert(CurrContext->Type == pass_type::raster);

	vkCmdDrawIndexed(CommandList, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void vulkan_command_list::
DrawIndirect(u32 ObjectDrawCount, u32 CommandStructureSize)
{
	assert(CurrContext->Type == pass_type::raster);
	assert(IndirectCommands);

	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);
	vulkan_buffer* IndirectCommandsAttachment = static_cast<vulkan_buffer*>(IndirectCommands);

	vkCmdDrawIndexedIndirectCount(CommandList, IndirectCommandsAttachment->Handle, 4, IndirectCommandsAttachment->Handle, IndirectCommandsAttachment->CounterOffset, ObjectDrawCount, CommandStructureSize);

	IndirectCommands = nullptr;
}

void vulkan_command_list::
Dispatch(u32 X, u32 Y, u32 Z)
{
	assert(CurrContext->Type == pass_type::compute);

	vulkan_compute_context* Context = static_cast<vulkan_compute_context*>(CurrContext);
	vkCmdDispatch(CommandList, (X + Context->BlockSizeX - 1) / Context->BlockSizeX, (Y + Context->BlockSizeY - 1) / Context->BlockSizeY, (Z + Context->BlockSizeZ - 1) / Context->BlockSizeZ);
}

bool vulkan_command_list::
SetGraphicsPipelineState(render_context* Context)
{
	assert(Context->Type == pass_type::raster);
	if(Context == CurrContext) return false;

	CurrContext = Context;
	vulkan_render_context* ContextToBind = static_cast<vulkan_render_context*>(Context);
	vkCmdBindPipeline(CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, ContextToBind->Pipeline);

	std::vector<VkDescriptorSet> SetsToBind;
	for(const VkDescriptorSet& DescriptorSet : ContextToBind->Sets)
	{
		if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
	}

	if(SetsToBind.size())
		vkCmdBindDescriptorSets(CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, ContextToBind->RootSignatureHandle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);

	return true;
}

bool vulkan_command_list::
SetComputePipelineState(compute_context* Context)
{
	assert(Context->Type == pass_type::compute);
	if(Context == CurrContext) return false;

	CurrContext = Context;
	vulkan_compute_context* ContextToBind = static_cast<vulkan_compute_context*>(Context);
	vkCmdBindPipeline(CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, ContextToBind->Pipeline);

	std::vector<VkDescriptorSet> SetsToBind;
	for(const VkDescriptorSet& DescriptorSet : ContextToBind->Sets)
	{
		if(DescriptorSet) SetsToBind.push_back(DescriptorSet);
	}

	if(SetsToBind.size())
		vkCmdBindDescriptorSets(CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, ContextToBind->RootSignatureHandle, 1, SetsToBind.size(), SetsToBind.data(), 0, nullptr);

	return true;
}

void vulkan_command_list::
SetConstant(void* Data, size_t Size)
{
	if(CurrContext->Type == pass_type::raster)
	{
		vulkan_render_context* ContextToBind = static_cast<vulkan_render_context*>(CurrContext);
		vkCmdPushConstants(CommandList, ContextToBind->RootSignatureHandle, ContextToBind->ConstantRange.stageFlags, 0, Size, Data);
	}
	else if(CurrContext->Type == pass_type::compute)
	{
		vulkan_compute_context* ContextToBind = static_cast<vulkan_compute_context*>(CurrContext);
		vkCmdPushConstants(CommandList, ContextToBind->RootSignatureHandle, ContextToBind->ConstantRange.stageFlags, 0, Size, Data);
	}
}

void vulkan_command_list::
SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight)
{
	vulkan_render_context* Context = static_cast<vulkan_render_context*>(CurrContext);

	VkViewport Viewport = {(float)StartX, (float)(GfxHeight - StartY), (float)RenderWidth, -(float)RenderHeight, 0, 1};
	vkCmdSetViewport(CommandList, 0, 1, &Viewport);

	VkRect2D Scissor = {{(s32)StartX, (s32)(GfxHeight - (RenderHeight + StartY))}, {RenderWidth, RenderHeight}};
	vkCmdSetScissor(CommandList, 0, 1, &Scissor);
}

void vulkan_command_list::
SetIndexBuffer(buffer* Buffer)
{
	vulkan_buffer* IndexAttachment = static_cast<vulkan_buffer*>(Buffer);
	vkCmdBindIndexBuffer(CommandList, IndexAttachment->Handle, 0, VK_INDEX_TYPE_UINT32);
}

void vulkan_command_list::
FillBuffer(buffer* Buffer, u32 Value)
{
	vulkan_buffer* VulkanBuffer = static_cast<vulkan_buffer*>(Buffer);
	SetBufferBarriers({{Buffer, AF_TransferWrite, PSF_Transfer}});
	vkCmdFillBuffer(CommandList, VulkanBuffer->Handle, 0, VulkanBuffer->Size, Value);
}

void vulkan_command_list::
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

	SetImageBarriers({{Texture, AF_TransferWrite, barrier_state::transfer_dst, SUBRESOURCES_ALL, PSF_Transfer}});
	vkCmdClearColorImage(CommandList, static_cast<vulkan_texture*>(Texture)->Handle, GetVKLayout(Texture->CurrentState[0]), &ClearColor, 1, &ClearRange);
}

void vulkan_command_list::
FillTexture(texture* Texture, float Depth, u32 Stencil)
{
	SetImageBarriers({{Texture, AF_TransferWrite, barrier_state::transfer_dst, SUBRESOURCES_ALL, PSF_Transfer}});

	VkClearDepthStencilValue ClearDepthStencil = {};
	ClearDepthStencil.depth = Depth;
	ClearDepthStencil.stencil = Stencil;

	VkImageSubresourceRange ClearRange = {};
	ClearRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	ClearRange.baseMipLevel = 0;
	ClearRange.baseArrayLayer = 0;
	ClearRange.levelCount = VK_REMAINING_MIP_LEVELS;
	ClearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCmdClearDepthStencilImage(CommandList, static_cast<vulkan_texture*>(Texture)->Handle, GetVKLayout(Texture->CurrentState[0]), &ClearDepthStencil, 1, &ClearRange);
}

void vulkan_command_list::
CopyImage(texture* Dst, texture* Src)
{
	vulkan_texture* SrcTexture = static_cast<vulkan_texture*>(Src);
	vulkan_texture* DstTexture = static_cast<vulkan_texture*>(Dst);

	SetImageBarriers({{SrcTexture, AF_TransferWrite, barrier_state::transfer_src, SUBRESOURCES_ALL, PSF_Transfer}, 
					  {DstTexture, AF_TransferWrite, barrier_state::transfer_dst, SUBRESOURCES_ALL, PSF_Transfer}});

	VkImageCopy ImageCopyRegion = {};
	ImageCopyRegion.srcSubresource.aspectMask = SrcTexture->Aspect;
	ImageCopyRegion.srcSubresource.layerCount = 1;
	ImageCopyRegion.dstSubresource.aspectMask = DstTexture->Aspect;
	ImageCopyRegion.dstSubresource.layerCount = 1;
	ImageCopyRegion.extent = {(u32)Src->Width, (u32)Src->Height, 1};

	vkCmdCopyImage(CommandList, SrcTexture->Handle, GetVKLayout(barrier_state::transfer_src), DstTexture->Handle, GetVKLayout(barrier_state::transfer_dst), 1, &ImageCopyRegion);
}

void vulkan_command_list::
SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	VkMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
	Barrier.srcAccessMask = GetVKAccessMask(SrcAccess, SrcStageMask);
	Barrier.dstAccessMask = GetVKAccessMask(DstAccess, DstStageMask);

	vkCmdPipelineBarrier(CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), VK_DEPENDENCY_BY_REGION_BIT, 1, &Barrier, 0, 0, 0, 0);
}

void vulkan_command_list::
SetBufferBarriers(const std::vector<buffer_barrier>& BarrierData)
{
	std::vector<VkBufferMemoryBarrier> Barriers;
	u32 DstStageMask = 0;
	u32 SrcStageMask = 0;

	for(const buffer_barrier& Data : BarrierData)
	{
		vulkan_buffer* Buffer = static_cast<vulkan_buffer*>(Data.Buffer);
		BuffersToCommon.insert(Buffer);

		u32 BufferPrevShader = Buffer->PrevShader;
		    BufferPrevShader = BufferPrevShader & PSF_BottomOfPipe ? PSF_TopOfPipe : BufferPrevShader;
		u32 BufferNextShader = Data.Shader;
		u32 ResourceLayoutPrev = BufferPrevShader & PSF_TopOfPipe ? 0 : Buffer->CurrentLayout;
		u32 ResourceLayoutNext = BufferNextShader & PSF_BottomOfPipe ? 0 : Data.Aspect;
		Buffer->CurrentLayout = ResourceLayoutNext;
		Buffer->PrevShader = BufferNextShader;

		SrcStageMask |= BufferPrevShader;
		DstStageMask |= BufferNextShader;

		VkBufferMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		Barrier.buffer = Buffer->Handle;
		Barrier.srcAccessMask = GetVKAccessMask(ResourceLayoutPrev, BufferPrevShader);
		Barrier.dstAccessMask = GetVKAccessMask(ResourceLayoutNext, BufferNextShader);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.offset = 0;
		Barrier.size = Buffer->Size;

		Barriers.push_back(Barrier);
	}

	if(Barriers.size())
		vkCmdPipelineBarrier(CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), 0, 0, 0, Barriers.size(), Barriers.data(), 0, 0);
}

void vulkan_command_list::
SetImageBarriers(const std::vector<texture_barrier>& BarrierData)
{
	std::vector<VkImageMemoryBarrier> Barriers;
	u32 SrcStageMask = 0;
	u32 DstStageMask = 0;

	for(const texture_barrier& Data : BarrierData)
	{
		vulkan_texture* Texture = static_cast<vulkan_texture*>(Data.Texture);
		TexturesToCommon.insert(Texture);

		barrier_state ResourceStateNext = Data.State;

		u32 TexturePrevShader = Texture->PrevShader;
		    TexturePrevShader = TexturePrevShader & PSF_BottomOfPipe ? PSF_TopOfPipe : TexturePrevShader;
		u32 TextureNextShader = Data.Shader;
		u32 ResourceLayoutNext = TextureNextShader & PSF_BottomOfPipe ? 0 : Data.Aspect;
		Texture->PrevShader = TextureNextShader;

		SrcStageMask |= TexturePrevShader;
		DstStageMask |= TextureNextShader;

		u32 MipToUse = Data.Mips;
		VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		Barrier.dstAccessMask = GetVKAccessMask(ResourceLayoutNext, TextureNextShader);
		Barrier.newLayout = GetVKLayout(ResourceStateNext);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.image = Texture->Handle;
		Barrier.subresourceRange.aspectMask = Texture->Aspect;
		if(MipToUse == SUBRESOURCES_ALL)
		{
			bool AreAllSubresourcesInSameState = true;
			for(u32 MipIdx = 1; MipIdx < Texture->Info.MipLevels; ++MipIdx)
			{
				if(Texture->CurrentLayout[0] != Texture->CurrentLayout[MipIdx])
					AreAllSubresourcesInSameState = false;
			}

			if(AreAllSubresourcesInSameState)
			{
				Barrier.srcAccessMask = GetVKAccessMask(TexturePrevShader & PSF_TopOfPipe ? 0 : Texture->CurrentLayout[0], TexturePrevShader);
				Barrier.oldLayout = GetVKLayout(TexturePrevShader & PSF_TopOfPipe ? barrier_state::undefined : Texture->CurrentState[0]);

				Barrier.subresourceRange.baseMipLevel   = 0;
				Barrier.subresourceRange.baseArrayLayer = 0;
				Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				Barriers.push_back(Barrier);
			}
			else
			{
				for(u32 MipIdx = 0; MipIdx < Texture->Info.MipLevels; ++MipIdx)
				{
					Barrier.srcAccessMask = GetVKAccessMask(TexturePrevShader & PSF_TopOfPipe ? 0 : Texture->CurrentLayout[MipIdx], TexturePrevShader);
					Barrier.oldLayout = GetVKLayout(TexturePrevShader & PSF_TopOfPipe ? barrier_state::undefined : Texture->CurrentState[MipIdx]);

					Barrier.subresourceRange.baseMipLevel   = MipIdx;
					Barrier.subresourceRange.baseArrayLayer = 0;
					Barrier.subresourceRange.levelCount = 1;
					Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
					Barriers.push_back(Barrier);
				}
			}

			std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), ResourceLayoutNext);
			std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), ResourceStateNext);
		}
		else
		{
			Barrier.srcAccessMask = GetVKAccessMask(TexturePrevShader & PSF_TopOfPipe ? 0 : Texture->CurrentLayout[MipToUse], TexturePrevShader);
			Barrier.oldLayout = GetVKLayout(TexturePrevShader & PSF_TopOfPipe ? barrier_state::undefined : Texture->CurrentState[MipToUse]);

			Barrier.subresourceRange.baseMipLevel   = MipToUse;
			Barrier.subresourceRange.baseArrayLayer = 0;
			Barrier.subresourceRange.levelCount = 1;
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			Texture->CurrentLayout[MipToUse] = ResourceLayoutNext;
			Texture->CurrentState[MipToUse] = ResourceStateNext;
			Barriers.push_back(Barrier);
		}
	}

	if(Barriers.size())
		vkCmdPipelineBarrier(CommandList, GetVKPipelineStage(SrcStageMask), GetVKPipelineStage(DstStageMask), 
							 0, 0, 0, 0, 0, 
							 (u32)Barriers.size(), Barriers.data());
}

void vulkan_command_list::
DebugGuiBegin(texture* RenderTarget)
{
	vulkan_texture* Clr = static_cast<vulkan_texture*>(RenderTarget);

	VkRenderingInfoKHR RenderingInfoGui = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
	RenderingInfoGui.renderArea = {{}, {u32(RenderTarget->Width), u32(RenderTarget->Height)}};
	RenderingInfoGui.layerCount = 1;

	VkRenderingAttachmentInfoKHR ColorInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR};
	ColorInfo.imageView = Clr->Views[0];
	ColorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	ColorInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	ColorInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorInfo.clearValue = {0, 0, 0, 0};
	RenderingInfoGui.colorAttachmentCount = 1;
	RenderingInfoGui.pColorAttachments = &ColorInfo;

	//std::fill(Clr->CurrentLayout.begin(), Clr->CurrentLayout.end(), AF_ColorAttachmentWrite);
	//std::fill(Clr->CurrentState.begin(), Clr->CurrentState.end(), barrier_state::color_attachment);
	//Clr->PrevShader = PSF_ColorAttachment;

	if(Gfx->Features13.dynamicRendering)
		vkCmdBeginRenderingKHR(CommandList, &RenderingInfoGui);
}

void vulkan_command_list::
DebugGuiEnd()
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandList);

	if(Gfx->Features13.dynamicRendering)
		vkCmdEndRenderingKHR(CommandList);
}

vulkan_render_context::
vulkan_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::vector<std::string> ShaderList, 
					  const std::vector<image_format>& NewColorTargetFormats, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines)
	: LoadOp(NewLoadOp), StoreOp(NewStoreOp)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	Device = Gfx->Device;
	Type = pass_type::raster;

	u32 PushConstantSize = 0;
	u32 PushConstantStage = 0;
	bool HavePushConstant = false;
	std::map<VkDescriptorType, u32> DescriptorTypeCounts;
	std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> ShaderRootLayout;
	std::map<u32, std::map<u32, image_type>> TextureTypes;
	std::map<u32, std::map<u32, bool>> Writables;

	std::string GlobalName;
	for(const std::string Shader : ShaderList)
	{
		GlobalName = Shader.substr(Shader.find("."));

		VkPipelineShaderStageCreateInfo Stage = {};
		Stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Stage.pName  = "main";
		if(Shader.find(".vert.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::vertex, Writables, TextureTypes, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_VERTEX_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_control, Writables, TextureTypes, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_eval, Writables, TextureTypes, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::geometry, Writables, TextureTypes, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_GEOMETRY_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			bool UsingPushConstant = false;
			Stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			Stage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::fragment, Writables, TextureTypes, ShaderRootLayout, DescriptorTypeCounts, UsingPushConstant, PushConstantSize, ShaderDefines);

			PushConstantStage |= UsingPushConstant * VK_SHADER_STAGE_FRAGMENT_BIT;
			HavePushConstant  |= UsingPushConstant;
		}
		ShaderStages.push_back(Stage);
	}

	std::vector<u32> DescriptorsSizes(ShaderRootLayout.size());
	std::vector<std::vector<VkDescriptorBindingFlags>> BindingFlags(ShaderRootLayout.size());
	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			auto Parameter = ShaderRootLayout[LayoutIdx][BindingIdx];
			DescriptorsSizes[LayoutIdx] += Parameter.descriptorCount;
			Parameters[LayoutIdx].push_back(Parameter);

			if(Parameter.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || 
			   Parameter.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				BindingFlags[LayoutIdx].push_back(LayoutIdx > 0 ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
				ParameterLayout[LayoutIdx].push_back({resource_type::texture_sampler, Parameter.descriptorCount, false, TextureTypes[LayoutIdx][BindingIdx], GetVKShaderStageRev(Parameter.stageFlags), barrier_state::shader_read, AF_ShaderRead});
			}
			else if(Parameter.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			{
				BindingFlags[LayoutIdx].push_back(LayoutIdx > 0 ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
				ParameterLayout[LayoutIdx].push_back({resource_type::texture_storage, Parameter.descriptorCount, true, TextureTypes[LayoutIdx][BindingIdx], GetVKShaderStageRev(Parameter.stageFlags), barrier_state::general, AF_ShaderWrite});
			}
			else if(Parameter.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				BindingFlags[LayoutIdx].push_back(0);
				ParameterLayout[LayoutIdx].push_back({resource_type::buffer_storage, Parameter.descriptorCount, Writables[LayoutIdx][BindingIdx], image_type::unknown, GetVKShaderStageRev(Parameter.stageFlags), barrier_state::general, Writables[LayoutIdx][BindingIdx] ? u32(AF_ShaderWrite) : u32(AF_ShaderRead)});
			}
		}
	}

	Layouts.resize(ShaderRootLayout.size());
	Sets.resize(ShaderRootLayout.size(), VK_NULL_HANDLE);
	PushDescriptors.resize(ShaderRootLayout.size());
	for(u32 SpaceIdx = 0; SpaceIdx < ShaderRootLayout.size(); ++SpaceIdx)
	{
		VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
		BindingFlagsInfo.bindingCount  = Parameters[SpaceIdx].size();
		BindingFlagsInfo.pBindingFlags = BindingFlags[SpaceIdx].data();

		PushDescriptors[SpaceIdx] = (DescriptorsSizes[SpaceIdx] < Gfx->PushDescProps.maxPushDescriptors);
		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
		DescriptorSetLayoutCreateInfo.flags = (PushDescriptors[SpaceIdx] * VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
		DescriptorSetLayoutCreateInfo.bindingCount = Parameters[SpaceIdx].size();
		DescriptorSetLayoutCreateInfo.pBindings = Parameters[SpaceIdx].data();
		DescriptorSetLayoutCreateInfo.pNext = SpaceIdx > 0 ? &BindingFlagsInfo : nullptr;

		VkDescriptorSetLayout DescriptorSetLayout;
		VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
		Layouts[SpaceIdx] = DescriptorSetLayout;
	}

	std::vector<VkDescriptorPoolSize> PoolSizes;
	for (const auto& DescriptorType : DescriptorTypeCounts)
	{
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

	// TODO: Better fillement when building render graph
	std::vector<VkFormat> ColorTargetFormats;
	std::vector<VkAttachmentReference> AttachmentReferences;
	std::vector<VkAttachmentDescription> AttachmentDescriptions;
	for(u32 FormatIdx = 0; FormatIdx < NewColorTargetFormats.size(); ++FormatIdx)
	{
		VkAttachmentReference   AttachRef  = {};
		VkAttachmentDescription AttachDesc = {};
		AttachDesc.format        = GetVKFormat(NewColorTargetFormats[FormatIdx]);
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
	DepthAttachRef.attachment = ColorTargetFormats.size();
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

	VkPipelineVertexInputStateCreateInfo VertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	InputAssemblyState.topology = GetVKTopology(InputData.Topology);

	std::vector<VkPipelineColorBlendAttachmentState> ColorAttachmentState(ColorTargetFormats.size());
	for(u32 Idx = 0; Idx < ColorTargetFormats.size(); ++Idx)
	{
		ColorAttachmentState[Idx].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorAttachmentState[Idx].blendEnable = InputData.UseBlend;
		ColorAttachmentState[Idx].srcColorBlendFactor = GetVKBlendFactor(InputData.BlendSrc);
		ColorAttachmentState[Idx].dstColorBlendFactor = GetVKBlendFactor(InputData.BlendDst);
		ColorAttachmentState[Idx].srcAlphaBlendFactor = GetVKBlendFactor(InputData.BlendSrc);
		ColorAttachmentState[Idx].dstAlphaBlendFactor = GetVKBlendFactor(InputData.BlendDst);
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
	RasterizationState.frontFace = GetVKFrontFace(InputData.FrontFace);
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

	size_t LastSlashPos = GlobalName.find_last_of('/');
    size_t LastDotPos = GlobalName.find_last_of('.');
    Name = GlobalName = GlobalName.substr(LastSlashPos + 1, LastDotPos - LastSlashPos - 1);

#ifdef CE_DEBUG
	VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	DebugNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
	DebugNameInfo.objectHandle = (u64)Pipeline;
	DebugNameInfo.pObjectName = Name.c_str();
	vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
#endif
}

vulkan_compute_context::
vulkan_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines)
{
	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	Device = Gfx->Device;
	Type = pass_type::compute;

	u32 PushConstantSize = 0;
	bool HavePushConstant = false;
	std::map<VkDescriptorType, u32> DescriptorTypeCounts;
	std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> ShaderRootLayout;
	std::map<u32, std::map<u32, image_type>> TextureTypes;
	std::map<u32, std::map<u32, bool>> Writables;

	ComputeStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	ComputeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	ComputeStage.module = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::compute, Writables, TextureTypes, ShaderRootLayout, DescriptorTypeCounts, HavePushConstant, PushConstantSize, ShaderDefines, &BlockSizeX, &BlockSizeY, &BlockSizeZ);
	ComputeStage.pName  = "main";

	std::vector<u32> DescriptorsSizes(ShaderRootLayout.size());
	std::vector<std::vector<VkDescriptorBindingFlags>> BindingFlags(ShaderRootLayout.size());
	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			auto Parameter = ShaderRootLayout[LayoutIdx][BindingIdx];
			DescriptorsSizes[LayoutIdx] += Parameter.descriptorCount;
			Parameters[LayoutIdx].push_back(Parameter);

			if(Parameter.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || 
			   Parameter.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				BindingFlags[LayoutIdx].push_back(LayoutIdx > 0 ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
				ParameterLayout[LayoutIdx].push_back({resource_type::texture_sampler, Parameter.descriptorCount, false, TextureTypes[LayoutIdx][BindingIdx], GetVKShaderStageRev(Parameter.stageFlags), barrier_state::shader_read, AF_ShaderRead});
			}
			else if(Parameter.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			{
				BindingFlags[LayoutIdx].push_back(LayoutIdx > 0 ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
				ParameterLayout[LayoutIdx].push_back({resource_type::texture_storage, Parameter.descriptorCount, true, TextureTypes[LayoutIdx][BindingIdx], GetVKShaderStageRev(Parameter.stageFlags), barrier_state::general, AF_ShaderWrite});
			}
			else if(Parameter.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				BindingFlags[LayoutIdx].push_back(0);
				ParameterLayout[LayoutIdx].push_back({resource_type::buffer_storage, Parameter.descriptorCount, Writables[LayoutIdx][BindingIdx], image_type::unknown, GetVKShaderStageRev(Parameter.stageFlags), barrier_state::general, Writables[LayoutIdx][BindingIdx] ? u32(AF_ShaderWrite) : u32(AF_ShaderRead)});
			}
		}
	}

	Layouts.resize(ShaderRootLayout.size());
	Sets.resize(ShaderRootLayout.size(), VK_NULL_HANDLE);
	PushDescriptors.resize(ShaderRootLayout.size());
	for(u32 SpaceIdx = 0; SpaceIdx < ShaderRootLayout.size(); ++SpaceIdx)
	{
		VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
		BindingFlagsInfo.bindingCount  = Parameters[SpaceIdx].size();
		BindingFlagsInfo.pBindingFlags = BindingFlags[SpaceIdx].data();

		PushDescriptors[SpaceIdx] = (DescriptorsSizes[SpaceIdx] < Gfx->PushDescProps.maxPushDescriptors);
		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
		DescriptorSetLayoutCreateInfo.flags = PushDescriptors[SpaceIdx] * VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		DescriptorSetLayoutCreateInfo.bindingCount = Parameters[SpaceIdx].size();
		DescriptorSetLayoutCreateInfo.pBindings = Parameters[SpaceIdx].data();
		DescriptorSetLayoutCreateInfo.pNext = SpaceIdx > 0 ? &BindingFlagsInfo : nullptr;

		VkDescriptorSetLayout DescriptorSetLayout;
		VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
		Layouts[SpaceIdx] = DescriptorSetLayout;
	}


	std::vector<VkDescriptorPoolSize> PoolSizes;
	for (const auto& DescriptorType : DescriptorTypeCounts)
	{
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

	size_t LastSlashPos = Shader.find_last_of('/');
    size_t LastDotPos = Shader.find_last_of('.');
	Name = Shader.substr(LastSlashPos + 1, LastDotPos - LastSlashPos - 1);

#ifdef CE_DEBUG
	VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	DebugNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
	DebugNameInfo.objectHandle = (u64)Pipeline;
	DebugNameInfo.pObjectName = Name.c_str();
	vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
#endif
}
