#pragma once

// TODO: make UpdateSize() function to do a resource recreation if update size is bigger than current one()
struct vulkan_buffer : public buffer
{
	friend vulkan_command_list;
	vulkan_buffer() = default;
	~vulkan_buffer() override = default;

	vulkan_buffer(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, void* Data, u64 NewSize, u64 Count, u32 Flags)
	{
		Flags |= resource_flags::RF_CopyDst;
		CreateResource(Backend, Heap, DebugName, NewSize, Count, Flags);
		Update(Backend, Data);
	}

	vulkan_buffer(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewSize, u64 Count, u32 Flags)
	{
		CreateResource(Backend, Heap, DebugName, NewSize, Count, Flags);
	}

	void Update(renderer_backend* Backend, void* Data) override
	{
#if 0
		vulkan_command_queue* CommandQueue = static_cast<vulkan_backend*>(Backend)->CommandQueue;

		CommandQueue->Reset();
		VkCommandBuffer CommandList = CommandQueue->AllocateCommandList();

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = 0;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = Size;

		vkCmdPipelineBarrier(CommandList, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(Size)};
		vkCmdCopyBuffer(CommandList, Temp, Handle, 1, &Region);

		CommandQueue->ExecuteAndRemove(&CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
#else
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->Update(this, Data);
		Cmd->EndOneTime();
#endif
	}

	void UpdateSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) override
	{
#if 0
		if(UpdateByteSize == 0) return;
		assert(UpdateByteSize <= Size);

		vulkan_command_queue* CommandQueue = static_cast<vulkan_backend*>(Backend)->CommandQueue;

		CommandQueue->Reset();
		VkCommandBuffer CommandList = CommandQueue->AllocateCommandList();

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = 0;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = UpdateByteSize;

		vkCmdPipelineBarrier(CommandList, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, UpdateByteSize, 0, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(CommandList, Temp, Handle, 1, &Region);

		CommandQueue->ExecuteAndRemove(&CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
#else
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->UpdateSize(this, Data, UpdateByteSize);
		Cmd->EndOneTime();
#endif
	}

	void ReadBackSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) override
	{
#if 0
		if (UpdateByteSize == 0) return;
		assert(UpdateByteSize <= Size);

		vulkan_command_queue* CommandQueue = static_cast<vulkan_backend*>(Backend)->CommandQueue;

		CommandQueue->Reset();
		VkCommandBuffer CommandList = CommandQueue->AllocateCommandList();

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Temp;
		CopyBarrier.srcAccessMask = 0;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = UpdateByteSize;

		vkCmdPipelineBarrier(CommandList, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(CommandList, Handle, Temp, 1, &Region);

		CommandQueue->ExecuteAndRemove(&CommandList);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(Data, CpuPtr, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VK_CHECK(vkDeviceWaitIdle(Device));
#else
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->ReadBackSize(this, Data, UpdateByteSize);
		Cmd->EndOneTime();
#endif
	}

	void Update(void* Data, command_list* Cmd) override
	{
		Cmd->Update(this, Data);
	}

	void UpdateSize(void* Data, u32 UpdateByteSize, command_list* Cmd) override
	{
		Cmd->UpdateSize(this, Data, UpdateByteSize);
	}

	void ReadBackSize(void* Data, u32 UpdateByteSize, command_list* Cmd) override
	{
		Cmd->ReadBackSize(this, Data, UpdateByteSize);
	}

	void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewSize, u64 Count, u32 Flags) override
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		vulkan_memory_heap* VulkanHeap = static_cast<vulkan_memory_heap*>(Heap);
		vulkan_command_queue* CommandQueue = static_cast<vulkan_backend*>(Backend)->CommandQueue;
		WithCounter = Flags & RF_WithCounter;
		PrevShader = PSF_TopOfPipe;

		Device = Gfx->Device;
		Size = NewSize * Count + WithCounter * sizeof(u32);
		CounterOffset = NewSize * Count;

		VkBufferCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		CreateInfo.size = Size;

		if(Flags & RF_VertexBuffer)
			CreateInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if(Flags & RF_IndexBuffer)
			CreateInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if(Flags & RF_IndirectBuffer)
			CreateInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if(Flags & RF_ConstantBuffer)
			CreateInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if(Flags & RF_StorageBuffer)
			CreateInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if(Flags & RF_CopySrc)
			CreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if(Flags & RF_CopyDst)
			CreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VmaAllocationCreateInfo AllocCreateInfo = {};
		AllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationInfo AllocationInfo;
		VK_CHECK(vmaCreateBuffer(VulkanHeap->Handle, &CreateInfo, &AllocCreateInfo, &Handle, &Allocation, &AllocationInfo));
		Memory = AllocationInfo.deviceMemory;

		Name = DebugName;
		std::string TempName = (DebugName + ".buffer");
		VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
		DebugNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		DebugNameInfo.objectHandle = (u64)Handle;
		DebugNameInfo.pObjectName = TempName.c_str();
		vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);

		CreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		CreateInfo.size = Size;
		vkCreateBuffer(Device, &CreateInfo, 0, &Temp);

		VkMemoryRequirements Requirements;
		vkGetBufferMemoryRequirements(Device, Temp, &Requirements);
		u32 MemoryTypeIndex = SelectMemoryType(Gfx->MemoryProperties, Requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(MemoryTypeIndex != ~0u);

		VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
		AllocateInfo.allocationSize = Requirements.size;
		vkAllocateMemory(Device, &AllocateInfo, 0, &TempMemory);

		vkBindBufferMemory(Device, Temp, TempMemory, 0);

		TempName += ".temp";
		DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
		DebugNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		DebugNameInfo.objectHandle = (u64)Temp;
		DebugNameInfo.pObjectName = TempName.c_str();
		vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);

		CommandQueue->Reset();
		VkCommandBuffer CommandList = CommandQueue->AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, NewSize, 0, &CpuPtr);
		memset(CpuPtr, 0, NewSize);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(Size)};
		vkCmdCopyBuffer(CommandList, Temp, Handle, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = Size;

		vkCmdPipelineBarrier(CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		CommandQueue->ExecuteAndRemove(&CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	void DestroyResource() override
	{
		vkFreeMemory(Device, Memory    , nullptr);
		vkFreeMemory(Device, TempMemory, nullptr);

		vkDestroyBuffer(Device, Temp  , nullptr);
		vkDestroyBuffer(Device, Handle, nullptr);

		TempMemory = 0;
		Temp = 0;
		Memory = 0;
		Handle = 0;
	}

	VkBuffer Handle;
	VkDeviceMemory Memory;
	VmaAllocation Allocation;

private:
	VkDevice Device;
	VkBuffer Temp;
	VkDeviceMemory TempMemory;
};

struct vulkan_texture_sampler : public texture_sampler
{
	vulkan_texture_sampler() = default;
	vulkan_texture_sampler(renderer_backend* Backend, u32 MipLevels = 1, const utils::texture::sampler_info& SamplerInfo = {border_color::black_opaque, sampler_address_mode::clamp_to_edge, sampler_reduction_mode::weighted_average, filter::linear, filter::linear, mipmap_mode::linear})
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		Device = Gfx->Device;

		VkSamplerReductionModeCreateInfoEXT ReductionMode = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT};
		ReductionMode.reductionMode = GetVKSamplerReductionMode(SamplerInfo.ReductionMode);

		VkSamplerCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		CreateInfo.pNext = &ReductionMode;
		CreateInfo.minFilter = GetVKFilter(SamplerInfo.MinFilter);
		CreateInfo.magFilter = GetVKFilter(SamplerInfo.MagFilter);
		CreateInfo.mipmapMode = GetVKMipmapMode(SamplerInfo.MipmapMode);
		CreateInfo.addressModeU = CreateInfo.addressModeV = CreateInfo.addressModeW = GetVKSamplerAddressMode(SamplerInfo.AddressMode);
		CreateInfo.borderColor = GetVKBorderColor(SamplerInfo.BorderColor);
		CreateInfo.compareEnable = false;
		CreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		CreateInfo.maxLod = MipLevels;

		VK_CHECK(vkCreateSampler(Device, &CreateInfo, nullptr, &SamplerHandle));
	}

	void DestroyObject()
	{
		vkDestroySampler(Device, SamplerHandle, nullptr);
		SamplerHandle = 0;
	}
	~vulkan_texture_sampler()
	{
		DestroyObject();
	}

	VkDevice Device;
	VkSampler SamplerHandle;
};

