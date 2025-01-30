#pragma once

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

		VK_CHECK(vkCreateSampler(Device, &CreateInfo, nullptr, &Handle));
	}

	void DestroyObject()
	{
		if(Handle != VK_NULL_HANDLE)
		{
			vkDestroySampler(Device, Handle, nullptr);
			Handle = VK_NULL_HANDLE;
		}
	}
	~vulkan_texture_sampler()
	{
		DestroyObject();
	}

	VkDevice Device = VK_NULL_HANDLE;
	VkSampler Handle = VK_NULL_HANDLE;
};

// TODO: make UpdateSize() function to do a resource recreation if update size is bigger than current one()
struct vulkan_buffer : public buffer
{
	friend vulkan_command_list;
	vulkan_buffer() = default;
	~vulkan_buffer() override { DestroyResource(); }

    vulkan_buffer(const vulkan_buffer&) = delete;
    vulkan_buffer& operator=(const vulkan_buffer&) = delete;

	vulkan_buffer(renderer_backend* Backend, std::string DebugName, void* Data, u64 NewSize, u64 Count, u32 Flags, bool IsStagingCreated = false)
	{
		CreateResource(Backend, DebugName, NewSize, Count, Flags);
		if(!IsStagingCreated)
		{
			UpdateBuffer = new vulkan_buffer(Gfx, Name + ".update_staging_buffer", nullptr, NewSize, Count, RF_CpuWrite, true);
			UploadBuffer = new vulkan_buffer(Gfx, Name + ".upload_staging_buffer", nullptr, NewSize, Count, RF_CpuRead, true);
		}
		if(Data) Update(Backend, Data);
	}

	void Update(renderer_backend* Backend, void* Data) override
	{
		if(!Data) return;
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->Update(this, Data);
		Cmd->EndOneTime();
	}

