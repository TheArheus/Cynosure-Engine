#pragma once


class vulkan_command_queue : public command_queue
{
	VkFence SubmitFence;
	VkDevice Device;
	VkCommandPool CommandAlloc;

public:
	VkQueue Handle;

	VkSemaphore AcquireSemaphore;
	VkSemaphore ReleaseSemaphore;

	vulkan_command_queue() = default;
	~vulkan_command_queue() override { DestroyObject(); };

	vulkan_command_queue(renderer_backend* Backend, u32 NewFamilyIndex, u32 QueueIndex = 0)
	{
		Init(Backend, NewFamilyIndex, QueueIndex);
	}

	void Init(renderer_backend* Backend, u32 NewFamilyIndex, u32 QueueIndex);

	void DestroyObject() override;
	void Reset() override;

	command_list* AllocateCommandList(command_list_level Level = command_list_level::primary) override;
	void Remove(command_list* CommandList) override;

	void Execute(const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>()) override;
	void Present(const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>()) override;

	void Execute(command_list* CommandList, const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>()) override;
	void Present(command_list* CommandList, const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>()) override;

	void ExecuteAndRemove(command_list* CommandList, const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>()) override;
};

struct vulkan_backend;
struct vulkan_gpu_sync : gpu_sync
{
	friend vulkan_backend;
	friend vulkan_command_queue;

public:
	vulkan_gpu_sync(renderer_backend* Gfx);
	~vulkan_gpu_sync() override;

	void CreateObject() override;
	void DestroyObject() override;

	void Signal(command_queue*) override;
	void Reset() override;

private:
	VkSemaphore Handle = 0;
	VkDevice Device;
};

class vulkan_queue_manager
{
	std::unordered_map<u32, std::vector<command_queue*>> Queues;

public:
	vulkan_queue_manager(std::vector<VkDeviceQueueCreateInfo> DeviceQueuesCreateInfo)
	{
	}

	vulkan_command_queue* GetQueue()
	{
	}
};

