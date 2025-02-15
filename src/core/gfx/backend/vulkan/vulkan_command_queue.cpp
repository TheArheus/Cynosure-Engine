
vulkan_gpu_sync::
vulkan_gpu_sync(renderer_backend* Gfx)
{
	Device = static_cast<vulkan_backend*>(Gfx)->Device;
	CreateObject();
}

vulkan_gpu_sync::
~vulkan_gpu_sync()
{
	DestroyObject();
}

void vulkan_gpu_sync::
CreateObject()
{
	CurrentWaitValue = 0;
	VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VkSemaphoreTypeCreateInfoKHR SemaphoreTypeInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR};
	SemaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
	SemaphoreTypeInfo.initialValue  = 0;
	SemaphoreCreateInfo.pNext = &SemaphoreTypeInfo;
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &Handle));
}

void vulkan_gpu_sync::
DestroyObject()
{
	if(Handle)
	{
		VkSemaphoreWaitInfo WaitInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
		WaitInfo.semaphoreCount = 1;
		WaitInfo.pSemaphores = &Handle;
		WaitInfo.pValues = &CurrentWaitValue;
		VK_CHECK(vkWaitSemaphoresKHR(Device, &WaitInfo, UINT64_MAX));

		vkDestroySemaphore(Device, Handle, nullptr);
		Handle = 0;
	}
}

void vulkan_gpu_sync::
Reset()
{
	DestroyObject();
	CreateObject();
}

void vulkan_gpu_sync::
Signal(command_queue* QueueSignal)
{
	//if(CurrentWaitValue > UINT64_MAX - 1) Reset();
	CurrentWaitValue++;
	VkSemaphoreSignalInfo SignalInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};
	SignalInfo.semaphore = Handle;
	SignalInfo.value     = CurrentWaitValue;
	VK_CHECK(vkSignalSemaphoreKHR(Device, &SignalInfo));
}



void vulkan_command_queue::
Init(renderer_backend* Backend, u32 NewFamilyIndex, u32 QueueIndex)
{
	Gfx = Backend;
	Device = static_cast<vulkan_backend*>(Backend)->Device;

	VkCommandPoolCreateInfo CommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CommandPoolCreateInfo.queueFamilyIndex = NewFamilyIndex;
	vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandAlloc);

	vkGetDeviceQueue(Device, NewFamilyIndex, QueueIndex, &Handle);

	VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &AcquireSemaphore));
	VK_CHECK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ReleaseSemaphore));
}

void vulkan_command_queue::
DestroyObject()
{
	vkDeviceWaitIdle(Device);
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

	std::vector<VkCommandBuffer> CommitLists;
	for(command_list* CommandList : CommandLists)
	{
		vulkan_command_list* Cmd = static_cast<vulkan_command_list*>(CommandList);
		if(CommandList->IsRunning) VK_CHECK(vkEndCommandBuffer(Cmd->Handle));
		CommitLists.push_back(Cmd->Handle);
		delete CommandList;
	}
	if(CommitLists.size()) vkFreeCommandBuffers(Device, CommandAlloc, CommitLists.size(), CommitLists.data());
	CommandLists.clear();
	if(CommandAlloc)
	{
		vkDestroyCommandPool(Device, CommandAlloc, nullptr);
		CommandAlloc = 0;
	}
}

command_list* vulkan_command_queue::
AllocateCommandList(command_list_level Level)
{
	vulkan_command_list* NewCommandList = new vulkan_command_list;
	NewCommandList->Gfx = static_cast<vulkan_backend*>(Gfx);
	NewCommandList->Device = Device;

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	CommandBufferAllocateInfo.commandBufferCount = 1;
	CommandBufferAllocateInfo.commandPool = CommandAlloc;
	CommandBufferAllocateInfo.level = GetVKCommandListLevel(Level);
	vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, &NewCommandList->Handle);
	CommandLists.push_back(NewCommandList);

	return NewCommandList;
}