	void UpdateSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) override
	{
		if(!Data) return;
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->UpdateSize(this, Data, UpdateByteSize);
		Cmd->EndOneTime();
	}

	void ReadBack(renderer_backend* Backend, void* Data) override
	{
		if(!Data) return;
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->ReadBack(this, Data);
		Cmd->EndOneTime();
	}

	void ReadBackSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) override
	{
		if(!Data) return;
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->ReadBackSize(this, Data, UpdateByteSize);
		Cmd->EndOneTime();
	}

	void CreateResource(renderer_backend* Backend, std::string DebugName, u64 NewSize, u64 Count, u32 Flags) override
	{
		Name = DebugName;
		Usage = Flags;
		Gfx = static_cast<vulkan_backend*>(Backend);
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
		//if(Flags & RF_CopySrc)
			CreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		//if(Flags & RF_CopyDst)
			CreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VmaAllocationCreateInfo AllocCreateInfo = {};
		AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		if(Flags & RF_CpuWrite)
		{
			AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		if(Flags & RF_CpuRead)
		{
			AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		VmaAllocationInfo AllocationInfo;
		VK_CHECK(vmaCreateBuffer(Gfx->AllocatorHandle, &CreateInfo, &AllocCreateInfo, &Handle, &Allocation, &AllocationInfo));

#ifdef CE_DEBUG
		std::string TempName = (DebugName + ".buffer");
		VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
		DebugNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		DebugNameInfo.objectHandle = (u64)Handle;
		DebugNameInfo.pObjectName = TempName.c_str();
		vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
#endif
	}

	void DestroyResource() override
	{
		if(Handle != VK_NULL_HANDLE)
		{
			if(Gfx && Allocation)
			{
				vmaDestroyBuffer(Gfx->AllocatorHandle, Handle, Allocation);
				Allocation = VK_NULL_HANDLE;
				Handle = VK_NULL_HANDLE;
			}
		}
		if(UpdateBuffer)
		{
			delete UpdateBuffer;
			UpdateBuffer = nullptr;
		}
		if(UploadBuffer)
		{
			delete UploadBuffer;
			UploadBuffer = nullptr;
		}

		Gfx = nullptr;
	}

	VkBuffer Handle = VK_NULL_HANDLE;
	VmaAllocation Allocation = VK_NULL_HANDLE;

private:
	VkDevice Device = VK_NULL_HANDLE;
	vulkan_backend* Gfx = nullptr;
};

struct vulkan_texture : public texture
{
	friend vulkan_command_list;
	vulkan_texture() = default;
	~vulkan_texture() override { DestroyStagingResource(); DestroyResource(); }

    vulkan_texture(const vulkan_texture&) = delete;
    vulkan_texture& operator=(const vulkan_texture&) = delete;

	vulkan_texture(renderer_backend* Backend, std::string DebugName, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const utils::texture::input_data& InputData)
	{
		CreateResource(Backend, DebugName, NewWidth, NewHeight, DepthOrArraySize, InputData);
		UpdateBuffer = new vulkan_buffer(Gfx, Name + ".update_staging_buffer", nullptr, Size, 1, RF_CpuWrite, true);
		UploadBuffer = new vulkan_buffer(Gfx, Name + ".upload_staging_buffer", nullptr, Size, 1, RF_CpuRead, true);
		if(Data) Update(Backend, Data);

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
		if(!Data) return;
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->Update(this, Data);
		Cmd->EndOneTime();
	}

	void ReadBack(renderer_backend* Backend, void* Data) override
	{
		if(!Data) return;
		std::unique_ptr<vulkan_command_list> Cmd = std::make_unique<vulkan_command_list>(Backend);
		Cmd->Begin();
		Cmd->ReadBack(this, Data);
		Cmd->EndOneTime();
	}

	void CreateResource(renderer_backend* Backend, std::string DebugName, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const utils::texture::input_data& InputData) override
	{
		Name = DebugName;
		Gfx = static_cast<vulkan_backend*>(Backend);

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
		if((InputData.Usage & image_flags::TF_CubeMap) || InputData.Type == image_type::Texture1D || InputData.Type == image_type::Texture2D)
		{
			CreateInfo.extent.depth = 1;
			CreateInfo.arrayLayers  = DepthOrArraySize;
		}
		else if(InputData.Type == image_type::Texture3D)
		{
			CreateInfo.extent.depth = DepthOrArraySize;
			CreateInfo.arrayLayers  = 1;
		}
		CreateInfo.mipLevels = InputData.MipLevels;
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
		//if(Info.Usage & image_flags::TF_CopySrc)
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
		VK_CHECK(vmaCreateImage(Gfx->AllocatorHandle, &CreateInfo, &AllocCreateInfo, &Handle, &Allocation, &AllocationInfo));
		Size = AllocationInfo.size;

#ifdef CE_DEBUG
		std::string TempName = (DebugName + ".texture");
		VkDebugUtilsObjectNameInfoEXT DebugNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
		DebugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		DebugNameInfo.objectHandle = (u64)Handle;
		DebugNameInfo.pObjectName = TempName.c_str();
		vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
#endif

		VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ViewCreateInfo.format = CreateInfo.format;
		ViewCreateInfo.image = Handle;
		ViewCreateInfo.subresourceRange.aspectMask = Aspect;
		ViewCreateInfo.subresourceRange.layerCount = InputData.Type != image_type::Texture3D ? DepthOrArraySize : 1;
		ViewCreateInfo.subresourceRange.levelCount = 1;

		if(InputData.Usage & image_flags::TF_CubeMap)
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		else if(InputData.Type == image_type::Texture1D)
			ViewCreateInfo.viewType = DepthOrArraySize > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
		else if(InputData.Type == image_type::Texture2D)
			ViewCreateInfo.viewType = DepthOrArraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		else if(InputData.Type == image_type::Texture3D)
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;

		for(u32 MipIdx = 0;
			MipIdx < InputData.MipLevels;
			++MipIdx)
		{
			ViewCreateInfo.subresourceRange.baseMipLevel = MipIdx;

			VkImageView Result = 0;
			VK_CHECK(vkCreateImageView(Device, &ViewCreateInfo, 0, &Result));
			Views.push_back(Result);

#ifdef CE_DEBUG
			std::string NewImageViewName = (Name + ".image_view #" + std::to_string(MipIdx));
			DebugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			DebugNameInfo.objectHandle = (u64)Result;
			DebugNameInfo.pObjectName = NewImageViewName.c_str();
			vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
#endif
		}
	}

	void DestroyResource() override
	{
		if(SamplerHandle != VK_NULL_HANDLE)
		{
			vkDestroySampler(Device, SamplerHandle, nullptr);
			SamplerHandle = VK_NULL_HANDLE;
		}
		for(VkImageView& View : Views)
		{
			vkDestroyImageView(Device, View, nullptr);
		}
		if(Handle != VK_NULL_HANDLE)
		{
			if(Gfx && Allocation)
			{
				vmaDestroyImage(Gfx->AllocatorHandle, Handle, Allocation);
				Allocation = VK_NULL_HANDLE;
			}
			Handle = VK_NULL_HANDLE;
		}

		Gfx = nullptr;
		Views.clear();
	}

	void DestroyStagingResource() override
	{
		if(UpdateBuffer)
		{
			delete UpdateBuffer;
			UpdateBuffer = nullptr;
		}
		if(UploadBuffer)
		{
			delete UploadBuffer;
			UploadBuffer = nullptr;
		}
	}

	VkImage Handle = VK_NULL_HANDLE;
	VkImageAspectFlags Aspect;
	std::vector<VkImageView> Views;

	VkSampler SamplerHandle = VK_NULL_HANDLE;
	VmaAllocation Allocation = VK_NULL_HANDLE;

private:
	VkDevice Device = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	vulkan_backend* Gfx = nullptr;
};
