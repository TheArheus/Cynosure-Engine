#pragma once

class directx12_command_queue
{
	D3D12_COMMAND_LIST_TYPE Type;
	ID3D12Device6* Device;
	std::vector<ID3D12GraphicsCommandList*> CommandLists;

public:
	ComPtr<ID3D12CommandQueue> Handle;
	ComPtr<ID3D12CommandAllocator> CommandAlloc;

	directx12_command_queue() = default;

	directx12_command_queue(ID3D12Device6* NewDevice, D3D12_COMMAND_LIST_TYPE NewType)
	{
		Init(NewDevice, NewType);
	}

	void Init(ID3D12Device6* NewDevice, D3D12_COMMAND_LIST_TYPE NewType)
	{
		Device = NewDevice;
		Type = NewType;

		D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
		CommandQueueDesc.Type = NewType;
		CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&Handle));

		Device->CreateCommandAllocator(Type, IID_PPV_ARGS(&CommandAlloc));
	}

	ID3D12GraphicsCommandList* AllocateCommandList()
	{
		ID3D12GraphicsCommandList* Result;
		Device->CreateCommandList(0, Type, CommandAlloc.Get(), nullptr, IID_PPV_ARGS(&Result));
		Result->Close();
		CommandLists.push_back(Result);

		return Result;
	}

	void Reset()
	{
		CommandAlloc->Reset();
	}

	void Execute()
	{
		for(ID3D12GraphicsCommandList* CommandList : CommandLists)
		{
			CommandList->Close();
		}

		std::vector<ID3D12CommandList*> CmdLists(CommandLists.begin(), CommandLists.end());
		Handle->ExecuteCommandLists(CmdLists.size(), CmdLists.data());
	}

	void Execute(ID3D12GraphicsCommandList* CommandList)
	{
		CommandList->Close();
		ID3D12CommandList* CmdLists[] = { CommandList };
		Handle->ExecuteCommandLists(_countof(CmdLists), CmdLists);
	}

	void ExecuteAndRemove(ID3D12GraphicsCommandList* CommandList)
	{
		Execute(CommandList);
		CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
	}

	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;
};

class directx12_fence
{
public:
	directx12_fence() = default;
	directx12_fence(ID3D12Device6* Device)
	{
		Init(Device);
	}

	void Init(ID3D12Device6* Device)
	{
		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
		FenceEvent = CreateEvent(nullptr, 0, 0, nullptr);
	}

	void Signal(directx12_command_queue* CommandQueue)
	{
		CommandQueue->Handle->Signal(Fence.Get(), ++CurrentFence);
	}

	void Flush(directx12_command_queue* CommandQueue)
	{
		Signal(CommandQueue);
		Wait();
	}

	void Wait()
	{
		if(Fence->GetCompletedValue() < CurrentFence)
		{
			HANDLE Event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			Fence->SetEventOnCompletion(CurrentFence, Event);
			WaitForSingleObject(Event, INFINITE);
			CloseHandle(Event);
		}
	}

private:
	u64 CurrentFence = 0;
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;
};
