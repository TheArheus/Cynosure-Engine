#pragma once


class directx12_command_queue : public command_queue
{
	D3D12_COMMAND_LIST_TYPE Type;
	ID3D12Device6* Device = nullptr;

public:
	ComPtr<ID3D12CommandQueue> Handle;
	ComPtr<ID3D12CommandAllocator> CommandAlloc;

	directx12_command_queue() = default;
	~directx12_command_queue() override { DestroyObject(); };

	directx12_command_queue(renderer_backend* Backend, D3D12_COMMAND_LIST_TYPE NewType)
	{
		Init(Backend, NewType);
	}

	void Init(renderer_backend* Backend, D3D12_COMMAND_LIST_TYPE NewType);

	void DestroyObject() override;
	void Reset() override;

	command_list* AllocateCommandList(command_list_level Level = command_list_level::primary) override;
	void Remove(command_list* CommandList) override;

	void Execute(const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>(), bool PlaceEndBarriers = true) override;
	void Present(const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>(), bool PlaceEndBarriers = true) override;

	void Execute(command_list* CommandList, const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>(), bool PlaceEndBarriers = true) override;
	void Present(command_list* CommandList, const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>(), bool PlaceEndBarriers = true) override;

	void ExecuteAndRemove(command_list* CommandList, const std::vector<gpu_sync*>& Syncs = std::vector<gpu_sync*>()) override;
};

struct directx12_backend;
struct directx12_gpu_sync : gpu_sync
{
	friend directx12_backend;
	friend directx12_command_queue;

public:
	directx12_gpu_sync(renderer_backend* Gfx);
	~directx12_gpu_sync() override;

	void CreateObject() override;
	void DestroyObject() override;

	void Signal(command_queue*) override;
	void Reset() override;

private:
	ID3D12Device6* Device = nullptr;
	ComPtr<ID3D12Fence> Handle = nullptr;
	HANDLE FenceEvent;
};
