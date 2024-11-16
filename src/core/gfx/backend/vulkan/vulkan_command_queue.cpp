
void vulkan_command_queue::
Init(VkDevice NewDevice, u32 NewFamilyIndex)
{
	Device = NewDevice;

	VkCommandPoolCreateInfo CommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CommandPoolCreateInfo.queueFamilyIndex = NewFamilyIndex;
	vkCreateCommandPool(Device, &CommandPoolCreateInfo, 0, &CommandAlloc);

	vkGetDeviceQueue(Device, NewFamilyIndex, 0, &Handle);
}

void vulkan_command_queue::
DestroyObject()
{
#if 0
	for(u32 Idx = 0;
		Idx < CommandLists.size();
		++Idx)
	{
		Execute(CommandLists[Idx], ReleaseSemaphore, AcquireSemaphore);
	}
#endif
	vkFreeCommandBuffers(Device, CommandAlloc, CommandLists.size(), CommandLists.data());
	CommandLists.clear();
	if(CommandAlloc)
	{
		vkDestroyCommandPool(Device, CommandAlloc, nullptr);
		CommandAlloc = 0;
	}
}

VkCommandBuffer vulkan_command_queue::
AllocateCommandList()
{
	VkCommandBuffer Result;

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	CommandBufferAllocateInfo.commandBufferCount = 1;
	CommandBufferAllocateInfo.commandPool = CommandAlloc;
	CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, &Result);
	CommandLists.push_back(Result);

	VkCommandBufferBeginInfo CommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(Result, &CommandBufferBeginInfo);

	return Result;
}

void vulkan_command_queue::
Reset()
{
	vkResetCommandPool(Device, CommandAlloc, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

void vulkan_command_queue::
Reset(VkCommandBuffer* CommandList)
{
	vkResetCommandBuffer(*CommandList, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	vkResetCommandPool(Device, CommandAlloc, 0);

	VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(*CommandList, &CommandBufferBeginInfo);
}

void vulkan_command_queue::
Execute()
{
	for(VkCommandBuffer CommandList : CommandLists)
	{
		VK_CHECK(vkEndCommandBuffer(CommandList));
	}

	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.commandBufferCount = CommandLists.size();
	SubmitInfo.pCommandBuffers = CommandLists.data();
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(Handle);
}

void vulkan_command_queue::
Execute(VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore)
{
	for(VkCommandBuffer CommandList : CommandLists)
	{
		vkEndCommandBuffer(CommandList);
	}

	VkPipelineStageFlags SubmitStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.pWaitDstStageMask = &SubmitStageFlag;
	SubmitInfo.commandBufferCount = CommandLists.size();
	SubmitInfo.pCommandBuffers = CommandLists.data();
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = AcquireSemaphore;
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = ReleaseSemaphore;
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(Handle);
}

void vulkan_command_queue::
Execute(VkCommandBuffer* CommandList)
{
	VK_CHECK(vkEndCommandBuffer(*CommandList));

	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = CommandList;
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
	vkQueueWaitIdle(Handle);
}

void vulkan_command_queue::
Execute(VkCommandBuffer* CommandList, VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore)
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

void vulkan_command_queue::
ExecuteAndRemove(VkCommandBuffer* CommandList)
{
	Execute(CommandList);
	vkFreeCommandBuffers(Device, CommandAlloc, 1, CommandList);
	CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), *CommandList), CommandLists.end());
	*CommandList = VK_NULL_HANDLE;
}

void vulkan_command_queue::
ExecuteAndRemove(VkCommandBuffer* CommandList, VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore)
{
	Execute(CommandList, ReleaseSemaphore, AcquireSemaphore);
	vkFreeCommandBuffers(Device, CommandAlloc, 1, CommandList);
	CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), *CommandList), CommandLists.end());
	*CommandList = VK_NULL_HANDLE;
}
