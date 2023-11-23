
class command_queue
{
	VkDevice Device;
	std::vector<VkCommandBuffer*> CommandLists;

public:
	VkQueue Handle;
	VkCommandPool CommandAlloc;

	command_queue() = default;

	template<class T>
	command_queue(T* Gfx)
	{
		Init(Gfx);
	}

	void DestroyObject()
	{
#if 0
		for(u32 Idx = 0;
			Idx < CommandLists.size();
			++Idx)
		{
			VkCommandBuffer* CommandList = CommandLists[Idx];
			Execute(CommandList, ReleaseSemaphore, AcquireSemaphore);
			CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
			delete CommandList;
		}
#endif
		if(CommandAlloc)
		{
			vkDestroyCommandPool(Device, CommandAlloc, nullptr);
			CommandAlloc = 0;
		}
	}


	template<class T>
	void Init(T* Gfx)
	{
		Device = Gfx->Device;

		VkCommandPoolCreateInfo CommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		CommandPoolCreateInfo.queueFamilyIndex = Gfx->FamilyIndex;
		vkCreateCommandPool(Device, &CommandPoolCreateInfo, 0, &CommandAlloc);

		vkGetDeviceQueue(Device, Gfx->FamilyIndex, 0, &Handle);
	}

	VkCommandBuffer* AllocateCommandList()
	{
		VkCommandBuffer* Result = new VkCommandBuffer;

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		CommandBufferAllocateInfo.commandBufferCount = 1;
		CommandBufferAllocateInfo.commandPool = CommandAlloc;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, Result);
		CommandLists.push_back(Result);

		VkCommandBufferBeginInfo CommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(*Result, &CommandBufferBeginInfo);

		return Result;
	}

	void Reset()
	{
		vkResetCommandPool(Device, CommandAlloc, 0);
	}

	void Reset(VkCommandBuffer* CommandList)
	{
		vkResetCommandBuffer(*CommandList, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		vkResetCommandPool(Device, CommandAlloc, 0);

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(*CommandList, &CommandBufferBeginInfo);
	}

	void Execute()
	{
		for(VkCommandBuffer* CommandList : CommandLists)
		{
			VK_CHECK(vkEndCommandBuffer(*CommandList));
		}

		std::vector<VkCommandBuffer> SubmitCommandLists;
		std::transform(CommandLists.begin(), CommandLists.end(), SubmitCommandLists.begin(), [](VkCommandBuffer* CommandList){return *CommandList;});

		VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		SubmitInfo.commandBufferCount = SubmitCommandLists.size();
		SubmitInfo.pCommandBuffers = SubmitCommandLists.data();
		VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
		vkQueueWaitIdle(Handle);
	}

	void Execute(VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore)
	{
		for(VkCommandBuffer* CommandList : CommandLists)
		{
			vkEndCommandBuffer(*CommandList);
		}

		std::vector<VkCommandBuffer> SubmitCommandLists;
		std::transform(CommandLists.begin(), CommandLists.end(), SubmitCommandLists.begin(), [](VkCommandBuffer* CommandList){return *CommandList;});

		VkPipelineStageFlags SubmitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		SubmitInfo.pWaitDstStageMask = &SubmitStageFlag;
		SubmitInfo.commandBufferCount = SubmitCommandLists.size();
		SubmitInfo.pCommandBuffers = SubmitCommandLists.data();
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = AcquireSemaphore;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = ReleaseSemaphore;
		VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
		vkQueueWaitIdle(Handle);
	}

	void Execute(VkCommandBuffer* CommandList)
	{
		VK_CHECK(vkEndCommandBuffer(*CommandList));

		VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = CommandList;
		VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
		vkQueueWaitIdle(Handle);
	}

	void Execute(VkCommandBuffer* CommandList, VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore)
	{
		VK_CHECK(vkEndCommandBuffer(*CommandList));

		VkPipelineStageFlags SubmitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		SubmitInfo.pWaitDstStageMask = &SubmitStageFlag;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = CommandList;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = AcquireSemaphore;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = ReleaseSemaphore;
		VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
		vkQueueWaitIdle(Handle);
	}

	void ExecuteAndRemove(VkCommandBuffer* CommandList)
	{
		Execute(CommandList);
		CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
		delete CommandList;
	}

	void ExecuteAndRemove(VkCommandBuffer* CommandList, VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore)
	{
		Execute(CommandList, ReleaseSemaphore, AcquireSemaphore);
		CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
		delete CommandList;
	}
};
