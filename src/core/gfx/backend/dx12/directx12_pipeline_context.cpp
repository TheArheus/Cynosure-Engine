
void directx12_resource_binder::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
#if 0
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);

	auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx + RepeatedResourceExec);
	auto ParameterType = ShaderRootLayout[Set][SetIndices[Set]][0].ParameterType;
	if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
	{
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->ShaderResourceView, ResourceHeap.Type);

		BindingDescriptions.push_back({dx12_descriptor_type::shader_resource, {}, ToBind->Handle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
	}
	else if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
	{
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->UnorderedAccessView, ResourceHeap.Type);

		BindingDescriptions.push_back({dx12_descriptor_type::unordered_access, {}, ToBind->Handle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
	}
	RootResourceBindingIdx += 1;
	ResourceBindingIdx += 1;
	SetIndices[Set] += 1;

	if(UseCounter && Buffer->WithCounter)
	{
		DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx + RepeatedResourceExec);
		if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
		{
			Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->CounterShaderResourceView, ResourceHeap.Type);
			BindingDescriptions.push_back({dx12_descriptor_type::shader_resource, {}, ToBind->CounterHandle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
		}
		else if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
		{
			Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->CounterUnorderedAccessView, ResourceHeap.Type);
			BindingDescriptions.push_back({dx12_descriptor_type::unordered_access, {}, ToBind->CounterHandle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
		}
		RootResourceBindingIdx += 1;
		ResourceBindingIdx += 1;
		SetIndices[Set] += 1;
	}
#endif
}

void directx12_resource_binder::
SetUniformBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
#if 0
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);
	SetIndices[Set] += 1;
#endif
}

void directx12_resource_binder::
SetSampledImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
#if 0
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][1].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap.GetGpuHandle(SamplersBindingIdx + RepeatedSamplerExec), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap.GetCpuHandle(SamplersBindingIdx + RepeatedSamplerExec);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap.Type);
		SamplersBindingIdx++;
	}
	SamplersBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
#endif
}

void directx12_resource_binder::
SetStorageImage(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
#if 0
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][0].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::unordered_access_table, ResourceHeap.GetGpuHandle(ResourceBindingIdx + RepeatedResourceExec), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx + RepeatedResourceExec);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->UnorderedAccessViews[ViewIdx], ResourceHeap.Type);
		ResourceBindingIdx++;
	}
	ResourceBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
#endif
}

void directx12_resource_binder::
SetImageSampler(const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
#if 0
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][0].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::shader_resource_table, ResourceHeap.GetGpuHandle(ResourceBindingIdx + RepeatedResourceExec), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx + RepeatedResourceExec);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->ShaderResourceViews[ViewIdx], ResourceHeap.Type);
		ResourceBindingIdx++;
	}
	ResourceBindingIdx += (BindingSize - Textures.size());

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap.GetGpuHandle(SamplersBindingIdx + RepeatedSamplerExec), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap.GetCpuHandle(SamplersBindingIdx + RepeatedSamplerExec);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap.Type);
		SamplersBindingIdx++;
	}
	SamplersBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
#endif
}

void directx12_command_list::
CreateResource(renderer_backend* Backend)
{
	Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();

	Fence.Init(Gfx->Device.Get());

	CommandList = Gfx->CommandQueue->AllocateCommandList();
}

void directx12_command_list::
AcquireNextImage()
{
	// NOTE: For directx12 this happens in the end of the frame
}

void directx12_command_list::
Begin()
{
	Gfx->CommandQueue->Reset();
	CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);
}

void directx12_command_list::
End()
{
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	Gfx->CommandQueue->Execute(CommandList);

	HRESULT DeviceResult = Gfx->Device->GetDeviceRemovedReason();
	if(!SUCCEEDED(DeviceResult))
	{
		printf("Device removed reason: %#08x\n", DeviceResult);
	}

	Fence.Flush(Gfx->CommandQueue);
}

void directx12_command_list::
EndOneTime()
{
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	Gfx->CommandQueue->Execute(CommandList);
	Fence.Flush(Gfx->CommandQueue);
}

