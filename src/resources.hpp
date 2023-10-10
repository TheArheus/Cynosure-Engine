
struct resource_layout
{
	VkAccessFlags Access;
	VkImageLayout ImageLayout;
};

// TODO: Add a constructor for using outside staging buffer
struct buffer
{
	buffer() = default;

	template<class T, class heap>
	buffer(std::unique_ptr<T>& App, heap* Heap, 
		   u64 _Offset, void* Data, u64 NewSize, u64& Alignment, bool NewWithCounter, VkBufferUsageFlags Flags) : WithCounter(NewWithCounter), Size(NewSize), Offset(_Offset)
	{
		Handle = Heap->Handle;
		Device = App->Device;
		Update(App, Data);
	}

	template<class T, class heap>
	buffer(std::unique_ptr<T>& App, heap* Heap, 
		   u64 _Offset, u64 NewSize, u64& Alignment, bool NewWithCounter, VkBufferUsageFlags Flags) : WithCounter(NewWithCounter), Size(NewSize), Offset(_Offset)
	{
		Handle = Heap->Handle;
		Device = App->Device;
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, 
		   void* Data, u64 NewSize, bool NewWithCounter, u64 Alignment, VkBufferUsageFlags Flags) : WithCounter(NewWithCounter), Size(NewSize), Offset(0)
	{
		Device = App->Device;
		CreateResource(App, NewSize, NewWithCounter, Alignment, Flags);
		Update(App, Data);
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, 
		   u64 NewSize, bool NewWithCounter, u64 Alignment, VkBufferUsageFlags Flags) : WithCounter(NewWithCounter), Size(NewSize), Offset(0)
	{
		Device = App->Device;
		CreateResource(App, NewSize, NewWithCounter, Alignment, Flags);
	}

