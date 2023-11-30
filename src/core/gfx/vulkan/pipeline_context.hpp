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

static VkShaderModule
LoadShaderModule(VkDevice Device, const char* Path)
{
	VkShaderModule Result = 0;
	FILE* File = fopen(Path, "rb");
	if(File)
	{
		fseek(File, 0, SEEK_END);
		long FileLength = ftell(File);
		fseek(File, 0, SEEK_SET);

		char* Buffer = (char*)malloc(FileLength);
		assert(Buffer);

		size_t ReadSize = fread(Buffer, 1, FileLength, File);
		assert(ReadSize == size_t(FileLength));
		assert(FileLength % 4 == 0);

		VkShaderModuleCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		CreateInfo.codeSize = FileLength;
		CreateInfo.pCode = reinterpret_cast<const u32*>(Buffer);

		VK_CHECK(vkCreateShaderModule(Device, &CreateInfo, 0, &Result));

		fclose(File);
	}
	return Result;
}

// TODO: Move CommandQueue to this struct
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

	template<typename T>
	global_pipeline_context(std::unique_ptr<T>& Context)
	{
		CreateResource(Context);
	}

	template<typename T>
	void CreateResource(std::unique_ptr<T>& Context)
	{
		Device = Context->Device;

		VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VK_CHECK(vkCreateSemaphore(Context->Device, &SemaphoreCreateInfo, nullptr, &AcquireSemaphore));
		VK_CHECK(vkCreateSemaphore(Context->Device, &SemaphoreCreateInfo, nullptr, &ReleaseSemaphore));

		CommandList = Context->CommandQueue.AllocateCommandList();
	}

	template<typename T>
	void Begin(std::unique_ptr<T>& Context)
	{
		Context->CommandQueue.Reset(CommandList);

		vkAcquireNextImageKHR(Context->Device, Context->Swapchain, ~0ull, AcquireSemaphore, VK_NULL_HANDLE, &BackBufferIndex);
	}

	template<typename T>
	void End(std::unique_ptr<T>& Context)
	{
		Context->CommandQueue.Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
	}

	template<typename T>
	void DeviceWaitIdle(std::unique_ptr<T>& Context)
	{
		vkDeviceWaitIdle(Context->Device);
	}

	template<typename T>
	void EndOneTime(std::unique_ptr<T>& Context)
	{
		Context->CommandQueue.ExecuteAndRemove(CommandList, &ReleaseSemaphore, &AcquireSemaphore);
	}

	template<typename T>
	void EmplaceColorTarget(std::unique_ptr<T>& Context, texture& Texture)
	{
		std::vector<VkImageMemoryBarrier> ImageCopyBarriers = 
		{
			CreateImageBarrier(Texture.Handle, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
			CreateImageBarrier(Context->SwapchainImages[BackBufferIndex], 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
		};
		ImageBarrier(*CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, ImageCopyBarriers);

		VkImageCopy ImageCopyRegion = {};
		ImageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageCopyRegion.srcSubresource.layerCount = 1;
		ImageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageCopyRegion.dstSubresource.layerCount = 1;
		ImageCopyRegion.extent = {u32(Texture.Width), u32(Texture.Height), u32(Texture.Depth)};

		vkCmdCopyImage(*CommandList, Texture.Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Context->SwapchainImages[BackBufferIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageCopyRegion);
	}

	template<typename T>
	void Present(std::unique_ptr<T>& Context)
	{
		std::vector<VkImageMemoryBarrier> ImageEndRenderBarriers = 
		{
			CreateImageBarrier(Context->SwapchainImages[BackBufferIndex], VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		};
		ImageBarrier(*CommandList, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ImageEndRenderBarriers);

		Context->CommandQueue.Execute(CommandList, &ReleaseSemaphore, &AcquireSemaphore);

		// NOTE: It shouldn't be there
		VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores = &ReleaseSemaphore;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &Context->Swapchain;
		PresentInfo.pImageIndices = &BackBufferIndex;
		vkQueuePresentKHR(Context->CommandQueue.Handle, &PresentInfo);
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

	void SetMemoryBarrier(const std::tuple<VkAccessFlags, VkAccessFlags>& BarrierData, 
						  VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		VkMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
		Barrier.srcAccessMask = std::get<0>(BarrierData);
		Barrier.dstAccessMask = std::get<1>(BarrierData);

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

	void SetImageBarrier(const std::tuple<texture&, VkAccessFlags, VkAccessFlags, VkImageLayout, VkImageLayout, VkImageAspectFlags>& BarrierData, 
						 VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		Barrier.image = std::get<0>(BarrierData).Handle;
		Barrier.srcAccessMask = std::get<1>(BarrierData);
		Barrier.dstAccessMask = std::get<2>(BarrierData);
		Barrier.oldLayout = std::get<3>(BarrierData);
		Barrier.newLayout = std::get<4>(BarrierData);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.subresourceRange.aspectMask = std::get<5>(BarrierData);
		Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		//std::get<0>(Data).Layout.Access      = std::get<1>(Data);
		//std::get<0>(Data).Layout.ImageLayout = std::get<2>(Data);

		vkCmdPipelineBarrier(*CommandList, SrcStageMask, DstStageMask, 
							 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 
							 1, &Barrier);
	}

	void SetImageBarrier(const std::tuple<VkImage&, VkAccessFlags, VkAccessFlags, VkImageLayout, VkImageLayout, VkImageAspectFlags>& BarrierData,
						 VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		Barrier.image = std::get<0>(BarrierData);
		Barrier.srcAccessMask = std::get<1>(BarrierData);
		Barrier.dstAccessMask = std::get<2>(BarrierData);
		Barrier.oldLayout = std::get<3>(BarrierData);
		Barrier.newLayout = std::get<4>(BarrierData);
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		Barrier.subresourceRange.aspectMask = std::get<5>(BarrierData);
		Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		vkCmdPipelineBarrier(*CommandList, SrcStageMask, DstStageMask, 
							 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 
							 1, &Barrier);
	}

	void SetImageBarriers(const std::vector<std::tuple<texture&, VkAccessFlags, VkAccessFlags, VkImageLayout, VkImageLayout, VkImageAspectFlags>>& TextureBarrierData, 
						  const std::vector<std::tuple<VkImage&, VkAccessFlags, VkAccessFlags, VkImageLayout, VkImageLayout, VkImageAspectFlags>>& ImageBarrierData,
						  VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask)
	{
		std::vector<VkImageMemoryBarrier> Barriers;
		for(const std::tuple<texture&, VkAccessFlags, VkAccessFlags, VkImageLayout, VkImageLayout, VkImageAspectFlags>& Data : TextureBarrierData)
		{
			VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			Barrier.image = std::get<0>(Data).Handle;
			Barrier.srcAccessMask = std::get<1>(Data);
			Barrier.dstAccessMask = std::get<2>(Data);
			Barrier.oldLayout = std::get<3>(Data);
			Barrier.newLayout = std::get<4>(Data);
			Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.subresourceRange.aspectMask = std::get<5>(Data);
			Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			//std::get<0>(Data).Layout.Access      = std::get<1>(Data);
			//std::get<0>(Data).Layout.ImageLayout = std::get<2>(Data);

			Barriers.push_back(Barrier);
		}
		for(const std::tuple<VkImage&, VkAccessFlags, VkAccessFlags, VkImageLayout, VkImageLayout, VkImageAspectFlags>& Data : ImageBarrierData)
		{
			VkImageMemoryBarrier Barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			Barrier.image = std::get<0>(Data);
			Barrier.srcAccessMask = std::get<1>(Data);
			Barrier.dstAccessMask = std::get<2>(Data);
			Barrier.oldLayout = std::get<3>(Data);
			Barrier.newLayout = std::get<4>(Data);
			Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.subresourceRange.aspectMask = std::get<5>(Data);
			Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			//std::get<0>(Data).Layout.Access      = std::get<1>(Data);
			//std::get<0>(Data).Layout.ImageLayout = std::get<2>(Data);

			Barriers.push_back(Barrier);
		}

		vkCmdPipelineBarrier(*CommandList, SrcStageMask, DstStageMask, 
							 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 
							 (u32)Barriers.size(), Barriers.data());
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

	template<typename T>
	render_context(std::unique_ptr<T>& Context,
				   const shader_input& Signature,
				   std::initializer_list<const std::string> ShaderList, const std::vector<VkFormat>& ColorTargetFormats, const input_data& InputData = {true, true, true, false, false, 0}) 
				 : InputSignature(&Signature), UseColorTarget(InputData.UseColor), UseDepthTarget(InputData.UseDepth)
	{		
		RenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
		Device = Context->Device;

		for(const std::string Shader : ShaderList)
		{
			VkPipelineShaderStageCreateInfo Stage = {};
			Stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			if(Shader.find(".vert.") != std::string::npos)
			{
				Stage.module = LoadShaderModule(Device, Shader.c_str());
				Stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
				Stage.pName = "main";
				ShaderStages.push_back(Stage);
			}
			if(Shader.find(".doma.") != std::string::npos)
			{
				Stage.module = LoadShaderModule(Device, Shader.c_str());
				Stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				Stage.pName = "main";
				ShaderStages.push_back(Stage);
			}
			if(Shader.find(".hull.") != std::string::npos)
			{
				Stage.module = LoadShaderModule(Device, Shader.c_str());
				Stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				Stage.pName = "main";
				ShaderStages.push_back(Stage);
			}
			if (Shader.find(".geom.") != std::string::npos)
			{
				Stage.module = LoadShaderModule(Device, Shader.c_str());
				Stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				Stage.pName = "main";
				ShaderStages.push_back(Stage);
			}
			if(Shader.find(".frag.") != std::string::npos)
			{
				Stage.module = LoadShaderModule(Device, Shader.c_str());
				Stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				Stage.pName = "main";
				ShaderStages.push_back(Stage);
			}
		}

		Pipeline = Context->CreateGraphicsPipeline(Signature.Handle, ShaderStages, ColorTargetFormats, InputData.UseColor, InputData.UseDepth, InputData.UseBackFace, InputData.UseOutline, InputData.ViewMask, InputData.UseMultiview);
	}

	render_context(const render_context&) = delete;
	render_context& operator=(const render_context&) = delete;

    render_context(render_context&& other) noexcept : 
		ColorTargets(std::move(other.ColorTargets)),
		DepthTarget(std::move(other.DepthTarget)),
		UseColorTarget(std::move(other.UseColorTarget)),
		UseDepthTarget(std::move(other.UseDepthTarget)),
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
			ColorTargets = std::move(other.ColorTargets);
			DepthTarget = std::move(other.DepthTarget);
			UseColorTarget = std::move(other.UseColorTarget);
			UseDepthTarget = std::move(other.UseDepthTarget);
			GlobalOffset = std::move(other.GlobalOffset);
			PushConstantIdx = std::move(other.PushConstantIdx);
			RenderingInfo = std::move(other.RenderingInfo);
			SetIndices = std::move(other.SetIndices);
			ShaderStages = std::move(other.ShaderStages);
			PushDescriptorBindings = std::move(other.PushDescriptorBindings);
			StaticDescriptorBindings = std::move(other.StaticDescriptorBindings);
			BufferInfos = std::move(other.BufferInfos);
			BufferArrayInfos = std::move(other.BufferArrayInfos);
			RenderingAttachmentInfos = std::move(other.RenderingAttachmentInfos);
			RenderingAttachmentInfoArrays = std::move(other.RenderingAttachmentInfoArrays);
			InputSignature = other.InputSignature;
			PipelineContext = std::move(other.PipelineContext);
			Device = std::move(other.Device);
			Pipeline = std::move(other.Pipeline);
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

	template<typename T>
	void Begin(std::unique_ptr<T>& Context, const global_pipeline_context& GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight)
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

	bool UseColorTarget;
	bool UseDepthTarget;

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

	template<class T>
	compute_context(std::unique_ptr<T>& Context, const shader_input& Signature, const std::string& Shader) :
		InputSignature(&Signature)
	{
		Device = Context->Device;

		ComputeStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		ComputeStage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
		ComputeStage.module = LoadShaderModule(Device, Shader.c_str());
		ComputeStage.pName  = "main";

		Pipeline = Context->CreateComputePipeline(Signature.Handle, ComputeStage);
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
			ComputeStage = std::move(other.ComputeStage);

			GlobalOffset = std::move(other.GlobalOffset);
			PushConstantIdx = std::move(other.PushConstantIdx);

			PushDescriptorBindings = std::move(other.PushDescriptorBindings);
			StaticDescriptorBindings = std::move(other.StaticDescriptorBindings);
			BufferInfos = std::move(other.BufferInfos);
			BufferArrayInfos = std::move(other.BufferArrayInfos);

			SetIndices = std::move(other.SetIndices);

			InputSignature = other.InputSignature;
			other.InputSignature = nullptr;
			PipelineContext = std::move(other.PipelineContext);

			Device = std::move(other.Device);
			Pipeline = std::move(other.Pipeline);
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