void directx12_command_list::
SetGraphicsPipelineState(render_context* Context)
{
	CurrentContext = Context;
	directx12_render_context* ContextToBind = static_cast<directx12_render_context*>(Context);
	CommandList->SetGraphicsRootSignature(ContextToBind->RootSignatureHandle.Get());
	CommandList->IASetPrimitiveTopology(PipelineTopology);
#if 0
	PipelineContext = static_cast<directx12_command_list*>(GlobalPipelineContext);

	PipelineContext->CommandList->SetPipelineState(Pipeline.Get());

	std::vector<ID3D12DescriptorHeap*> Heaps;
	if (IsResourceHeapInited) Heaps.push_back(ResourceHeap.Handle.Get());
	if (IsSamplersHeapInited) Heaps.push_back(SamplersHeap.Handle.Get());
	PipelineContext->CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());

	PipelineContext->CommandList->SetGraphicsRootSignature(RootSignatureHandle.Get());

	for(u32 BindingIdx = 0; BindingIdx < BindingDescriptions.size(); ++BindingIdx)
	{
		const descriptor_binding& BindingDesc = BindingDescriptions[BindingIdx];
		switch(BindingDesc.Type)
		{
			case dx12_descriptor_type::shader_resource_table:
			case dx12_descriptor_type::unordered_access_table:
			case dx12_descriptor_type::constant_buffer_table:
			case dx12_descriptor_type::sampler:
			{
				PipelineContext->CommandList->SetGraphicsRootDescriptorTable(BindingDesc.Idx, BindingDesc.TableBegin);
			} break;
			case dx12_descriptor_type::shader_resource:
			{
				PipelineContext->CommandList->SetGraphicsRootShaderResourceView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
			case dx12_descriptor_type::unordered_access:
			{
				PipelineContext->CommandList->SetGraphicsRootUnorderedAccessView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
		}
	}
#endif
}

void directx12_command_list::
SetComputePipelineState(compute_context* Context)
{
	CurrentContext = Context;
	directx12_compute_context* ContextToBind = static_cast<directx12_compute_context*>(Context);
	CommandList->SetComputeRootSignature(ContextToBind->RootSignatureHandle.Get());

#if 0
	PipelineContext = static_cast<directx12_command_list*>(GlobalPipelineContext);

	PipelineContext->CommandList->SetPipelineState(Pipeline.Get());

	std::vector<ID3D12DescriptorHeap*> Heaps;
	if (IsResourceHeapInited) Heaps.push_back(ResourceHeap.Handle.Get());
	if (IsSamplersHeapInited) Heaps.push_back(SamplersHeap.Handle.Get());
	PipelineContext->CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());

	PipelineContext->CommandList->SetComputeRootSignature(RootSignatureHandle.Get());

	for(u32 BindingIdx = 0; BindingIdx < BindingDescriptions.size(); ++BindingIdx)
	{
		const descriptor_binding& BindingDesc = BindingDescriptions[BindingIdx];
		switch(BindingDesc.Type)
		{
			case dx12_descriptor_type::shader_resource_table:
			case dx12_descriptor_type::unordered_access_table:
			case dx12_descriptor_type::constant_buffer_table:
			case dx12_descriptor_type::sampler:
			{
				PipelineContext->CommandList->SetComputeRootDescriptorTable(BindingDesc.Idx, BindingDesc.TableBegin);
			} break;
			case dx12_descriptor_type::shader_resource:
			{
				PipelineContext->CommandList->SetComputeRootShaderResourceView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
			case dx12_descriptor_type::unordered_access:
			{
				PipelineContext->CommandList->SetComputeRootUnorderedAccessView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
		}
	}
#endif
}

void directx12_command_list::
SetConstant(void* Data, size_t Size)
{
	//PipelineContext->CommandList->SetGraphicsRoot32BitConstants(HaveDrawID + RootResourceBindingIdx + RootSamplersBindingIdx, Size / sizeof(u32), Data, 0);
}

void directx12_command_list::
SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight)
{
	D3D12_VIEWPORT Viewport = {(r32)StartX, (r32)StartY, (r32)RenderWidth, (r32)RenderHeight, 0, 1};
	CommandList->RSSetViewports(1, &Viewport);

	D3D12_RECT Scissors = {(LONG)StartX, (LONG)StartY, (LONG)RenderWidth, (LONG)RenderHeight};
	CommandList->RSSetScissorRects(1, &Scissors);
}

void directx12_command_list::
SetIndexBuffer(buffer* Buffer)
{
	directx12_buffer* IndexAttachment = static_cast<directx12_buffer*>(Buffer);
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {IndexAttachment->Handle->GetGPUVirtualAddress(), (u32)IndexAttachment->Size, DXGI_FORMAT_R32_UINT};
	CommandList->IASetIndexBuffer(&IndexBufferView);
}

void directx12_command_list::
EmplaceColorTarget(texture* RenderTexture)
{
	directx12_texture* Texture = static_cast<directx12_texture*>(RenderTexture);

	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), GetDXImageLayout(Texture->CurrentState[0], Texture->CurrentLayout[0]), D3D12_RESOURCE_STATE_COPY_SOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(Gfx->SwapchainImages[BackBufferIndex].Get(), Gfx->SwapchainCurrentState[BackBufferIndex], D3D12_RESOURCE_STATE_COPY_DEST)
	};

	std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), AF_TransferRead);
	std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), barrier_state::general);
	Gfx->SwapchainCurrentState[BackBufferIndex] = D3D12_RESOURCE_STATE_COPY_DEST;
	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	CommandList->CopyResource(Gfx->SwapchainImages[BackBufferIndex].Get(), Texture->Handle.Get());
}