	template<class T>
	void Update(std::unique_ptr<T>& App, void* Data)
	{
		App->CommandQueue.Reset();
		VkCommandBuffer* CommandList = App->CommandQueue.AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(Size)};
		vkCmdCopyBuffer(*CommandList, Temp, Handle, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = 0;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = Size;

		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		App->CommandQueue.ExecuteAndRemove(CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class T>
	void UpdateSize(std::unique_ptr<T>& App, void* Data, u32 UpdateByteSize)
	{
		if(UpdateByteSize == 0) return;
		assert(UpdateByteSize >= Size);

		App->CommandQueue.Reset();
		VkCommandBuffer* CommandList = App->CommandQueue.AllocateCommandList();

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, UpdateByteSize, 0, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(*CommandList, Temp, Handle, 1, &Region);

		VkBufferMemoryBarrier CopyBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		CopyBarrier.buffer = Handle;
		CopyBarrier.srcAccessMask = 0;
		CopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		CopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		CopyBarrier.offset = 0;
		CopyBarrier.size = UpdateByteSize;

		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &CopyBarrier, 0, 0);

		App->CommandQueue.ExecuteAndRemove(CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class T, class global_pipeline_context>
	void Update(std::unique_ptr<T>& App, void* Data, global_pipeline_context& PipelineContext)
	{
		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(Size)};
		vkCmdCopyBuffer(*PipelineContext.CommandList, Temp, Handle, 1, &Region);
	}

	template<class T, class global_pipeline_context>
	void UpdateSize(std::unique_ptr<T>& App, void* Data, u32 UpdateByteSize, global_pipeline_context& PipelineContext)
	{
		if(UpdateByteSize == 0) return;
		assert(UpdateByteSize < Size);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, UpdateByteSize, 0, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VkBufferCopy Region = {0, 0, VkDeviceSize(UpdateByteSize)};
		vkCmdCopyBuffer(*PipelineContext.CommandList, Temp, Handle, 1, &Region);
	}

	template<class T>
	void ReadBackSize(std::unique_ptr<T>& App, void* Data, u32 UpdateByteSize)
	{
		App->CommandQueue.Reset();
		VkCommandBuffer* CommandList = App->CommandQueue.AllocateCommandList();

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

		App->CommandQueue.ExecuteAndRemove(CommandList);

		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(Data, CpuPtr, UpdateByteSize);
		vkUnmapMemory(Device, TempMemory);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class T, class global_pipeline_context>
	void ReadBackSize(std::unique_ptr<T>& App, void* Data, u32 UpdateByteSize, global_pipeline_context& PipelineContext)
	{
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

	template<class T, class heap>
	void CreateResource(std::unique_ptr<T>& App, heap* Heap, 
		   u64 Offset, u64 NewSize, u64& Alignment, bool NewWithCounter, VkBufferUsageFlags Flags)
	{
		Size = Alignment == 0 ? NewSize + WithCounter * sizeof(u32) : 
						 AlignUp(NewSize + WithCounter * sizeof(u32), Alignment);
		CounterOffset = NewSize;

		VkBufferCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		CreateInfo.usage = Flags;
		CreateInfo.size = Size;
		VK_CHECK(vkCreateBuffer(Device, &CreateInfo, 0, &Handle));

		VkMemoryRequirements Requirements;
		vkGetBufferMemoryRequirements(Device, Handle, &Requirements);
		Alignment = Requirements.alignment;

		u32 MemoryTypeIndex = SelectMemoryType(App->MemoryProperties, Requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		assert(MemoryTypeIndex != ~0u);

		vkBindBufferMemory(Device, Handle, Heap->Memory, Offset);
	}

	template<class T>
	void CreateResource(std::unique_ptr<T>& App, 
		   u64 NewSize, bool NewWithCounter, u64 Alignment, VkBufferUsageFlags Flags)
	{

		Size = Alignment == 0 ? NewSize + WithCounter * sizeof(u32) : 
						 AlignUp(NewSize + WithCounter * sizeof(u32), Alignment);
		CounterOffset = NewSize;

		VkBufferCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		CreateInfo.usage = Flags;
		CreateInfo.size = Size;
		VK_CHECK(vkCreateBuffer(Device, &CreateInfo, 0, &Handle));

		VkMemoryRequirements Requirements;
		vkGetBufferMemoryRequirements(Device, Handle, &Requirements);

		u32 MemoryTypeIndex = SelectMemoryType(App->MemoryProperties, Requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		assert(MemoryTypeIndex != ~0u);

		VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
		AllocateInfo.allocationSize = Requirements.size;
		VK_CHECK(vkAllocateMemory(Device, &AllocateInfo, 0, &Memory));

		vkBindBufferMemory(Device, Handle, Memory, 0);

		CreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		CreateInfo.size = Size;
		vkCreateBuffer(Device, &CreateInfo, 0, &Temp);

		vkGetBufferMemoryRequirements(Device, Temp, &Requirements);
		MemoryTypeIndex = SelectMemoryType(App->MemoryProperties, Requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(MemoryTypeIndex != ~0u);

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

	u64  Size;
	u64  Offset;
	u32  CounterOffset = 0;
	bool WithCounter   = false;

	VkBuffer Handle;
	VkDeviceMemory Memory;

	resource_layout Layout;

private:
	VkDevice Device;
	VkBuffer Temp;
	VkDeviceMemory TempMemory;
};

struct texture
{
	struct sampler
	{
		sampler() = default;

		sampler(VkDevice& Device, u32 MipLevelCount, VkSamplerReductionMode ReductionModeValue)
		{
			VkSamplerReductionModeCreateInfoEXT ReductionMode = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT};
			ReductionMode.reductionMode = ReductionModeValue;

			VkSamplerCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
			CreateInfo.pNext = &ReductionMode;
			CreateInfo.magFilter = VK_FILTER_LINEAR;
			CreateInfo.minFilter = VK_FILTER_LINEAR;
			CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
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
		VkImageViewType ViewType;
		u32 MipLevels;
		u32 Layers;
		u64 Alignment;
		bool UseStagingBuffer;
	};

	texture() = default;

	template<class T, class heap>
	texture(std::unique_ptr<T>& App, heap* Heap, u64 Offset, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const input_data& InputData = {VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, 1, 1, 0, false}, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE): Width(NewWidth), Height(NewHeight), Depth(DepthOrArraySize)
	{
		Info = InputData;
		Device = App->Device;
		Sampler = sampler(Device, InputData.MipLevels, ReductionMode);

		CreateResource(App->MemoryProperties, Heap, Offset, NewWidth, NewHeight, DepthOrArraySize, InputData.ImageType, InputData.Format, InputData.Usage, InputData.MipLevels, InputData.Layers, InputData.Alignment);
		Update(App, Data);

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			CreateView(InputData.ViewType, MipIdx, 1);
		}
		if(!Info.UseStagingBuffer)
			DestroyStagingResource();
	}

	template<class T, class heap>
	texture(std::unique_ptr<T>& App, heap* Heap, u64 Offset, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const input_data& InputData = {VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, 1, 1, 0, false}, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE): Width(NewWidth), Height(NewHeight), Depth(DepthOrArraySize)
	{
		Info = InputData;
		Device = App->Device;
		Sampler = sampler(Device, InputData.MipLevels, ReductionMode);

		CreateResource(App->MemoryProperties, Heap, Offset, NewWidth, NewHeight, DepthOrArraySize, InputData.ImageType, InputData.Format, InputData.Usage, InputData.MipLevels, InputData.Layers, InputData.Alignment);

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			CreateView(InputData.ViewType, MipIdx, 1);
		}
		if(!Info.UseStagingBuffer)
			DestroyStagingResource();
	}

	template<class T>
	texture(std::unique_ptr<T>& App, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const input_data& InputData = {VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, 1, 1, 0, false}, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE): Width(NewWidth), Height(NewHeight), Depth(DepthOrArraySize), Info(InputData)
	{
		Info = InputData;
		Device = App->Device;
		Sampler = sampler(Device, InputData.MipLevels, ReductionMode);

		CreateResource(App->MemoryProperties, NewWidth, NewHeight, DepthOrArraySize, InputData.ImageType, InputData.Format, InputData.Usage, InputData.MipLevels, InputData.Layers, InputData.Alignment);
		Update(App, Data);

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			CreateView(InputData.ViewType, MipIdx, 1);
		}
		if(!Info.UseStagingBuffer)
			DestroyStagingResource();
	}

	template<class T>
	texture(std::unique_ptr<T>& App, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const input_data& InputData = {VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, 1, 1, 0, false}, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE): Width(NewWidth), Height(NewHeight), Depth(DepthOrArraySize), Info(InputData)
	{
		Info = InputData;
		Device = App->Device;
		Sampler = sampler(Device, InputData.MipLevels, ReductionMode);

		CreateResource(App->MemoryProperties, NewWidth, NewHeight, DepthOrArraySize, InputData.ImageType, InputData.Format, InputData.Usage, InputData.MipLevels, InputData.Layers, InputData.Alignment);

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			CreateView(InputData.ViewType, MipIdx, 1);
		}
		if(!Info.UseStagingBuffer)
			DestroyStagingResource();
	}

	template<class T>
	void Update(std::unique_ptr<T>& App, void* Data)
	{
		App->CommandQueue.Reset();
		VkCommandBuffer* CommandList = App->CommandQueue.AllocateCommandList();

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
		Region.imageSubresource.layerCount = 1;
		Region.imageExtent = {u32(Width), u32(Height), u32(Depth)};
		vkCmdCopyBufferToImage(*CommandList, Temp, Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

		CopyBarrier = CreateImageBarrier(Handle, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vkCmdPipelineBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &CopyBarrier);

		App->CommandQueue.ExecuteAndRemove(CommandList);

		VK_CHECK(vkDeviceWaitIdle(Device));
	}

	template<class T, class global_pipeline_context>
	void Update(std::unique_ptr<T>& App, void* Data, global_pipeline_context& PipelineContext)
	{
		void* CpuPtr;
		vkMapMemory(Device, TempMemory, 0, Size, 0, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		vkUnmapMemory(Device, TempMemory);

		VkImageMemoryBarrier CopyBarrier = CreateImageBarrier(Handle, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdPipelineBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &CopyBarrier);

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = (Info.Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = {0, 0, 0};
		Region.imageExtent = {u32(Width), u32(Height), u32(Depth)};
		vkCmdCopyBufferToImage(*PipelineContext.CommandList, Temp, Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

		CopyBarrier = CreateImageBarrier(Handle, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vkCmdPipelineBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &CopyBarrier);
	}

	template<class T>
	void ReadBack(std::unique_ptr<T>& App, void* Data)
	{
	}

	template<class heap>
	void CreateResource(const VkPhysicalDeviceMemoryProperties& _MemoryProperties, heap* Heap, u64 Offset, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, VkImageType ImageType = VK_IMAGE_TYPE_2D, VkFormat _Format = VK_FORMAT_R8G8B8A8_UINT, u32 _MipLevels = 1, u32 Layers = 1, u64 _Alignment = 0)
	{
	}

	void CreateResource(const VkPhysicalDeviceMemoryProperties& _MemoryProperties, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, VkImageType ImageType = VK_IMAGE_TYPE_2D, VkFormat _Format = VK_FORMAT_R8G8B8A8_UINT, VkImageUsageFlags _Usage = VK_IMAGE_USAGE_STORAGE_BIT, u32 _MipLevels = 1, u32 Layers = 1, u64 _Alignment = 0)
	{
		Layout.ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		MemoryProperties = _MemoryProperties;

		VkImageCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		CreateInfo.imageType = ImageType;
		CreateInfo.format = _Format;
		CreateInfo.extent = {(u32)NewWidth, (u32)NewHeight, (u32)DepthOrArraySize};
		CreateInfo.mipLevels = _MipLevels;
		CreateInfo.arrayLayers = Layers;
		CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;//App->MsaaQuality;
		CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		CreateInfo.usage = _Usage;
		CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

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

		VkBufferCreateInfo TempCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		TempCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		TempCreateInfo.size = Size;
		VK_CHECK(vkCreateBuffer(Device, &TempCreateInfo, 0, &Temp));

		vkGetBufferMemoryRequirements(Device, Temp, &MemoryRequirements);

		MemoryTypeIndex = SelectMemoryType(MemoryProperties, MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(MemoryTypeIndex != ~0u);

		AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
		AllocateInfo.allocationSize = Size;
		vkAllocateMemory(Device, &AllocateInfo, 0, &TempMemory);
		vkBindBufferMemory(Device, Temp, TempMemory, 0);
	}

	void CreateView(VkImageViewType Type, u32 MipLevel, u32 LevelCount)
	{
		VkImageAspectFlags Aspect = (Info.Format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ViewCreateInfo.format = Info.Format;
		ViewCreateInfo.viewType = Type;
		ViewCreateInfo.image = Handle;
		ViewCreateInfo.subresourceRange.aspectMask = Aspect;
		ViewCreateInfo.subresourceRange.baseMipLevel = MipLevel;
		ViewCreateInfo.subresourceRange.layerCount = 1;
		ViewCreateInfo.subresourceRange.levelCount = LevelCount;

		VkImageView Result = 0;
		VK_CHECK(vkCreateImageView(Device, &ViewCreateInfo, 0, &Result));
		Views.push_back(Result);
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
	input_data Info;

	VkImage Handle;
	VkDeviceMemory Memory;
	std::vector<VkImageView> Views;

	sampler Sampler;
	resource_layout Layout;

private:
	u64 Size;

	VkDevice Device;
	VkBuffer Temp;
	VkDeviceMemory TempMemory;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
};

// TODO: Implement this properly
class memory_heap
{
public:
	template<typename T>
	memory_heap(std::unique_ptr<T>& Gfx, u64 NewSize, VkBufferUsageFlags Flags) : 
		Size(NewSize)
	{
		//Handle = buffer(Gfx, NewSize, );
	}

	void Reset()
	{
		BeginData = 0;
	}

	template<class T>
	buffer PushBuffer(std::unique_ptr<T>& App, 
					  void* Data, u64 DataSize)
	{
		u64 AlignedSize = 0;
		buffer Buffer(App, this, BeginData, Data, DataSize, AlignedSize);
		assert(AlignedSize + TotalSize <= Size);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Buffer;
	}

	buffer Handle;

private:
	u64 Size = 0;
	u64 TotalSize = 0;
	u64 BeginData = 0;
	u64 Alignment = 0;
};