// TODO: Better image view handling
struct vulkan_texture : public texture
{
	friend vulkan_command_list;
	vulkan_texture() = default;
	~vulkan_texture() override = default;

	vulkan_texture(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const utils::texture::input_data& InputData = {image_format::R8G8B8A8_UINT, image_type::Texture2D, image_flags::TF_Storage, 1, 1, false, barrier_state::undefined, {border_color::black_opaque, sampler_address_mode::clamp_to_edge, sampler_reduction_mode::weighted_average, filter::linear, filter::linear, mipmap_mode::linear}})
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		CreateResource(Backend, Heap, DebugName, NewWidth, NewHeight, DepthOrArraySize, InputData);
		if(Data || Info.UseStagingBuffer)
		{
			CreateStagingResource();
			if(Data) Update(Backend, Data);
		}
		if(Data && !Info.UseStagingBuffer)
			DestroyStagingResource();

		VkSamplerReductionModeCreateInfoEXT ReductionMode = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT};
		ReductionMode.reductionMode = GetVKSamplerReductionMode(Info.SamplerInfo.ReductionMode);

		VkSamplerCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		CreateInfo.pNext = &ReductionMode;
		CreateInfo.minFilter = GetVKFilter(Info.SamplerInfo.MinFilter);
		CreateInfo.magFilter = GetVKFilter(Info.SamplerInfo.MagFilter);
		CreateInfo.mipmapMode = GetVKMipmapMode(Info.SamplerInfo.MipmapMode);
		CreateInfo.addressModeU = CreateInfo.addressModeV = CreateInfo.addressModeW = GetVKSamplerAddressMode(Info.SamplerInfo.AddressMode);
		CreateInfo.borderColor = GetVKBorderColor(Info.SamplerInfo.BorderColor);
		CreateInfo.compareEnable = false;
		CreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		CreateInfo.maxLod = Info.MipLevels;

		VK_CHECK(vkCreateSampler(Device, &CreateInfo, nullptr, &SamplerHandle));
	}

	void Update(renderer_backend* Backend, void* Data) override
	{
#if 0
		vulkan_command_queue* CommandQueue = static_cast<vulkan_backend*>(Backend)->CommandQueue;

		CommandQueue->Reset();
		VkCommandBuffer CommandList = CommandQueue->AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkImageMemoryBarrier CopyBarrier = CreateImageBarrier(Handle, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdPipelineBarrier(CommandList, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &CopyBarrier);

		VkBufferImageCopy Region = {};
		Region.imageSubresource.aspectMask = Aspect;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = Info.Layers;
		Region.imageExtent = {u32(Width), u32(Height), u32(Depth)};
		vkCmdCopyBufferToImage(CommandList, Temp, Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

		CommandQueue->ExecuteAndRemove(&CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
#else
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->Update(this, Data);
		Cmd->EndOneTime();
#endif
	}

	void ReadBack(renderer_backend* Backend, void* Data) override
	{
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->ReadBack(this, Data);
		Cmd->EndOneTime();
	}

	void Update(void* Data, command_list* Cmd) override
	{
		Cmd->Update(this, Data);
	}

	void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const utils::texture::input_data& InputData) override
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		vulkan_memory_heap* VulkanHeap = static_cast<vulkan_memory_heap*>(Heap);
		vulkan_command_queue* CommandQueue = static_cast<vulkan_backend*>(Backend)->CommandQueue;

		CurrentLayout.resize(InputData.MipLevels);
		CurrentState.resize(InputData.MipLevels);

		MemoryProperties = Gfx->MemoryProperties;
		Device = Gfx->Device;

		Aspect = 0;
		Width  = NewWidth;
		Height = NewHeight;
		Depth  = DepthOrArraySize;
		Info   = InputData;
		PrevShader = PSF_TopOfPipe;

		VkImageCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		CreateInfo.imageType = GetVKImageType(Info.Type);
		CreateInfo.format = GetVKFormat(Info.Format);
		CreateInfo.extent.width  = (u32)NewWidth;
		CreateInfo.extent.height = (u32)NewHeight;
		CreateInfo.extent.depth  = (u32)DepthOrArraySize;
		CreateInfo.mipLevels = InputData.MipLevels;
		CreateInfo.arrayLayers = Info.Layers;
		CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;//Gfx->MsaaQuality;
		CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if(Info.Usage & image_flags::TF_CubeMap)
			CreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		if(Info.Usage & image_flags::TF_DepthTexture || Info.Usage & image_flags::TF_StencilTexture)
			CreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if(Info.Usage & image_flags::TF_ColorAttachment)
			CreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if(Info.Usage & image_flags::TF_Storage)
			CreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		if(Info.Usage & image_flags::TF_Sampled)
			CreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if(Info.Usage & image_flags::TF_CopySrc)
			CreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		//if(Info.Usage & image_flags::TF_CopyDst)
			CreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkClearColorValue ClearColor = {};
		VkClearDepthStencilValue ClearDepthStencil = {};
		if(Info.Usage & image_flags::TF_ColorAttachment || Info.Usage & image_flags::TF_ColorTexture)
		{
			Aspect |= VK_IMAGE_ASPECT_COLOR_BIT;
			ClearColor.float32[0] = 0.0f;
			ClearColor.float32[1] = 0.0f;
			ClearColor.float32[2] = 0.0f;
			ClearColor.float32[3] = 1.0f;
		}
		if(Info.Usage & image_flags::TF_DepthTexture)
		{
			Aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
			ClearDepthStencil.depth = 1.0f;
			ClearDepthStencil.stencil = 0;
		}
		if(Info.Usage & image_flags::TF_StencilTexture)
		{
			Aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
			ClearDepthStencil.depth = 1.0f;
			ClearDepthStencil.stencil = 0;
		}

		VmaAllocationCreateInfo AllocCreateInfo = {};
		AllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationInfo AllocationInfo;
		VK_CHECK(vmaCreateImage(VulkanHeap->Handle, &CreateInfo, &AllocCreateInfo, &Handle, &Allocation, &AllocationInfo));
		Memory = AllocationInfo.deviceMemory;
		Size = AllocationInfo.size;

		Name = DebugName;

		std::string TempName = (DebugName + ".texture");
		VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
		DebugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		DebugNameInfo.objectHandle = (u64)Handle;
		DebugNameInfo.pObjectName = TempName.c_str();
		vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);

		VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ViewCreateInfo.format = CreateInfo.format;
		ViewCreateInfo.image = Handle;
		ViewCreateInfo.subresourceRange.aspectMask = Aspect;
		ViewCreateInfo.subresourceRange.layerCount = Info.Layers;
		ViewCreateInfo.subresourceRange.levelCount = 1;

		if(InputData.Usage & image_flags::TF_CubeMap)
		{
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		}
		else
		{
			if(InputData.Type == image_type::Texture1D)
				ViewCreateInfo.viewType = InputData.Layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
			if(InputData.Type == image_type::Texture2D)
				ViewCreateInfo.viewType = InputData.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			if(InputData.Type == image_type::Texture3D)
				ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		}
		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			ViewCreateInfo.subresourceRange.baseMipLevel = MipIdx;

			VkImageView Result = 0;
			VK_CHECK(vkCreateImageView(Device, &ViewCreateInfo, 0, &Result));
			Views.push_back(Result);

			std::string NewImageViewName = (Name + ".image_view #" + std::to_string(MipIdx));
			DebugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			DebugNameInfo.objectHandle = (u64)Result;
			DebugNameInfo.pObjectName = NewImageViewName.c_str();
			vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
		}
	}

	void CreateStagingResource() override
	{
		VkBufferCreateInfo TempCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		TempCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		TempCreateInfo.size = Size;
		VK_CHECK(vkCreateBuffer(Device, &TempCreateInfo, 0, &Temp));

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, Temp, &MemoryRequirements);

		u32 MemoryTypeIndex = SelectMemoryType(MemoryProperties, MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(MemoryTypeIndex != ~0u);

		VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
		AllocateInfo.allocationSize = Size;
		vkAllocateMemory(Device, &AllocateInfo, 0, &TempMemory);
		vkBindBufferMemory(Device, Temp, TempMemory, 0);

		std::string TempName = (Name + ".buffer.temp");
		VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
		DebugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		DebugNameInfo.objectHandle = (u64)Temp;
		DebugNameInfo.pObjectName = TempName.c_str();
		vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
	}

	void DestroyResource() override
	{
		vkFreeMemory(Device, Memory, nullptr);
		for(VkImageView& View : Views)
		{
			vkDestroyImageView(Device, View, nullptr);
		}
		vkDestroyImage(Device, Handle, nullptr);

		Memory = 0;
		Handle = 0;
		Views.clear();
	}

	void DestroyStagingResource() override
	{
		vkFreeMemory(Device, TempMemory, nullptr);
		vkDestroyBuffer(Device, Temp, nullptr);

		TempMemory = 0;
		Temp = 0;
	}

	VkImage Handle;
	VkDeviceMemory Memory;
	VkImageAspectFlags Aspect;
	std::vector<VkImageView> Views;

	VkSampler SamplerHandle;
	VmaAllocation Allocation;

