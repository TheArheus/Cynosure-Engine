
directx12_gpu_sync::
directx12_gpu_sync(renderer_backend* Gfx)
{
	Device = static_cast<directx12_backend*>(Gfx)->Device.Get();
	CreateObject();
}

directx12_gpu_sync::
~directx12_gpu_sync()
{
	DestroyObject();
}

void directx12_gpu_sync::
CreateObject()
{
	CurrentWaitValue = 0;
	Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Handle));
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void directx12_gpu_sync::
DestroyObject()
{
	CloseHandle(FenceEvent);
	if(Handle)
	{
		Handle.Reset();
	}
}

void directx12_gpu_sync::
Signal(command_queue* CommandQueue)
{
	CurrentWaitValue++;
	static_cast<directx12_command_queue*>(CommandQueue)->Handle->Signal(Handle.Get(), CurrentWaitValue);
}

void directx12_gpu_sync::
Reset()
{
	DestroyObject();
	CreateObject();
}



void directx12_command_queue::
Init(renderer_backend* Backend, D3D12_COMMAND_LIST_TYPE NewType)
{
	Gfx = Backend;
	Device = static_cast<directx12_backend*>(Backend)->Device.Get();
	Type = NewType;

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Type = NewType;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&Handle));

	Device->CreateCommandAllocator(Type, IID_PPV_ARGS(&CommandAlloc));
}

void directx12_command_queue::
DestroyObject()
{
	for(command_list* CommandList : CommandLists)
	{
		directx12_command_list* Cmd = static_cast<directx12_command_list*>(CommandList);
		Cmd->Handle->Release();
		delete CommandList;
	}
	CommandLists.clear();

	if(CommandAlloc)
	{
		CommandAlloc.Reset();
	}

	if(Handle)
	{
		Handle.Reset();
	}
}

void directx12_command_queue::
Reset()
{
	CommandAlloc->Reset();
}

command_list* directx12_command_queue::
AllocateCommandList(command_list_level Level)
{
	directx12_command_list* NewCommandList = new directx12_command_list;
	NewCommandList->Gfx = static_cast<directx12_backend*>(Gfx);
	NewCommandList->Device = Device;
	NewCommandList->CommandAlloc = CommandAlloc.Get();

	Device->CreateCommandList(0, Type, CommandAlloc.Get(), nullptr, IID_PPV_ARGS(&NewCommandList->Handle));
	NewCommandList->Handle->Close();
	CommandLists.push_back(NewCommandList);

	return NewCommandList;
}

void directx12_command_queue::
Remove(command_list* CommandList)
{
	if(CommandList == nullptr) return;
	static_cast<directx12_command_list*>(CommandList)->Handle->Release();
	delete CommandList;
	CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
}

void directx12_command_queue::
Execute(const std::vector<gpu_sync*>& Syncs)
{
	Gfx->Wait(Syncs);

	std::vector<ID3D12CommandList*> CommitLists;
	for(command_list* CommandList : CommandLists)
	{
		CommandList->PlaceEndOfFrameBarriers();
		CommandList->IsRunning = false;

		directx12_command_list* Cmd = static_cast<directx12_command_list*>(CommandList);
		Cmd->Handle->Close();
		CommitLists.push_back(Cmd->Handle);

		CommandList->PrevContext = nullptr;
		CommandList->CurrContext = nullptr;
	}
	Handle->ExecuteCommandLists(CommitLists.size(), CommitLists.data());

	for(gpu_sync* Sync : Syncs)
	{
		Sync->Signal(this);
	}
}

void directx12_command_queue::
Present(const std::vector<gpu_sync*>& Syncs)
{
	Gfx->Wait(Syncs);

	std::vector<ID3D12CommandList*> CommitLists;
	for(command_list* CommandList : CommandLists)
	{
		CommandList->PlaceEndOfFrameBarriers();
		CommandList->IsRunning = false;

		directx12_command_list* Cmd = static_cast<directx12_command_list*>(CommandList);
		Cmd->Handle->Close();
		CommitLists.push_back(Cmd->Handle);

		CommandList->PrevContext = nullptr;
		CommandList->CurrContext = nullptr;
	}
	Handle->ExecuteCommandLists(CommitLists.size(), CommitLists.data());

	for(gpu_sync* Sync : Syncs)
	{
		Sync->Signal(this);
	}

	static_cast<directx12_backend*>(Gfx)->SwapChain->Present(0, static_cast<directx12_backend*>(Gfx)->TearingSupport * DXGI_PRESENT_ALLOW_TEARING);
}

void directx12_command_queue::
Execute(command_list* CommandList, const std::vector<gpu_sync*>& Syncs)
{
	Gfx->Wait(Syncs);

	ID3D12CommandList* CmdHandle = static_cast<ID3D12CommandList*>(static_cast<directx12_command_list*>(CommandList)->Handle);

	CommandList->PlaceEndOfFrameBarriers();
	static_cast<directx12_command_list*>(CommandList)->Handle->Close();
	Handle->ExecuteCommandLists(1, &CmdHandle);

	for(gpu_sync* Sync : Syncs)
	{
		Sync->Signal(this);
	}

	CommandList->PrevContext = nullptr;
	CommandList->CurrContext = nullptr;
}

void directx12_command_queue::
Present(command_list* CommandList, const std::vector<gpu_sync*>& Syncs)
{
	Gfx->Wait(Syncs);

	ID3D12CommandList* CmdHandle = static_cast<ID3D12CommandList*>(static_cast<directx12_command_list*>(CommandList)->Handle);

	CommandList->PlaceEndOfFrameBarriers();
	static_cast<directx12_command_list*>(CommandList)->Handle->Close();
	Handle->ExecuteCommandLists(1, &CmdHandle);

	static_cast<directx12_backend*>(Gfx)->SwapChain->Present(0, static_cast<directx12_backend*>(Gfx)->TearingSupport * DXGI_PRESENT_ALLOW_TEARING);

	for(gpu_sync* Sync : Syncs)
	{
		Sync->Signal(this);
	}

	CommandList->PrevContext = nullptr;
	CommandList->CurrContext = nullptr;
}

void directx12_command_queue::
ExecuteAndRemove(command_list* CommandList, const std::vector<gpu_sync*>& Syncs)
{
	Execute(CommandList, Syncs);
	Remove(CommandList);
}