void directx12_command_list::
Present()
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(Gfx->SwapchainImages[BackBufferIndex].Get(), Gfx->SwapchainCurrentState[BackBufferIndex], D3D12_RESOURCE_STATE_COMMON)
	};
	Gfx->SwapchainCurrentState[BackBufferIndex] = D3D12_RESOURCE_STATE_COMMON;

	for(u32 Idx = 0; Idx < BuffersToCommon.size(); ++Idx)
	{
		directx12_buffer* Resource = static_cast<directx12_buffer*>(*std::next(BuffersToCommon.begin(), Idx));

		if(GetDXBufferLayout(Resource->CurrentLayout, Resource->PrevShader) == D3D12_RESOURCE_STATE_COMMON) continue;
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource->Handle.Get(), GetDXBufferLayout(Resource->CurrentLayout, Resource->PrevShader), D3D12_RESOURCE_STATE_COMMON));

		Resource->CurrentLayout = 0;
		Resource->PrevShader = 0;
	}

	for(u32 Idx = 0; Idx < TexturesToCommon.size(); ++Idx)
	{
		directx12_texture* Resource = static_cast<directx12_texture*>(*std::next(TexturesToCommon.begin(), Idx));

		if(GetDXImageLayout(Resource->CurrentState[0], Resource->CurrentLayout[0], Resource->PrevShader) == D3D12_RESOURCE_STATE_COMMON) continue;
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource->Handle.Get(), GetDXImageLayout(Resource->CurrentState[0], Resource->CurrentLayout[0], Resource->PrevShader), D3D12_RESOURCE_STATE_COMMON));

		std::fill(Resource->CurrentLayout.begin(), Resource->CurrentLayout.end(), 0);
		std::fill(Resource->CurrentState.begin(), Resource->CurrentState.end(), barrier_state::general);
		Resource->PrevShader = 0;
	}

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	BuffersToCommon.clear();
	TexturesToCommon.clear();
	Gfx->CommandQueue->Execute(CommandList);

	HRESULT DeviceResult = Gfx->Device->GetDeviceRemovedReason();
	if(!SUCCEEDED(DeviceResult))
	{
		printf("Device removed reason: %#08x\n", DeviceResult);

		ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
		Gfx->Device->QueryInterface(IID_PPV_ARGS(&pDred));
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
		D3D12_DRED_PAGE_FAULT_OUTPUT1 DredPageFaultOutput;
		pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput);
		pDred->GetPageFaultAllocationOutput1(&DredPageFaultOutput);

		int fin = 5;
	}

	Fence.Flush(Gfx->CommandQueue);

	Gfx->SwapChain->Present(0, 0);
	BackBufferIndex = Gfx->SwapChain->GetCurrentBackBufferIndex();
}

void directx12_command_list::
BindShaderParameters(void* Data)
{
#if 0
	PipelineContext->CommandList->SetPipelineState(Pipeline.Get());

	std::vector<ID3D12DescriptorHeap*> Heaps;
	if (IsResourceHeapInited) Heaps.push_back(ResourceHeap.Handle.Get());
	if (IsSamplersHeapInited) Heaps.push_back(SamplersHeap.Handle.Get());
	PipelineContext->CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());

	PipelineContext->CommandList->SetGraphicsRootSignature(RootSignatureHandle.Get());

	for(u32 BindingIdx = 0; BindingIdx < BindingDescriptions.size(); ++BindingIdx)
	{
		const descriptor_binding& BindingDesc = BindingDescriptions[BindingIdx];
		switch(BindingDesc.Type)
		{
			case dx12_descriptor_type::shader_resource_table:
			case dx12_descriptor_type::unordered_access_table:
			case dx12_descriptor_type::constant_buffer_table:
			case dx12_descriptor_type::sampler:
			{
				PipelineContext->CommandList->SetGraphicsRootDescriptorTable(BindingDesc.Idx, BindingDesc.TableBegin);
			} break;
			case dx12_descriptor_type::shader_resource:
			{
				PipelineContext->CommandList->SetGraphicsRootShaderResourceView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
			case dx12_descriptor_type::unordered_access:
			{
				PipelineContext->CommandList->SetGraphicsRootUnorderedAccessView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
		}
	}
#endif
}

void directx12_command_list::
DrawIndirect(buffer* IndirectCommands, u32 ObjectDrawCount, u32 CommandStructureSize)
{
	assert(CurrentContext->Type == pass_type::graphics);

	directx12_render_context* Context = static_cast<directx12_render_context*>(CurrentContext);

	directx12_buffer* Indirect = static_cast<directx12_buffer*>(IndirectCommands);
	CommandList->ExecuteIndirect(Context->IndirectSignatureHandle.Get(), ObjectDrawCount, Indirect->Handle.Get(), !Context->HaveDrawID * 4, Indirect->CounterHandle.Get(), 0);
}

void directx12_command_list::
Dispatch(u32 X, u32 Y, u32 Z)
{
	assert(CurrentContext->Type == pass_type::compute);

	directx12_compute_context* Context = static_cast<directx12_compute_context*>(CurrentContext);
	CommandList->Dispatch((X + Context->BlockSizeX - 1) / Context->BlockSizeX, (Y + Context->BlockSizeY - 1) / Context->BlockSizeY, (Z + Context->BlockSizeZ - 1) / Context->BlockSizeZ);
}

void directx12_command_list::
FillBuffer(buffer* Buffer, u32 Value)
{
}

void directx12_command_list::
FillTexture(texture* Buffer, vec4 Value)
{
}

void directx12_command_list::
CopyImage(texture* Dst, texture* Src)
{
}

void directx12_command_list::
SetBufferBarrier(const std::tuple<buffer*, u32>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	directx12_buffer* Buffer = static_cast<directx12_buffer*>(std::get<0>(BarrierData));
	std::vector<CD3DX12_RESOURCE_BARRIER> Barrier;

	u32 ResourceLayoutNext = std::get<1>(BarrierData);

	D3D12_RESOURCE_STATES CurrState = GetDXBufferLayout(Buffer->CurrentLayout, DstStageMask);
	D3D12_RESOURCE_STATES NextState = GetDXBufferLayout(ResourceLayoutNext, DstStageMask);

	if(CurrState != NextState)
	{
		Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Buffer->Handle.Get(), CurrState, NextState));
		if(Buffer->WithCounter)
			Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Buffer->CounterHandle.Get(), CurrState, NextState));
	}
	else
	{
		Barrier.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer->Handle.Get()));
		if(Buffer->WithCounter)
			Barrier.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer->CounterHandle.Get()));
	}

	Buffer->CurrentLayout = ResourceLayoutNext;
	Buffer->PrevShader = DstStageMask;

	CommandList->ResourceBarrier(Barrier.size(), Barrier.data());
}