void vulkan_command_queue::
Reset()
{
	vkResetCommandPool(Device, CommandAlloc, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

void vulkan_command_queue::
Remove(command_list* CommandList)
{
	if(CommandList == nullptr) return;
	vkFreeCommandBuffers(Device, CommandAlloc, 1, &static_cast<vulkan_command_list*>(CommandList)->Handle);
	delete CommandList;
	CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
}

void vulkan_command_queue::
Execute(const std::vector<gpu_sync*>& Syncs)
{
	std::vector<VkCommandBuffer> CommitLists;
	for(command_list* CommandList : CommandLists)
	{
		CommandList->PlaceEndOfFrameBarriers();
		CommandList->IsRunning = false;

		vulkan_command_list* Cmd = static_cast<vulkan_command_list*>(CommandList);
		VK_CHECK(vkEndCommandBuffer(Cmd->Handle));
		CommitLists.push_back(Cmd->Handle);

		CommandList->PrevContext = nullptr;
		CommandList->CurrContext = nullptr;
	}

	std::vector<u64> WaitValues;
	std::vector<u64> SignalValues;
	std::vector<VkSemaphore> WaitSemaphores;
	std::vector<VkSemaphore> SignalSemaphores;
	for(gpu_sync* Sync : Syncs)
	{
		WaitValues.push_back(Sync->CurrentWaitValue);
		SignalValues.push_back(++Sync->CurrentWaitValue);
		WaitSemaphores.push_back(static_cast<vulkan_gpu_sync*>(Sync)->Handle);
	}
	SignalSemaphores = WaitSemaphores;
	WaitSemaphores.push_back(AcquireSemaphore);
	SignalSemaphores.push_back(ReleaseSemaphore);

	VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
	TimelineSubmitInfo.waitSemaphoreValueCount = WaitValues.size();
	TimelineSubmitInfo.pWaitSemaphoreValues = WaitValues.data();
	TimelineSubmitInfo.signalSemaphoreValueCount = SignalValues.size();
	TimelineSubmitInfo.pSignalSemaphoreValues = SignalValues.data();

	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.pNext = &TimelineSubmitInfo;
	SubmitInfo.commandBufferCount = CommitLists.size();
	SubmitInfo.pCommandBuffers = CommitLists.data();
	if(Syncs.size())
	{
		SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
		SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
		SubmitInfo.signalSemaphoreCount = SignalSemaphores.size();
		SubmitInfo.pSignalSemaphores = SignalSemaphores.data();
	}
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));	
	if(!Syncs.size()) vkQueueWaitIdle(Handle);
}

void vulkan_command_queue::
Execute(command_list* CommandList, const std::vector<gpu_sync*>& Syncs)
{
	CommandList->PlaceEndOfFrameBarriers();
	CommandList->IsRunning = false;

	VkCommandBuffer Cmd = static_cast<vulkan_command_list*>(CommandList)->Handle;
	VK_CHECK(vkEndCommandBuffer(Cmd));

	std::vector<u64> WaitValues;
	std::vector<u64> SignalValues;
	std::vector<VkSemaphore> WaitSemaphores;
	std::vector<VkSemaphore> SignalSemaphores;
	for(gpu_sync* Sync : Syncs)
	{
		WaitValues.push_back(Sync->CurrentWaitValue);
		SignalValues.push_back(++Sync->CurrentWaitValue);
		WaitSemaphores.push_back(static_cast<vulkan_gpu_sync*>(Sync)->Handle);
	}
	SignalSemaphores = WaitSemaphores;
	WaitSemaphores.push_back(AcquireSemaphore);
	SignalSemaphores.push_back(ReleaseSemaphore);

	VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
	TimelineSubmitInfo.waitSemaphoreValueCount = WaitValues.size();
	TimelineSubmitInfo.pWaitSemaphoreValues = WaitValues.data();
	TimelineSubmitInfo.signalSemaphoreValueCount = SignalValues.size();
	TimelineSubmitInfo.pSignalSemaphoreValues = SignalValues.data();

	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.pNext = &TimelineSubmitInfo;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &Cmd;
	if(Syncs.size())
	{
		SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
		SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
		SubmitInfo.signalSemaphoreCount = SignalSemaphores.size();
		SubmitInfo.pSignalSemaphores = SignalSemaphores.data();
	}
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));
	if(!Syncs.size()) vkQueueWaitIdle(Handle);

	CommandList->PrevContext = nullptr;
	CommandList->CurrContext = nullptr;
}