private:
	VkDevice Device;
	VkBuffer Temp;
	VkDeviceMemory TempMemory;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
};

void vulkan_memory_heap::
CreateResource(renderer_backend* Backend)
{
	VmaVulkanFunctions VulkanFunctions = {};
	VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	VulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	VmaAllocatorCreateInfo AllocatorInfo = {};
	AllocatorInfo.vulkanApiVersion = Gfx->HighestUsedVulkanVersion;
	AllocatorInfo.instance = Gfx->Instance;
	AllocatorInfo.physicalDevice = Gfx->PhysicalDevice;
	AllocatorInfo.device = Gfx->Device;
	AllocatorInfo.pVulkanFunctions = &VulkanFunctions;
	vmaCreateAllocator(&AllocatorInfo, &Handle);
}

buffer* vulkan_memory_heap::
PushBuffer(renderer_backend* Backend, std::string DebugName, u64 DataSize, u64 Count, u32 Flags)
{
	buffer* Buffer = new vulkan_buffer(Backend, this, DebugName, DataSize, Count, Flags);
	u64 Hash = std::hash<buffer>()(*Buffer);
	return Buffer;
}

buffer* vulkan_memory_heap::
PushBuffer(renderer_backend* Backend, std::string DebugName, void* Data, u64 DataSize, u64 Count, u32 Flags)
{
	buffer* Buffer = new vulkan_buffer(Backend, this, DebugName, Data, DataSize, Count, Flags);
	u64 Hash = std::hash<buffer>()(*Buffer);
	return Buffer;
}

texture* vulkan_memory_heap::
PushTexture(renderer_backend* Backend, std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
{
	texture* Texture = new vulkan_texture(Backend, this, DebugName, nullptr, Width, Height, Depth, InputData);
	u64 Hash = std::hash<texture>()(*Texture);
	return  Texture;
}

texture* vulkan_memory_heap::
PushTexture(renderer_backend* Backend, std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
{
	texture* Texture = new vulkan_texture(Backend, this, DebugName, Data, Width, Height, Depth, InputData);
	u64 Hash = std::hash<texture>()(*Texture);
	return  Texture;
}