void directx12_command_list::
SetBufferBarriers(const std::vector<std::tuple<buffer*, u32>>& BarrierData, 
				  u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	for(const std::tuple<buffer*, u32>& Data : BarrierData)
	{
		directx12_buffer* Buffer = static_cast<directx12_buffer*>(std::get<0>(Data));

		u32 ResourceLayoutNext = std::get<1>(Data);

		D3D12_RESOURCE_STATES CurrState = GetDXBufferLayout(Buffer->CurrentLayout, DstStageMask);
		D3D12_RESOURCE_STATES NextState = GetDXBufferLayout(ResourceLayoutNext, DstStageMask);

		if(CurrState != NextState)
		{
			Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Buffer->Handle.Get(), CurrState, NextState));
			if(Buffer->WithCounter)
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Buffer->CounterHandle.Get(), CurrState, NextState));
		}
		else
		{
			Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer->Handle.Get()));
			if(Buffer->WithCounter)
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer->CounterHandle.Get()));
		}

		Buffer->PrevShader = DstStageMask;
		Buffer->CurrentLayout = ResourceLayoutNext;
	}

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());
}

void directx12_command_list::
SetImageBarriers(const std::vector<std::tuple<texture*, u32, barrier_state, u32>>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	for(const std::tuple<texture*, u32, barrier_state, u32>& Data : BarrierData)
	{
		directx12_texture* Texture = static_cast<directx12_texture*>(std::get<0>(Data));

		u32 SubresourceIndex = std::get<3>(Data);
		u32 MipIdx = SubresourceIndex;
		if(SubresourceIndex == ~0u) SubresourceIndex = 0;

		u32 ResourceLayoutNext = std::get<1>(Data);
		barrier_state ResourceStateNext = std::get<2>(Data);

		D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(Texture->CurrentState[SubresourceIndex], Texture->CurrentLayout[SubresourceIndex]);
		D3D12_RESOURCE_STATES NextState = GetDXImageLayout(ResourceStateNext, ResourceLayoutNext, DstStageMask);

		if(CurrState != NextState)
			Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), CurrState, NextState, MipIdx));
		else
			Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));

		if(MipIdx != ~0u)
		{
			Texture->CurrentLayout[SubresourceIndex] = ResourceLayoutNext;
			Texture->CurrentState[SubresourceIndex] = ResourceStateNext;
		}
		else
		{
			std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), ResourceLayoutNext);
			std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), ResourceStateNext);
		}

		Texture->PrevShader = DstStageMask;
	}

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());
}

void directx12_command_list::
SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, barrier_state, u32>>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	for(const std::tuple<std::vector<texture*>, u32, barrier_state, u32>& Data : BarrierData)
	{
		const std::vector<texture*>& Textures = std::get<0>(Data);
		u32 SubresourceIndex = std::get<3>(Data);
		u32 MipIdx = SubresourceIndex;
		if(SubresourceIndex == ~0u) SubresourceIndex = 0;
		if(!Textures.size()) continue;
		for(texture* TextureData : Textures)
		{
			directx12_texture* Texture = static_cast<directx12_texture*>(TextureData);

			u32 ResourceLayoutNext = std::get<1>(Data);
			barrier_state ResourceStateNext = std::get<2>(Data);

			D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(Texture->CurrentState[SubresourceIndex], Texture->CurrentLayout[SubresourceIndex]);
			D3D12_RESOURCE_STATES NextState = GetDXImageLayout(ResourceStateNext, ResourceLayoutNext, DstStageMask);

			if(CurrState != NextState)
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), CurrState, NextState, MipIdx));
			else
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));

			if(MipIdx != ~0u)
			{
				Texture->CurrentLayout[SubresourceIndex] = ResourceLayoutNext;
				Texture->CurrentState[SubresourceIndex] = ResourceStateNext;
			}
			else
			{
				std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), ResourceLayoutNext);
				std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), ResourceStateNext);
			}

			Texture->PrevShader = DstStageMask;
		}
	}

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());
}