void vulkan_command_queue::
Present(const std::vector<gpu_sync*>& Syncs)
{
	VkPipelineStageFlags SubmitStageFlag = 0;
	std::vector<VkCommandBuffer> CommitLists;
	for(command_list* CommandList : CommandLists)
	{
		CommandList->PlaceEndOfFrameBarriers();
		CommandList->IsRunning = false;

		vulkan_command_list* Cmd = static_cast<vulkan_command_list*>(CommandList);
		VK_CHECK(vkEndCommandBuffer(Cmd->Handle));
		CommitLists.push_back(Cmd->Handle);

		SubmitStageFlag |= Cmd->CurrentStage;
		CommandList->PrevContext = nullptr;
		CommandList->CurrContext = nullptr;
	}

	std::vector<u64> WaitValues;
	std::vector<u64> SignalValues;
	std::vector<VkSemaphore> WaitSemaphores;
	std::vector<VkSemaphore> SignalSemaphores;
	std::vector<VkPipelineStageFlags> FinalPipelineStageFlags;
	for(gpu_sync* Sync : Syncs)
	{
		FinalPipelineStageFlags.push_back(SubmitStageFlag);
		WaitValues.push_back(Sync->CurrentWaitValue);
		SignalValues.push_back(++Sync->CurrentWaitValue);
		WaitSemaphores.push_back(static_cast<vulkan_gpu_sync*>(Sync)->Handle);
	}
	SignalSemaphores = WaitSemaphores;
	WaitValues.push_back(0);
	SignalValues.push_back(0);
	WaitSemaphores.push_back(AcquireSemaphore);
	SignalSemaphores.push_back(ReleaseSemaphore);

	VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
	TimelineSubmitInfo.waitSemaphoreValueCount = WaitValues.size();
	TimelineSubmitInfo.pWaitSemaphoreValues = WaitValues.data();
	TimelineSubmitInfo.signalSemaphoreValueCount = SignalValues.size();
	TimelineSubmitInfo.pSignalSemaphoreValues = SignalValues.data();

	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.pNext = &TimelineSubmitInfo;
	SubmitInfo.pWaitDstStageMask = FinalPipelineStageFlags.data();
	SubmitInfo.commandBufferCount = CommitLists.size();
	SubmitInfo.pCommandBuffers = CommitLists.data();
	SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
	SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
	SubmitInfo.signalSemaphoreCount = SignalSemaphores.size();
	SubmitInfo.pSignalSemaphores = SignalSemaphores.data();
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = &ReleaseSemaphore;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &static_cast<vulkan_backend*>(Gfx)->Swapchain;
	PresentInfo.pImageIndices = &Gfx->BackBufferIndex;
	vkQueuePresentKHR(Handle, &PresentInfo);
}

void vulkan_command_queue::
Present(command_list* CommandList, const std::vector<gpu_sync*>& Syncs)
{
	CommandList->PlaceEndOfFrameBarriers();
	CommandList->IsRunning = false;

	VkCommandBuffer Cmd = static_cast<vulkan_command_list*>(CommandList)->Handle;
	VK_CHECK(vkEndCommandBuffer(Cmd));

	std::vector<u64> WaitValues;
	std::vector<u64> SignalValues;
	std::vector<VkSemaphore> WaitSemaphores;
	std::vector<VkSemaphore> SignalSemaphores;
	std::vector<VkPipelineStageFlags> FinalPipelineStageFlags;
	for(gpu_sync* Sync : Syncs)
	{
		FinalPipelineStageFlags.push_back(static_cast<vulkan_command_list*>(CommandList)->CurrentStage);
		WaitValues.push_back(Sync->CurrentWaitValue);
		SignalValues.push_back(++Sync->CurrentWaitValue);
		WaitSemaphores.push_back(static_cast<vulkan_gpu_sync*>(Sync)->Handle);
	}
	SignalSemaphores = WaitSemaphores;
	WaitValues.push_back(0);
	SignalValues.push_back(0);
	FinalPipelineStageFlags.push_back(static_cast<vulkan_command_list*>(CommandList)->CurrentStage);
	WaitSemaphores.push_back(AcquireSemaphore);
	SignalSemaphores.push_back(ReleaseSemaphore);

	VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
	TimelineSubmitInfo.waitSemaphoreValueCount = WaitValues.size();
	TimelineSubmitInfo.pWaitSemaphoreValues = WaitValues.data();
	TimelineSubmitInfo.signalSemaphoreValueCount = SignalValues.size();
	TimelineSubmitInfo.pSignalSemaphoreValues = SignalValues.data();

	VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
	SubmitInfo.pNext = &TimelineSubmitInfo;
	SubmitInfo.pWaitDstStageMask = FinalPipelineStageFlags.data();
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &Cmd;
	SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
	SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
	SubmitInfo.signalSemaphoreCount = SignalSemaphores.size();
	SubmitInfo.pSignalSemaphores = SignalSemaphores.data();
	VK_CHECK(vkQueueSubmit(Handle, 1, &SubmitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = &ReleaseSemaphore;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &static_cast<vulkan_backend*>(Gfx)->Swapchain;
	PresentInfo.pImageIndices = &Gfx->BackBufferIndex;
	vkQueuePresentKHR(Handle, &PresentInfo);

	CommandList->PrevContext = nullptr;
	CommandList->CurrContext = nullptr;
}

void vulkan_command_queue::
ExecuteAndRemove(command_list* CommandList, const std::vector<gpu_sync*>& Syncs)
{
	Execute(CommandList, Syncs);
	Remove(CommandList);
}
