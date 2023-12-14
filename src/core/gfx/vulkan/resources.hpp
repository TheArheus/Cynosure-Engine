#pragma once

// TODO: make UpdateSize() function to do a resource recreation if update size is bigger than current one()
struct buffer
{
	struct resource_layout
	{
		VkAccessFlags Access;
		VkPipelineStageFlags StageMask;
	};

	buffer() = default;

	template<class heap>
	buffer(renderer_backend* Backend, heap* Heap, command_queue& CommandQueue,
		   void* Data, u64 NewSize, bool NewWithCounter, VkBufferUsageFlags Flags) : WithCounter(NewWithCounter), Size(NewSize)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		Device = Gfx->Device;
		CreateResource(Backend, Heap, NewSize, NewWithCounter, Flags);
		Update(CommandQueue, Data);
	}

	template<class heap>
	buffer(renderer_backend* Backend, heap* Heap, command_queue& CommandQueue,
		   u64 NewSize, bool NewWithCounter, VkBufferUsageFlags Flags) : WithCounter(NewWithCounter), Size(NewSize)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		Device = Gfx->Device;
		CreateResource(Backend, Heap, NewSize, NewWithCounter, Flags);
	}

	void Update(command_queue& CommandQueue, void* Data)
	{
		CommandQueue.Reset();
		VkCommandBuffer* CommandList = CommandQueue.AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(Size)};
		vkCmdCopyBuffer(*CommandList, Temp, Handle, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = Size;

		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		CommandQueue.ExecuteAndRemove(CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	void UpdateSize(command_queue& CommandQueue, void* Data, u32 UpdateByteSize)
	{
		if(UpdateByteSize == 0) return;
		assert(UpdateByteSize <= Size);

		CommandQueue.Reset();
		VkCommandBuffer* CommandList = CommandQueue.AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, UpdateByteSize, 0, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(*CommandList, Temp, Handle, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = UpdateByteSize;

		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		CommandQueue.ExecuteAndRemove(CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class global_pipeline_context>
	void Update(void* Data, global_pipeline_context& PipelineContext)
	{
		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(Size)};
		vkCmdCopyBuffer(*PipelineContext.CommandList, Temp, Handle, 1, &Region);
	}

	template<class global_pipeline_context>
	void UpdateSize(void* Data, u32 UpdateByteSize, global_pipeline_context& PipelineContext)
	{
		if(UpdateByteSize == 0) return;
		assert(UpdateByteSize <= Size);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, UpdateByteSize, 0, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(*PipelineContext.CommandList, Temp, Handle, 1, &Region);
	}

	void ReadBackSize(command_queue& CommandQueue, void* Data, u32 UpdateByteSize)
	{
		if (UpdateByteSize == 0) return;
		assert(UpdateByteSize <= Size);

		CommandQueue.Reset();
		VkCommandBuffer* CommandList = CommandQueue.AllocateCommandList();

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(*CommandList, Handle, Temp, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Temp;
		CopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = UpdateByteSize;

		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		CommandQueue.ExecuteAndRemove(CommandList);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(Data, CpuPtr, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class global_pipeline_context>
	void ReadBackSize(void* Data, u32 UpdateByteSize, global_pipeline_context& PipelineContext)
	{
		if (UpdateByteSize == 0) return;
		assert(UpdateByteSize <= Size);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(*PipelineContext.CommandList, Handle, Temp, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Temp;
		CopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = UpdateByteSize;

		vkCmdPipelineBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(Data, CpuPtr, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);
	}

	template<class heap>
	void CreateResource(renderer_backend* Backend, heap* Heap, 
		   u64 NewSize, bool NewWithCounter, VkBufferUsageFlags Flags)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		Size = NewSize + WithCounter * sizeof(u32);
		CounterOffset = NewSize;

		VkBufferCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		CreateInfo.usage = Flags;
		CreateInfo.size = Size;

		VmaAllocationCreateInfo AllocCreateInfo = {};
		AllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationInfo AllocationInfo;
		VK_CHECK(vmaCreateBuffer(Heap->Handle, &CreateInfo, &AllocCreateInfo, &Handle, &Allocation, &AllocationInfo));
		Memory = AllocationInfo.deviceMemory;

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
	}

	void DestroyResource()
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

	u64  Size          = 0;
	u64  Alignment     = 0;
	u32  CounterOffset = 0;
	bool WithCounter   = false;

	VkBuffer Handle;
	VkDeviceMemory Memory;
	VmaAllocation Allocation;

	resource_layout Layout = {0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

private:
	VkDevice Device;
	VkBuffer Temp;
	VkDeviceMemory TempMemory;
};

struct texture
{
	struct resource_layout
	{
		VkAccessFlags Access;
		VkImageLayout ImageLayout;
		VkImageAspectFlags ImageAspect;
		VkPipelineStageFlags StageMask;
	};

	struct sampler
	{
		sampler() = default;

		sampler(VkDevice& Device, u32 MipLevelCount, VkSamplerReductionMode ReductionModeValue, VkSamplerAddressMode AddressMode)
		{
			VkSamplerReductionModeCreateInfoEXT ReductionMode = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT};
			ReductionMode.reductionMode = ReductionModeValue;

			VkSamplerCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
			CreateInfo.pNext = &ReductionMode;
			CreateInfo.magFilter = VK_FILTER_LINEAR;
			CreateInfo.minFilter = VK_FILTER_LINEAR;
			CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			CreateInfo.addressModeU = AddressMode;
			CreateInfo.addressModeV = AddressMode;
			CreateInfo.addressModeW = AddressMode;
			CreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			CreateInfo.compareEnable = false;
			CreateInfo.compareOp = VK_COMPARE_OP_NEVER;
			CreateInfo.maxLod = MipLevelCount;

			VK_CHECK(vkCreateSampler(Device, &CreateInfo, nullptr, &Handle));
		}

		VkSampler Handle;
	};
	struct input_data
	{
		VkFormat Format;
		VkImageUsageFlags Usage;
		VkImageType ImageType;
		VkImageLayout StartLayout;
		VkImageViewType ViewType;
		u32 MipLevels;
		u32 Layers;
		bool UseStagingBuffer;
	};

	texture() = default;

	template<class heap>
	texture(renderer_backend* Backend, heap* Heap, command_queue& CommandQueue, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const input_data& InputData = {VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TYPE_2D, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_VIEW_TYPE_2D, 1, 1, false}, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE): Width(NewWidth), Height(NewHeight), Depth(DepthOrArraySize)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		MemoryProperties = Gfx->MemoryProperties;

		if(InputData.Layers == 6)
		{
			Width  = Max(NewWidth, NewHeight);
			Height = Max(NewWidth, NewHeight);
		}

		Info = InputData;
		Device = Gfx->Device;
		Sampler = sampler(Device, InputData.MipLevels, ReductionMode, AddressMode);
		Layout.ImageAspect = (Info.Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		CreateResource(Heap, NewWidth, NewHeight, DepthOrArraySize, InputData);
		if(Data || Info.UseStagingBuffer)
		{
			CreateStagingResource();
			Update(CommandQueue, Data);
		}
		if(Data && !Info.UseStagingBuffer)
			DestroyStagingResource();

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			CreateView(InputData.ViewType, MipIdx, 1);
		}
	}

	texture(renderer_backend* Backend, command_queue& CommandQueue, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const input_data& InputData = {VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TYPE_2D, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_VIEW_TYPE_2D, 1, 1, false}, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE): Width(NewWidth), Height(NewHeight), Depth(DepthOrArraySize), Info(InputData)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		MemoryProperties = Gfx->MemoryProperties;

		if(InputData.Layers == 6)
		{
			Width  = Max(NewWidth, NewHeight);
			Height = Max(NewWidth, NewHeight);
		}

		Info = InputData;
		Device = Gfx->Device;
		Sampler = sampler(Device, InputData.MipLevels, ReductionMode, AddressMode);
		Layout.ImageAspect = (Info.Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		CreateResource(NewWidth, NewHeight, DepthOrArraySize, InputData);
		if(Data || Info.UseStagingBuffer)
		{
			CreateStagingResource();
			Update(CommandQueue, Data);
		}
		if(Data && !Info.UseStagingBuffer)
			DestroyStagingResource();

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			CreateView(InputData.ViewType, MipIdx, 1);
		}
	}

	void Update(command_queue& CommandQueue, void* Data)
	{
		CommandQueue.Reset();
		VkCommandBuffer* CommandList = CommandQueue.AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkImageMemoryBarrier CopyBarrier = CreateImageBarrier(Handle, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &CopyBarrier);

		VkBufferImageCopy Region = {};
		Region.imageSubresource.aspectMask = (Info.Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = Info.Layers;
		Region.imageExtent = {u32(Width), u32(Height), u32(Depth)};
		vkCmdCopyBufferToImage(*CommandList, Temp, Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

		CopyBarrier = CreateImageBarrier(Handle, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &CopyBarrier);

		CommandQueue.ExecuteAndRemove(CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class global_pipeline_context>
	void Update(void* Data, global_pipeline_context& PipelineContext)
	{
		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = (Info.Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = Info.Layers;
		Region.imageOffset = {0, 0, 0};
		Region.imageExtent = {u32(Width), u32(Height), u32(Depth)};
		vkCmdCopyBufferToImage(*PipelineContext.CommandList, Temp, Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
	}

	void ReadBack(renderer_backend* Backend, void* Data)
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
	}

	template<class heap>
	void CreateResource(heap* Heap, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const input_data& InputData)
	{
		Info = InputData;

		VkImageCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		CreateInfo.flags = Info.Layers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		CreateInfo.imageType = Info.ImageType;
		CreateInfo.format = Info.Format;
		CreateInfo.extent.width  = Info.Layers == 6 ? Max((u32)NewWidth, (u32)NewHeight) : (u32)NewWidth;
		CreateInfo.extent.height = Info.Layers == 6 ? Max((u32)NewWidth, (u32)NewHeight) : (u32)NewHeight;
		CreateInfo.extent.depth  = (u32)DepthOrArraySize;
		CreateInfo.mipLevels = InputData.MipLevels;
		CreateInfo.arrayLayers = Info.Layers;
		CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;//Gfx->MsaaQuality;
		CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		CreateInfo.usage = Info.Usage;
		CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CreateInfo.initialLayout = Info.StartLayout;

		VmaAllocationCreateInfo AllocCreateInfo = {};
		AllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationInfo AllocationInfo;
		VK_CHECK(vmaCreateImage(Heap->Handle, &CreateInfo, &AllocCreateInfo, &Handle, &Allocation, &AllocationInfo));
		Memory = AllocationInfo.deviceMemory;
		Size = AllocationInfo.size;
	}

	void CreateResource(u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const input_data& InputData)
	{
		Info = InputData;

		VkImageCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		CreateInfo.flags = Info.Layers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		CreateInfo.imageType = Info.ImageType;
		CreateInfo.format = Info.Format;
		CreateInfo.extent.width  = Info.Layers == 6 ? Max((u32)NewWidth, (u32)NewHeight) : (u32)NewWidth;
		CreateInfo.extent.height = Info.Layers == 6 ? Max((u32)NewWidth, (u32)NewHeight) : (u32)NewHeight;
		CreateInfo.extent.depth  = (u32)DepthOrArraySize;
		CreateInfo.mipLevels = Info.MipLevels;
		CreateInfo.arrayLayers = Info.Layers;
		CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;//Gfx->MsaaQuality;
		CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		CreateInfo.usage = Info.Usage;
		CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CreateInfo.initialLayout = Info.StartLayout;

		VK_CHECK(vkCreateImage(Device, &CreateInfo, 0, &Handle));

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, Handle, &MemoryRequirements);
		Size = MemoryRequirements.size;

		u32 MemoryTypeIndex = SelectMemoryType(MemoryProperties, MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		assert(MemoryTypeIndex != ~0u);

		VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
		AllocateInfo.allocationSize = MemoryRequirements.size;

		vkAllocateMemory(Device, &AllocateInfo, 0, &Memory);
		vkBindImageMemory(Device, Handle, Memory, 0);
	}

	void CreateView(VkImageViewType Type, u32 MipLevel, u32 LevelCount)
	{
		VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ViewCreateInfo.format = Info.Format;
		ViewCreateInfo.viewType = Type;
		ViewCreateInfo.image = Handle;
		ViewCreateInfo.subresourceRange.aspectMask = Layout.ImageAspect;
		ViewCreateInfo.subresourceRange.baseMipLevel = MipLevel;
		ViewCreateInfo.subresourceRange.layerCount = Info.Layers;
		ViewCreateInfo.subresourceRange.levelCount = LevelCount;

		VkImageView Result = 0;
		VK_CHECK(vkCreateImageView(Device, &ViewCreateInfo, 0, &Result));
		Views.push_back(Result);
	}

	void CreateStagingResource()
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
	}

	void DestroyResource()
	{
		vkFreeMemory(Device, Memory, nullptr);
		for(VkImageView& View : Views)
		{
			vkDestroyImageView(Device, View, nullptr);
		}
		vkDestroyImage(Device, Handle, nullptr);
		vkDestroySampler(Device, Sampler.Handle, nullptr);

		Memory = 0;
		Handle = 0;
		Sampler.Handle = 0;
		Views.clear();
	}

	void DestroyStagingResource()
	{
		vkFreeMemory(Device, TempMemory, nullptr);
		vkDestroyBuffer(Device, Temp, nullptr);

		TempMemory = 0;
		Temp = 0;
	}

	u64 Width;
	u64 Height;
	u64 Depth;
	u64 Size;
	input_data Info;

	VkImage Handle;
	VkDeviceMemory Memory;
	VmaAllocation Allocation;
	std::vector<VkImageView> Views;

	sampler Sampler;
	resource_layout Layout = {0, VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

private:
	VkDevice Device;
	VkBuffer Temp;
	VkDeviceMemory TempMemory;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
};

// TODO: Implement this properly
class memory_heap
{
public:
	memory_heap() = default;

	memory_heap(renderer_backend* Backend)
	{
		CreateResource(Backend);
	}

	void CreateResource(renderer_backend* Backend)
	{
		VmaVulkanFunctions VulkanFunctions = {};
		VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		VulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);
		VmaAllocatorCreateInfo AllocatorInfo = {};
		AllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		AllocatorInfo.instance = Gfx->Instance;
		AllocatorInfo.physicalDevice = Gfx->PhysicalDevice;
		AllocatorInfo.device = Gfx->Device;
		AllocatorInfo.pVulkanFunctions = &VulkanFunctions;
		vmaCreateAllocator(&AllocatorInfo, &Handle);
	}

	buffer PushBuffer(renderer_backend* Backend, command_queue& CommandQueue,
					   u64 DataSize, bool NewWithCounter, VkBufferUsageFlags Flags)
	{
		buffer Buffer(Backend, this, CommandQueue, DataSize, NewWithCounter, Flags);
		return Buffer;
	}

	buffer PushBuffer(renderer_backend* Backend, command_queue& CommandQueue,
					   void* Data, u64 DataSize, bool NewWithCounter, VkBufferUsageFlags Flags)
	{
		buffer Buffer(Backend, this, CommandQueue, Data, DataSize, NewWithCounter, Flags);
		return Buffer;
	}

	texture PushTexture(renderer_backend* Backend, command_queue& CommandQueue, u32 Width, u32 Height, u32 Depth, const texture::input_data& InputData, 
						 VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, 
						 VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		texture Texture(Backend, this, CommandQueue, nullptr, Width, Height, Depth, InputData, ReductionMode, AddressMode);
		return  Texture;
	}

	texture PushTexture(renderer_backend* Backend, command_queue& CommandQueue,
					     void* Data, u32 Width, u32 Height, u32 Depth, const texture::input_data& InputData, 
						 VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, 
						 VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		texture Texture(Backend, this, CommandQueue, Data, Width, Height, Depth, InputData, ReductionMode, AddressMode);
		return  Texture;
	}

	VmaAllocator Handle;
};