void directx12_command_list::
DebugGuiBegin(texture* RenderTarget)
{
	directx12_texture* Clr = static_cast<directx12_texture*>(RenderTarget);

	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	if(GetDXImageLayout(Clr->CurrentState[0], Clr->CurrentLayout[0]) != D3D12_RESOURCE_STATE_RENDER_TARGET) 
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Clr->Handle.Get(), GetDXImageLayout(Clr->CurrentState[0], Clr->CurrentLayout[0]), D3D12_RESOURCE_STATE_RENDER_TARGET));
	else
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Clr->Handle.Get()));

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	std::fill(Clr->CurrentLayout.begin(), Clr->CurrentLayout.end(), AF_ColorAttachmentWrite);
	std::fill(Clr->CurrentState.begin(), Clr->CurrentState.end(), barrier_state::general);

	ImGui_ImplDX12_NewFrame();

	std::vector<ID3D12DescriptorHeap*> Heaps;
	Heaps.push_back(Gfx->ImGuiResourcesHeap.Handle.Get());

	CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());
	CommandList->OMSetRenderTargets(1, &Clr->RenderTargetViews[0], false, nullptr);
}

void directx12_command_list::
DebugGuiEnd() 
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);
}

directx12_render_context::
directx12_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::vector<std::string> ShaderList, 
						 const std::vector<image_format>& ColorTargetFormats, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines)
	: LoadOp(NewLoadOp), StoreOp(NewStoreOp)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();
	Info = InputData;

	D3D12_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	RasterDesc.CullMode = GetDXCullMode(InputData.CullMode);
	RasterDesc.FrontCounterClockwise = true;
	RasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	RasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterDesc.DepthClipEnable = true;
	//RasterDesc.MultisampleEnable = MsaaState;
	//RasterDesc.AntialiasedLineEnable = MsaaState;
	RasterDesc.ForcedSampleCount = 0;
	RasterDesc.ConservativeRaster = InputData.UseConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = InputData.UseDepth;
	DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	DepthStencilDesc.StencilEnable = false;
	DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
	DepthStencilDesc.BackFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDesc = {};

	std::unordered_map<u32, u32> DescriptorHeapSizes;
	D3D12_ROOT_PARAMETER DrawConstantDesc = {};
	D3D12_ROOT_PARAMETER PushConstantDesc = {};

	std::string GlobalName;
	std::vector<ComPtr<ID3DBlob>> ShadersBlob;
	std::map<u32, std::map<u32, u32>> NewBindings;
	for(std::string Shader : ShaderList)
	{
		GlobalName = Shader.substr(Shader.find("."));
		if(Shader.find(".vert.") != std::string::npos)
		{
			PipelineDesc.VS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::vertex, HaveDrawID, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			Shader.replace(Shader.find("glsl"), 4, "hlsl");
			PipelineDesc.DS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_control, HaveDrawID, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			Shader.replace(Shader.find("glsl"), 4, "hlsl");
			PipelineDesc.HS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_eval, HaveDrawID, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			Shader.replace(Shader.find("glsl"), 4, "hlsl");
			PipelineDesc.GS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::geometry, HaveDrawID, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			PipelineDesc.PS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::fragment, HaveDrawID, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
	}

	std::vector<D3D12_ROOT_PARAMETER> Parameters;
	for(u32 LayoutIdx = 1; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			D3D12_ROOT_PARAMETER& Parameter0 = ShaderRootLayout[LayoutIdx][BindingIdx][0];
			for(u32 ParamIdx = 0; ParamIdx < ShaderRootLayout[LayoutIdx][BindingIdx].size(); ++ParamIdx)
			{
				D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[LayoutIdx][BindingIdx][ParamIdx];
				if(Parameter.DescriptorTable.pDescriptorRanges)
				{
					D3D12_DESCRIPTOR_RANGE* pDescriptorRanges = const_cast<D3D12_DESCRIPTOR_RANGE*>(Parameter.DescriptorTable.pDescriptorRanges);
					pDescriptorRanges->BaseShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				else
				{
					Parameter.Descriptor.ShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				Parameters.push_back(Parameter);
			}
		}
	}
	for(u32 LayoutIdx = 0; LayoutIdx < 1; LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			D3D12_ROOT_PARAMETER& Parameter0 = ShaderRootLayout[LayoutIdx][BindingIdx][0];
			for(u32 ParamIdx = 0; ParamIdx < ShaderRootLayout[LayoutIdx][BindingIdx].size(); ++ParamIdx)
			{
				D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[LayoutIdx][BindingIdx][ParamIdx];
				if(Parameter.DescriptorTable.pDescriptorRanges)
				{
					D3D12_DESCRIPTOR_RANGE* pDescriptorRanges = const_cast<D3D12_DESCRIPTOR_RANGE*>(Parameter.DescriptorTable.pDescriptorRanges);
					pDescriptorRanges->BaseShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				else
				{
					Parameter.Descriptor.ShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				Parameters.push_back(Parameter);
			}
		}
	}

	if(HaveDrawID)
	{
		DrawConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		DrawConstantDesc.Constants.ShaderRegister = HavePushConstant;
		DrawConstantDesc.Constants.Num32BitValues = 1;
		DrawConstantDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		Parameters.push_back(DrawConstantDesc);
	}

	if(HavePushConstant)
	{
		PushConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		PushConstantDesc.Constants.Num32BitValues = PushConstantSize / sizeof(u32);
		PushConstantDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameters.push_back(PushConstantDesc);
	}

	size_t LastSlashPos = GlobalName.find_last_of('/');
    size_t LastDotPos = GlobalName.find_last_of('.');
    GlobalName = GlobalName.substr(LastSlashPos + 1, LastDotPos - LastSlashPos - 1);

	// TODO: TEMPORAL HACK. Need a better solution here for different tasks
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != 0)
	{
		ResourceHeap = descriptor_heap(Gfx->Device.Get(), Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * 16, 16384u), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(ResourceHeap.Handle.Get(), (GlobalName + ".resource_heap").c_str());
		IsResourceHeapInited = true;
	}
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != 0)
	{
		SamplersHeap = descriptor_heap(Gfx->Device.Get(), Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] * 16, 2048u), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(SamplersHeap.Handle.Get(), (GlobalName + ".samplers_heap").c_str());
		IsSamplersHeapInited = true;
	}

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Init(Parameters.size(), Parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> Signature;
	ComPtr<ID3DBlob> Error;
	HRESULT Res = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &Signature, &Error);
	if(!SUCCEEDED(Res))
	{
		std::cerr << std::string(static_cast<const char*>(Error->GetBufferPointer())) << std::endl;
	}
	Gfx->Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignatureHandle));

	PipelineDesc.pRootSignature = RootSignatureHandle.Get();
	for(u32 FormatIdx = 0; FormatIdx < ColorTargetFormats.size(); ++FormatIdx) PipelineDesc.RTVFormats[FormatIdx] = GetDXFormat(ColorTargetFormats[FormatIdx]);

	PipelineDesc.RasterizerState = RasterDesc;
	PipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PipelineDesc.DepthStencilState = DepthStencilDesc;
	PipelineDesc.SampleMask = UINT_MAX;
	PipelineDesc.PrimitiveTopologyType = InputData.UseOutline ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PipelineTopology = InputData.UseOutline ? D3D_PRIMITIVE_TOPOLOGY_LINELIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PipelineDesc.NumRenderTargets = ColorTargetFormats.size();
	PipelineDesc.SampleDesc.Count = 1; //MsaaState ? MsaaQuality : 1;
	PipelineDesc.SampleDesc.Quality = 0; //MsaaState ? (MsaaQuality - 1) : 0;
	PipelineDesc.DSVFormat = InputData.UseDepth ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;
	PipelineDesc.SampleDesc.Count = 1;

	Res = Gfx->Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&Pipeline));

	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> IndirectArgs;
	D3D12_INDIRECT_ARGUMENT_DESC IndirectArg = {};
	IndirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	IndirectArg.Constant.RootParameterIndex = Parameters.size() - 1;
	IndirectArg.Constant.DestOffsetIn32BitValues = 0;
	IndirectArg.Constant.Num32BitValuesToSet = 1;
	if(HaveDrawID) IndirectArgs.push_back(IndirectArg);
	IndirectArg = {};
	IndirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	IndirectArgs.push_back(IndirectArg);

	D3D12_COMMAND_SIGNATURE_DESC IndirectDesc = {};
	IndirectDesc.ByteStride = sizeof(indirect_draw_indexed_command);
	IndirectDesc.NumArgumentDescs = IndirectArgs.size();
	IndirectDesc.pArgumentDescs = IndirectArgs.data();
	Gfx->Device->CreateCommandSignature(&IndirectDesc, HaveDrawID ? RootSignatureHandle.Get() : nullptr, IID_PPV_ARGS(&IndirectSignatureHandle));
}

