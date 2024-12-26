#pragma once

// TODO: better command lists allocation
class vulkan_command_queue
{
	std::vector<VkCommandBuffer> CommandLists;

public:
	VkQueue Handle;
	VkCommandPool CommandAlloc;
	VkDevice Device;

	vulkan_command_queue() = default;
	~vulkan_command_queue() { DestroyObject(); };

	vulkan_command_queue(VkDevice NewDevice, u32 NewFamilyIndex)
	{
		Init(NewDevice, NewFamilyIndex);
	}

	void Init(VkDevice NewDevice, u32 NewFamilyIndex);

	void DestroyObject();

	VkCommandBuffer AllocateCommandList();

	void Reset();
	void Reset(VkCommandBuffer* CommandList);

	void Remove(VkCommandBuffer* CommandList);

	void Execute();
	void Execute(VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore, VkFence* Fence = nullptr);
	void Execute(VkCommandBuffer* CommandList);
	void Execute(VkCommandBuffer* CommandList, VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore, VkFence* Fence = nullptr);

	void ExecuteAndRemove(VkCommandBuffer* CommandList);
	void ExecuteAndRemove(VkCommandBuffer* CommandList, VkSemaphore* ReleaseSemaphore, VkSemaphore* AcquireSemaphore);
};