void directx12_render_context::
Begin(command_list* GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight)  
{
	PipelineContext = static_cast<directx12_command_list*>(GlobalPipelineContext);

	PipelineContext->CommandList->SetPipelineState(Pipeline.Get());

	std::vector<ID3D12DescriptorHeap*> Heaps;
	if (IsResourceHeapInited) Heaps.push_back(ResourceHeap.Handle.Get());
	if (IsSamplersHeapInited) Heaps.push_back(SamplersHeap.Handle.Get());
	PipelineContext->CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());

	PipelineContext->CommandList->SetGraphicsRootSignature(RootSignatureHandle.Get());

	for(u32 BindingIdx = 0; BindingIdx < BindingDescriptions.size(); ++BindingIdx)
	{
		const descriptor_binding& BindingDesc = BindingDescriptions[BindingIdx];
		switch(BindingDesc.Type)
		{
			case dx12_descriptor_type::shader_resource_table:
			case dx12_descriptor_type::unordered_access_table:
			case dx12_descriptor_type::constant_buffer_table:
			case dx12_descriptor_type::sampler:
			{
				PipelineContext->CommandList->SetGraphicsRootDescriptorTable(BindingDesc.Idx, BindingDesc.TableBegin);
			} break;
			case dx12_descriptor_type::shader_resource:
			{
				PipelineContext->CommandList->SetGraphicsRootShaderResourceView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
			case dx12_descriptor_type::unordered_access:
			{
				PipelineContext->CommandList->SetGraphicsRootUnorderedAccessView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
		}
	}

	D3D12_VIEWPORT Viewport = {0, 0, (r32)RenderWidth, (r32)RenderHeight, 0, 1};
	D3D12_RECT     Scissors = {0, 0, (LONG)RenderWidth, (LONG)RenderHeight};

	PipelineContext->CommandList->RSSetViewports(1, &Viewport);
	PipelineContext->CommandList->RSSetScissorRects(1, &Scissors);
	PipelineContext->CommandList->IASetPrimitiveTopology(PipelineTopology);
}

void directx12_render_context::
End()  
{
	RepeatedResourceExec += ResourceBindingIdx;
	RepeatedSamplerExec += SamplersBindingIdx;

	ResourceBindingIdx = 0;
	SamplersBindingIdx = 0;
	RootResourceBindingIdx = 0;
	RootSamplersBindingIdx = 0;

	ColorTargets.clear();
	ResourceHeap.Reset();
	SamplersHeap.Reset();
	BuffersToCommon.clear();
	TexturesToCommon.clear();
	BindingDescriptions.clear();
}

void directx12_render_context::
SetColorTarget(u32 RenderWidth, u32 RenderHeight, const std::vector<texture*>& ColorAttachments, vec4 Clear, u32 Face, bool EnableMultiview)
{
	D3D12_RECT Scissors = {0, 0, (LONG)RenderWidth, (LONG)RenderHeight};
	if(EnableMultiview)
	{
		directx12_texture* Attachment = static_cast<directx12_texture*>(ColorAttachments[0]);
		TexturesToCommon.insert(Attachment);
		ColorTargets.push_back(Attachment->RenderTargetViews[Face]);
		if(LoadOp == load_op::clear)
		{
			PipelineContext->CommandList->ClearRenderTargetView(ColorTargets[0], Clear.E, 0, nullptr);
		}
	}
	else
	{
		for(u32 i = 0; i < ColorAttachments.size(); i++)
		{
			directx12_texture* Attachment = static_cast<directx12_texture*>(ColorAttachments[i]);
			TexturesToCommon.insert(Attachment);
			ColorTargets.push_back(Attachment->RenderTargetViews[0]);
			if(LoadOp == load_op::clear)
			{
				PipelineContext->CommandList->ClearRenderTargetView(ColorTargets[i], Clear.E, 0, nullptr);
			}
		}
	}
}

void directx12_render_context::
SetDepthTarget(u32 RenderWidth, u32 RenderHeight, texture* DepthAttachment, vec2 Clear, u32 Face, bool EnableMultiview)  
{
	directx12_texture* Attachment = static_cast<directx12_texture*>(DepthAttachment);
	TexturesToCommon.insert(Attachment);

	DepthStencilTarget = Attachment->DepthStencilViews[Face];
	if(LoadOp == load_op::clear)
	{
		D3D12_RECT Scissors = {0, 0, (LONG)RenderWidth, (LONG)RenderHeight};
		PipelineContext->CommandList->ClearDepthStencilView(DepthStencilTarget, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Clear.x, Clear.y, 1, &Scissors);
	}
}

#if 0
void directx12_render_context::
Draw(buffer* VertexBuffer, u32 FirstVertex, u32 VertexCount)  
{
	//CommandList->IASetVertexBuffers(0, 1, &View.Handle);
	//CommandList->DrawInstanced(View.VertexCount, 1, View.VertexBegin, 0);
	SetIndices.clear();
}

void directx12_render_context::
DrawIndexed(buffer* IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount)
{
	directx12_buffer* Indices  = static_cast<directx12_buffer*>(IndexBuffer);
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {Indices->Handle->GetGPUVirtualAddress(), (u32)Indices->Size, DXGI_FORMAT_R32_UINT};

	if((ColorTargets.size() > 0) || Info.UseDepth)
		PipelineContext->CommandList->OMSetRenderTargets(ColorTargets.size(), ColorTargets.data(), Info.UseDepth, &DepthStencilTarget);
	PipelineContext->CommandList->IASetIndexBuffer(&IndexBufferView);
	PipelineContext->CommandList->DrawIndexedInstanced(IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
	SetIndices.clear();
}

void directx12_render_context::
DrawIndirect(u32 ObjectDrawCount, buffer* IndexBuffer, buffer* IndirectCommands, u32 CommandStructureSize)  
{
	directx12_buffer* Indices  = static_cast<directx12_buffer*>(IndexBuffer);
	directx12_buffer* Indirect = static_cast<directx12_buffer*>(IndirectCommands);
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {Indices->Handle->GetGPUVirtualAddress(), (u32)Indices->Size, DXGI_FORMAT_R32_UINT};

	if((ColorTargets.size() > 0) || Info.UseDepth)
		PipelineContext->CommandList->OMSetRenderTargets(ColorTargets.size(), ColorTargets.data(), Info.UseDepth, &DepthStencilTarget);
	PipelineContext->CommandList->IASetIndexBuffer(&IndexBufferView);
	PipelineContext->CommandList->ExecuteIndirect(IndirectSignatureHandle.Get(), ObjectDrawCount, Indirect->Handle.Get(), !HaveDrawID * 4, Indirect->CounterHandle.Get(), 0);

	SetIndices.clear();

	PipelineContext->TexturesToCommon.insert(TexturesToCommon.begin(), TexturesToCommon.end());
	PipelineContext->BuffersToCommon.insert(BuffersToCommon.begin(), BuffersToCommon.end());
	PipelineContext->BuffersToCommon.insert(IndirectCommands);
}
#endif

directx12_compute_context::
directx12_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();

	std::unordered_map<u32, u32> DescriptorHeapSizes;
	D3D12_ROOT_PARAMETER PushConstantDesc = {};
	PushConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

	bool HaveDrawID = false;
	D3D12_COMPUTE_PIPELINE_STATE_DESC PipelineDesc = {};
	std::map<u32, std::map<u32, u32>> NewBindings;
	PipelineDesc.CS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::compute, HaveDrawID, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines, &BlockSizeX, &BlockSizeY, &BlockSizeZ);

	std::vector<D3D12_ROOT_PARAMETER> Parameters;
	for(u32 LayoutIdx = 1; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			D3D12_ROOT_PARAMETER& Parameter0 = ShaderRootLayout[LayoutIdx][BindingIdx][0];
			for(u32 ParamIdx = 0; ParamIdx < ShaderRootLayout[LayoutIdx][BindingIdx].size(); ++ParamIdx)
			{
				D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[LayoutIdx][BindingIdx][ParamIdx];
				if(Parameter.DescriptorTable.pDescriptorRanges)
				{
					D3D12_DESCRIPTOR_RANGE* pDescriptorRanges = const_cast<D3D12_DESCRIPTOR_RANGE*>(Parameter.DescriptorTable.pDescriptorRanges);
					pDescriptorRanges->BaseShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				else
				{
					Parameter.Descriptor.ShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				Parameters.push_back(Parameter);
			}
		}
	}

	for(u32 LayoutIdx = 0; LayoutIdx < 1; LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			D3D12_ROOT_PARAMETER& Parameter0 = ShaderRootLayout[LayoutIdx][BindingIdx][0];
			for(u32 ParamIdx = 0; ParamIdx < ShaderRootLayout[LayoutIdx][BindingIdx].size(); ++ParamIdx)
			{
				D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[LayoutIdx][BindingIdx][ParamIdx];
				if(Parameter.DescriptorTable.pDescriptorRanges)
				{
					D3D12_DESCRIPTOR_RANGE* pDescriptorRanges = const_cast<D3D12_DESCRIPTOR_RANGE*>(Parameter.DescriptorTable.pDescriptorRanges);
					pDescriptorRanges->BaseShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				else
				{
					Parameter.Descriptor.ShaderRegister = NewBindings[LayoutIdx][BindingIdx];
				}
				Parameters.push_back(Parameter);
			}
		}
	}

	if(HavePushConstant)
	{
		PushConstantDesc.Constants.Num32BitValues = PushConstantSize / sizeof(u32);
		PushConstantDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameters.push_back(PushConstantDesc);
	}

	size_t LastSlashPos = Shader.find_last_of('/');
    size_t LastDotPos = Shader.find_last_of('.');
	std::string GlobalName = Shader.substr(LastSlashPos + 1, LastDotPos - LastSlashPos - 1);

	// TODO: TEMPORAL HACK. Need a better solution here for different tasks
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != 0)
	{
		ResourceHeap = descriptor_heap(Gfx->Device.Get(), Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * 16, 16384u), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(ResourceHeap.Handle.Get(), (GlobalName + ".resource_heap").c_str());
		IsResourceHeapInited = true;
	}
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != 0)
	{
		SamplersHeap = descriptor_heap(Gfx->Device.Get(), Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] * 16, 2048u), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(SamplersHeap.Handle.Get(), (GlobalName + ".samplers_heap").c_str());
		IsSamplersHeapInited = true;
	}

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Init(Parameters.size(), Parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> Signature;
	ComPtr<ID3DBlob> Error;
	HRESULT Res = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &Signature, &Error);
	if(!SUCCEEDED(Res))
	{
		std::cerr << std::string(static_cast<const char*>(Error->GetBufferPointer())) << std::endl;
	}
	Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignatureHandle));

	PipelineDesc.pRootSignature = RootSignatureHandle.Get();

	Res = Device->CreateComputePipelineState(&PipelineDesc, IID_PPV_ARGS(&Pipeline));
}
