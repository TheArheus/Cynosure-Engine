

void directx12_resource_binder::
SetStorageBufferView(resource* Buffer, u32 Set)
{
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);

	auto DescriptorHandle = ResourceHeap->GetCpuHandle(ResourceBindingIdx);
	auto ParameterType = ShaderRootLayout[Set][SetIndices[Set]][0].ParameterType;
	if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
	{
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->ShaderResourceView, ResourceHeap->Type);

		BindingDescriptions.push_back({dx12_descriptor_type::shader_resource, {}, ToBind->Handle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
	}
	else if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
	{
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->UnorderedAccessView, ResourceHeap->Type);

		BindingDescriptions.push_back({dx12_descriptor_type::unordered_access, {}, ToBind->Handle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
	}
	RootResourceBindingIdx += 1;
	ResourceBindingIdx += 1;
	SetIndices[Set] += 1;

	if(ToBind->WithCounter)
	{
		DescriptorHandle = ResourceHeap->GetCpuHandle(ResourceBindingIdx);
		if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
		{
			Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->CounterShaderResourceView, ResourceHeap->Type);
			BindingDescriptions.push_back({dx12_descriptor_type::shader_resource, {}, ToBind->CounterHandle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
		}
		else if(ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
		{
			Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->CounterUnorderedAccessView, ResourceHeap->Type);
			BindingDescriptions.push_back({dx12_descriptor_type::unordered_access, {}, ToBind->CounterHandle->GetGPUVirtualAddress(), RootResourceBindingIdx + RootSamplersBindingIdx, 1});
		}
		RootResourceBindingIdx += 1;
		ResourceBindingIdx += 1;
		SetIndices[Set] += 1;
	}
}

void directx12_resource_binder::
SetUniformBufferView(resource* Buffer, u32 Set)
{
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	SetIndices[Set] += 1;
}

void directx12_resource_binder::
SetSampledImage(u32 BindingCount, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap->GetGpuHandle(SamplersBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap->GetCpuHandle(SamplersBindingIdx);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap->Type);
		SamplersBindingIdx++;
	}
	SamplersBindingIdx += (BindingCount - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_resource_binder::
SetStorageImage(u32 BindingCount, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	BindingDescriptions.push_back({dx12_descriptor_type::unordered_access_table, ResourceHeap->GetGpuHandle(ResourceBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap->GetCpuHandle(ResourceBindingIdx);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->UnorderedAccessViews[ViewIdx], ResourceHeap->Type);
		ResourceBindingIdx++;
	}
	ResourceBindingIdx += (BindingCount - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_resource_binder::
SetImageSampler(u32 BindingCount, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx, u32 Set)
{
	BindingDescriptions.push_back({dx12_descriptor_type::shader_resource_table, ResourceHeap->GetGpuHandle(ResourceBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap->GetCpuHandle(ResourceBindingIdx);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->ShaderResourceViews[ViewIdx], ResourceHeap->Type);
		ResourceBindingIdx++;
	}
	ResourceBindingIdx += (BindingCount - Textures.size());

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap->GetGpuHandle(SamplersBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap->GetCpuHandle(SamplersBindingIdx);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap->Type);
		SamplersBindingIdx++;
	}
	SamplersBindingIdx += (BindingCount - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_command_list::
CreateResource(renderer_backend* Backend)
{
	Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();

	CommandList = Gfx->CommandQueue->AllocateCommandList();
}

void directx12_command_list::
DestroyObject()
{
	Gfx->Fence->Flush(Gfx->CommandQueue);
	if(CommandList)
	{
		Gfx->CommandQueue->Remove(&CommandList);
		CommandList = nullptr;
	}
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
PlaceEndOfFrameBarriers()
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	if(Gfx->SwapchainCurrentState[BackBufferIndex] != D3D12_RESOURCE_STATE_COMMON)
	{
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Gfx->SwapchainImages[BackBufferIndex].Get(), Gfx->SwapchainCurrentState[BackBufferIndex], D3D12_RESOURCE_STATE_COMMON));
		Gfx->SwapchainCurrentState[BackBufferIndex] = D3D12_RESOURCE_STATE_COMMON;
	}

#ifdef CE_DEBUG
	// NOTE: This will be needed only when PIX capture will be acquired
	{
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

			bool AreAllSubresourcesInSameState = true;
			for(u32 MipIdx = 1; MipIdx < Resource->Info.MipLevels; ++MipIdx)
			{
				if(Resource->CurrentLayout[0] != Resource->CurrentLayout[MipIdx])
					AreAllSubresourcesInSameState = false;
			}

			if(AreAllSubresourcesInSameState)
			{
				if(GetDXImageLayout(Resource->CurrentState[0], Resource->CurrentLayout[0], Resource->PrevShader) != D3D12_RESOURCE_STATE_COMMON)
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource->Handle.Get(), GetDXImageLayout(Resource->CurrentState[0], Resource->CurrentLayout[0], Resource->PrevShader), D3D12_RESOURCE_STATE_COMMON));

			}
			else
			{
				for(u32 MipIdx = 0; MipIdx < Resource->Info.MipLevels; ++MipIdx)
				{
					if(GetDXImageLayout(Resource->CurrentState[MipIdx], Resource->CurrentLayout[MipIdx], Resource->PrevShader) != D3D12_RESOURCE_STATE_COMMON)
					Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource->Handle.Get(), GetDXImageLayout(Resource->CurrentState[MipIdx], Resource->CurrentLayout[MipIdx], Resource->PrevShader), D3D12_RESOURCE_STATE_COMMON, MipIdx));
				}
			}

			std::fill(Resource->CurrentLayout.begin(), Resource->CurrentLayout.end(), 0);
			std::fill(Resource->CurrentState.begin(), Resource->CurrentState.end(), barrier_state::general);
			Resource->PrevShader = 0;
		}
	}
#endif
	if(Barriers.size())
		CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	BuffersToCommon.clear();
	TexturesToCommon.clear();
}

void directx12_command_list::
End()
{
	PlaceEndOfFrameBarriers();
	Gfx->CommandQueue->Execute(&CommandList);
	Gfx->Fence->Flush(Gfx->CommandQueue);
}

void directx12_command_list::
EndOneTime()
{
	PlaceEndOfFrameBarriers();
	Gfx->CommandQueue->ExecuteAndRemove(&CommandList);
	Gfx->Fence->Flush(Gfx->CommandQueue);
}

void directx12_command_list::
Present()
{
	PlaceEndOfFrameBarriers();
	Gfx->CommandQueue->Execute(&CommandList);
	Gfx->Fence->Flush(Gfx->CommandQueue);

	Gfx->SwapChain->Present(0, 0);
	BackBufferIndex = Gfx->SwapChain->GetCurrentBackBufferIndex();
}

void directx12_command_list::
Update(buffer* BufferToUpdate, void* Data)
{
	directx12_buffer* Buffer = static_cast<directx12_buffer*>(BufferToUpdate);
	SetBufferBarriers({{BufferToUpdate, AF_TransferWrite, PSF_Transfer}});

	void* CpuPtr = nullptr;
	Buffer->TempHandle->Map(0, nullptr, &CpuPtr);
	memcpy(CpuPtr, Data, Buffer->Size);
	Buffer->TempHandle->Unmap(0, 0);

	CommandList->CopyResource(Buffer->Handle.Get(), Buffer->TempHandle.Get());
}

void directx12_command_list::
UpdateSize(buffer* BufferToUpdate, void* Data, u32 UpdateByteSize)
{
	directx12_buffer* Buffer = static_cast<directx12_buffer*>(BufferToUpdate);

	if(UpdateByteSize == 0) return;
	assert(UpdateByteSize <= Buffer->Size);

	SetBufferBarriers({{BufferToUpdate, AF_TransferWrite, PSF_Transfer}});

	void* CpuPtr = nullptr;
	Buffer->TempHandle->Map(0, nullptr, &CpuPtr);
	memcpy(CpuPtr, Data, UpdateByteSize);
	Buffer->TempHandle->Unmap(0, 0);

	CommandList->CopyBufferRegion(Buffer->Handle.Get(), 0, Buffer->TempHandle.Get(), 0, UpdateByteSize);
}

void directx12_command_list::
ReadBack(buffer* BufferToRead, void* Data)
{
	directx12_buffer* Buffer = static_cast<directx12_buffer*>(BufferToRead);
	SetBufferBarriers({{BufferToRead, AF_TransferRead, PSF_Transfer}});

	CommandList->CopyBufferRegion(Buffer->TempHandle.Get(), 0, Buffer->Handle.Get(), 0, Buffer->Size);

	void* CpuPtr = nullptr;
	Buffer->TempHandle->Map(0, nullptr, &CpuPtr);
	memcpy(Data, CpuPtr, Buffer->Size);
	Buffer->TempHandle->Unmap(0, 0);
}

void directx12_command_list::
ReadBackSize(buffer* BufferToRead, void* Data, u32 UpdateByteSize)
{
	directx12_buffer* Buffer = static_cast<directx12_buffer*>(BufferToRead);
	SetBufferBarriers({{BufferToRead, AF_TransferRead, PSF_Transfer}});

	CommandList->CopyBufferRegion(Buffer->TempHandle.Get(), 0, Buffer->Handle.Get(), 0, UpdateByteSize);

	void* CpuPtr = nullptr;
	Buffer->TempHandle->Map(0, nullptr, &CpuPtr);
	memcpy(Data, CpuPtr, UpdateByteSize);
	Buffer->TempHandle->Unmap(0, 0);
}

void directx12_command_list::
Update(texture* TextureToUpdate, void* Data)
{
	directx12_texture* Texture = static_cast<directx12_texture*>(TextureToUpdate);
	SetImageBarriers({{TextureToUpdate, AF_TransferWrite, barrier_state::transfer_dst, SUBRESOURCES_ALL, PSF_Transfer}});

	D3D12_SUBRESOURCE_FOOTPRINT SubresourceDesc = {};
	SubresourceDesc.Format   = GetDXFormat(Texture->Info.Format);
	SubresourceDesc.Width    = Texture->Width;
	SubresourceDesc.Height   = Texture->Height;
	SubresourceDesc.Depth    = Texture->Depth;
	SubresourceDesc.RowPitch = AlignUp(Texture->Width * GetPixelSize(Texture->Info.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	void* CpuPtr = nullptr;
	Texture->TempHandle->Map(0, nullptr, &CpuPtr);

	for (u32 z = 0; z < Texture->Depth;  z++)
	{
		for (u32 y = 0; y < Texture->Height; y++)
		{
		  u8* Dst = (u8*)(CpuPtr) + y * SubresourceDesc.RowPitch + z * Texture->Height * SubresourceDesc.RowPitch;
		  u8* Src = (u8*)(Data)   + y * Texture->Width * GetPixelSize(Texture->Info.Format) + z * Texture->Width * Texture->Height * GetPixelSize(Texture->Info.Format);
		  memcpy(Dst, Src, GetPixelSize(Texture->Info.Format) * Texture->Width);
		}
	}

	Texture->TempHandle->Unmap(0, 0);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT TextureFootprint = {};
	TextureFootprint.Footprint = SubresourceDesc;

	D3D12_TEXTURE_COPY_LOCATION SrcCopyLocation = {};
	SrcCopyLocation.pResource = Texture->TempHandle.Get();
	SrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	SrcCopyLocation.PlacedFootprint = TextureFootprint;

	D3D12_TEXTURE_COPY_LOCATION DstCopyLocation = {};
	DstCopyLocation.pResource = Texture->Handle.Get();
	DstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	DstCopyLocation.SubresourceIndex = 0;

	CommandList->CopyTextureRegion(&DstCopyLocation, 0, 0, 0, 
								   &SrcCopyLocation, nullptr);
}

void directx12_command_list::
ReadBack(texture* TextureToRead, void* Data)
{
	directx12_texture* Texture = static_cast<directx12_texture*>(TextureToRead);
	SetImageBarriers({{TextureToRead, AF_TransferRead, barrier_state::transfer_src, SUBRESOURCES_ALL, PSF_Transfer}});

	CommandList->CopyResource(Texture->TempHandle.Get(), Texture->Handle.Get());

	void* CpuPtr = nullptr;
	Texture->TempHandle->Map(0, nullptr, &CpuPtr);
	memcpy(Data, CpuPtr, Texture->Width * Texture->Height * Texture->Depth * GetPixelSize(Texture->Info.Format));
	Texture->TempHandle->Unmap(0, 0);
}

void directx12_command_list::
SetGraphicsPipelineState(render_context* Context)
{
	CurrentContext = Context;
	directx12_render_context* ContextToBind = static_cast<directx12_render_context*>(Context);
	CommandList->SetGraphicsRootSignature(ContextToBind->RootSignatureHandle.Get());
	CommandList->SetPipelineState(ContextToBind->Pipeline.Get());
	CommandList->IASetPrimitiveTopology(ContextToBind->PipelineTopology);
}

void directx12_command_list::
SetComputePipelineState(compute_context* Context)
{
	CurrentContext = Context;
	directx12_compute_context* ContextToBind = static_cast<directx12_compute_context*>(Context);
	CommandList->SetComputeRootSignature(ContextToBind->RootSignatureHandle.Get());
	CommandList->SetPipelineState(ContextToBind->Pipeline.Get());
}

void directx12_command_list::
SetConstant(void* Data, size_t Size)
{
	if(CurrentContext->Type == pass_type::graphics)
	{
		directx12_render_context* ContextToBind = static_cast<directx12_render_context*>(CurrentContext);
		assert(ContextToBind->HavePushConstant);
		CommandList->SetGraphicsRoot32BitConstants(ContextToBind->PushConstantOffset, Size / sizeof(u32), Data, 0);
	}
	else if(CurrentContext->Type == pass_type::compute)
	{
		directx12_compute_context* ContextToBind = static_cast<directx12_compute_context*>(CurrentContext);
		assert(ContextToBind->HavePushConstant);
		CommandList->SetComputeRoot32BitConstants(ContextToBind->PushConstantOffset, Size / sizeof(u32), Data, 0);
	}
}

void directx12_command_list::
SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight)
{
	D3D12_VIEWPORT Viewport = {(r32)StartX, (r32)(RenderHeight - StartY), (r32)RenderWidth, -(r32)RenderHeight, 0, 1};
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
SetColorTarget(const std::vector<texture*>& ColorAttachments, vec4 Clear)
{
	assert(CurrentContext->Type == pass_type::graphics);
	directx12_render_context* Context = static_cast<directx12_render_context*>(CurrentContext);

#if 0
	if(EnableMultiview)
	{
		directx12_texture* Attachment = static_cast<directx12_texture*>(ColorAttachments[0]);
		TexturesToCommon.insert(Attachment);
		ColorTargets.push_back(Attachment->RenderTargetViews[Face]);
		if(LoadOp == load_op::clear)
		{
			CommandList->ClearRenderTargetView(ColorTargets[0], Clear.E, 0, nullptr);
		}
	}
	else
	{
#endif
		for(u32 i = 0; i < ColorAttachments.size(); i++)
		{
			texture* ColorTarget = ColorAttachments[i];
			directx12_texture* Attachment = static_cast<directx12_texture*>(ColorTarget);
			SetImageBarriers({{ColorTarget, AF_ColorAttachmentWrite, barrier_state::color_attachment, SUBRESOURCES_ALL, PSF_ColorAttachment}});
			TexturesToCommon.insert(Attachment);
			ColorTargets.push_back(Attachment->RenderTargetViews[0]);
			if(Context->LoadOp == load_op::clear)
			{
				CommandList->ClearRenderTargetView(ColorTargets[i], Clear.E, 0, nullptr);
			}
		}
#if 0
	}
#endif
}

void directx12_command_list::
SetDepthTarget(texture* DepthAttachment, vec2 Clear)
{
	assert(CurrentContext->Type == pass_type::graphics);
	directx12_render_context* Context = static_cast<directx12_render_context*>(CurrentContext);

	directx12_texture* Attachment = static_cast<directx12_texture*>(DepthAttachment);
	SetImageBarriers({{DepthAttachment, AF_DepthStencilAttachmentWrite, barrier_state::depth_stencil_attachment, SUBRESOURCES_ALL, PSF_EarlyFragment}});
	TexturesToCommon.insert(Attachment);

	DepthStencilTarget = Attachment->DepthStencilViews[0];
	if(Context->LoadOp == load_op::clear)
	{
		CommandList->ClearDepthStencilView(DepthStencilTarget, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Clear.x, Clear.y, 0, nullptr);
	}
}

void directx12_command_list::
SetStencilTarget(texture* StencilAttachment, vec2 Clear)
{
	assert(CurrentContext->Type == pass_type::graphics);
	directx12_render_context* Context = static_cast<directx12_render_context*>(CurrentContext);

#if 0
	directx12_texture* Attachment = static_cast<directx12_texture*>(StencilAttachment);
	TexturesToCommon.insert(Attachment);

	DepthStencilTarget = Attachment->DepthStencilViews[0];
	if(Context->LoadOp == load_op::clear)
	{
		CommandList->ClearDepthStencilView(DepthStencilTarget, D3D12_CLEAR_FLAG_STENCIL, Clear.x, Clear.y, 0, nullptr);
	}
	std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), AF_DepthStencilAttachmentWrite);
	std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), barrier_state::depth_stencil_attachment);
	Texture->PrevShader = PSF_EarlyFragment;
#endif
};

void directx12_command_list::
BindShaderParameters(const array<binding_packet>& Data)
{
	if(!Data.size()) return;
    directx12_resource_binder Binder(Gfx, CurrentContext);

	u32 Offset = 0;
    auto DX12BindImageSampler = [&](const binding_packet& Packet, const descriptor_param& Parameter, u32 LayoutIdx) 
	{
        Binder.SetImageSampler(Parameter.Count, Packet.Resources, Parameter.ImageType, Parameter.BarrierState, Packet.SubresourceIndex, LayoutIdx);

        for (resource* CurrentResource : Packet.Resources) 
		{
			texture* CurrentTexture = (texture*)CurrentResource;
            TexturesToCommon.insert(CurrentTexture);
            AttachmentImageBarriers.push_back({CurrentTexture, Parameter.AspectMask, Parameter.BarrierState, Packet.Mips, Parameter.ShaderToUse});
        }
    };

    auto DX12BindStorageTexture = [&](const binding_packet& Packet, const descriptor_param& Parameter, u32 LayoutIdx) 
	{
        Binder.SetStorageImage(Parameter.Count, Packet.Resources, Parameter.ImageType, Parameter.BarrierState, Packet.SubresourceIndex, LayoutIdx);

        for (resource* CurrentResource : Packet.Resources) 
		{
			texture* CurrentTexture = (texture*)CurrentResource;
            TexturesToCommon.insert(CurrentTexture);
            AttachmentImageBarriers.push_back({CurrentTexture, Parameter.AspectMask, Parameter.BarrierState, Packet.Mips, Parameter.ShaderToUse});
        }
    };

    auto DX12BindBuffer = [&](buffer* BufferToBind, const descriptor_param& Parameter) 
	{
        Binder.SetStorageBufferView(BufferToBind);
        BuffersToCommon.insert(BufferToBind);
        AttachmentBufferBarriers.push_back({BufferToBind, Parameter.AspectMask, Parameter.ShaderToUse});
    };

    auto DX12SetRootDescriptors = [&](auto* ContextToBind, auto SetDescriptorTable, auto SetShaderResourceView, auto SetUnorderedAccessView) 
	{
        ContextToBind->ResourceBindingIdx = Binder.ResourceBindingIdx;
        ContextToBind->SamplersBindingIdx = Binder.SamplersBindingIdx;

        std::vector<ID3D12DescriptorHeap*> Heaps;
        if (ContextToBind->IsResourceHeapInited) Heaps.push_back(ContextToBind->ResourceHeap->Handle.Get());
        if (ContextToBind->IsSamplersHeapInited) Heaps.push_back(ContextToBind->SamplersHeap->Handle.Get());
        CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());

        for (const auto& BindingDesc : Binder.BindingDescriptions) 
		{
            switch (BindingDesc.Type) 
			{
                case dx12_descriptor_type::shader_resource_table:
                case dx12_descriptor_type::unordered_access_table:
                case dx12_descriptor_type::constant_buffer_table:
                case dx12_descriptor_type::sampler:
                    (CommandList->*SetDescriptorTable)(BindingDesc.Idx, BindingDesc.TableBegin);
                    break;
                case dx12_descriptor_type::shader_resource:
                    (CommandList->*SetShaderResourceView)(BindingDesc.Idx, BindingDesc.ResourceBegin);
                    break;
                case dx12_descriptor_type::unordered_access:
                    (CommandList->*SetUnorderedAccessView)(BindingDesc.Idx, BindingDesc.ResourceBegin);
                    break;
            }
        }
    };

    for (u32 ParamIdx = 0; ParamIdx < CurrentContext->ParameterLayout[0].size(); ++ParamIdx, ++Offset) 
	{
        const auto& Parameter = CurrentContext->ParameterLayout[0][ParamIdx];
        if (Parameter.Type == resource_type::buffer_storage || Parameter.Type == resource_type::buffer_uniform) 
		{
			buffer* BufferToBind = (buffer*)Data[Offset].Resource;
            ParamIdx += BufferToBind->WithCounter;
        } 
    }

	for (u32 LayoutIdx = 1; LayoutIdx < CurrentContext->ParameterLayout.size(); ++LayoutIdx) 
	{
		for(u32 ParamIdx = 0; ParamIdx < CurrentContext->ParameterLayout[LayoutIdx].size(); ++ParamIdx, ++Offset)
		{
			const descriptor_param& Parameter = CurrentContext->ParameterLayout[LayoutIdx][ParamIdx];
			if(Parameter.Type == resource_type::buffer_storage || Parameter.Type == resource_type::buffer_uniform)
			{
				assert(false && "Buffer in static storage. Currently is not available, use buffers in the inputs");
			}
			else if (Parameter.Type == resource_type::texture_sampler) 
			{
				DX12BindImageSampler(Data[Offset], Parameter, LayoutIdx);
			}
			else if(Parameter.Type == resource_type::texture_storage)
			{
				assert(false && "Storage image in static storage. Check the shader bindings. Could be image sampler or buffer");
			}
		}
	}

	Offset = 0;
    for (u32 ParamIdx = 0; ParamIdx < CurrentContext->ParameterLayout[0].size(); ++ParamIdx, ++Offset) 
	{
        const auto& Parameter = CurrentContext->ParameterLayout[0][ParamIdx];
        if (Parameter.Type == resource_type::buffer_storage || Parameter.Type == resource_type::buffer_uniform) 
		{
			buffer* BufferToBind = (buffer*)Data[Offset].Resource;
            DX12BindBuffer(BufferToBind, Parameter);

            ParamIdx += BufferToBind->WithCounter;
        } 
		else if (Parameter.Type == resource_type::texture_sampler)
		{
            DX12BindImageSampler(Data[Offset], Parameter, 0);
        }
		else if (Parameter.Type == resource_type::texture_storage)
		{
            DX12BindStorageTexture(Data[Offset], Parameter, 0);
        }
    }

    if (CurrentContext->Type == pass_type::graphics) 
	{
        DX12SetRootDescriptors(
            static_cast<directx12_render_context*>(CurrentContext),
            &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable,
            &ID3D12GraphicsCommandList::SetGraphicsRootShaderResourceView,
            &ID3D12GraphicsCommandList::SetGraphicsRootUnorderedAccessView
        );
    } 
	else if (CurrentContext->Type == pass_type::compute) 
	{
        DX12SetRootDescriptors(
            static_cast<directx12_compute_context*>(CurrentContext),
            &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable,
            &ID3D12GraphicsCommandList::SetComputeRootShaderResourceView,
            &ID3D12GraphicsCommandList::SetComputeRootUnorderedAccessView
        );
    }
}

void directx12_command_list::
DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount)
{
	assert(CurrentContext->Type == pass_type::graphics);
	directx12_render_context* Context = static_cast<directx12_render_context*>(CurrentContext);

	SetBufferBarriers(AttachmentBufferBarriers);
	SetImageBarriers(AttachmentImageBarriers);

	CommandList->OMSetRenderTargets(ColorTargets.size(), ColorTargets.data(), Context->Info.UseDepth, Context->Info.UseDepth ? &DepthStencilTarget : nullptr);

	CommandList->DrawIndexedInstanced(IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);

	ColorTargets.clear();
	DepthStencilTarget = {};

	AttachmentImageBarriers.clear();
	AttachmentBufferBarriers.clear();
}

void directx12_command_list::
DrawIndirect(buffer* IndirectCommands, u32 ObjectDrawCount, u32 CommandStructureSize)
{
	assert(CurrentContext->Type == pass_type::graphics);
	directx12_render_context* Context = static_cast<directx12_render_context*>(CurrentContext);

	BuffersToCommon.insert(IndirectCommands);
	AttachmentBufferBarriers.push_back({IndirectCommands, AF_IndirectCommandRead, PSF_DrawIndirect});

	SetBufferBarriers(AttachmentBufferBarriers);
	SetImageBarriers(AttachmentImageBarriers);

	CommandList->OMSetRenderTargets(ColorTargets.size(), ColorTargets.data(), Context->Info.UseDepth, Context->Info.UseDepth ? &DepthStencilTarget : nullptr);

	directx12_buffer* Indirect = static_cast<directx12_buffer*>(IndirectCommands);
	CommandList->ExecuteIndirect(Context->IndirectSignatureHandle.Get(), ObjectDrawCount, Indirect->Handle.Get(), !Context->HaveDrawID * 4, Indirect->CounterHandle.Get(), 0);

	ColorTargets.clear();
	DepthStencilTarget = {};

	AttachmentImageBarriers.clear();
	AttachmentBufferBarriers.clear();
}

void directx12_command_list::
Dispatch(u32 X, u32 Y, u32 Z)
{
	assert(CurrentContext->Type == pass_type::compute);

	SetBufferBarriers(AttachmentBufferBarriers);
	SetImageBarriers(AttachmentImageBarriers);

	directx12_compute_context* Context = static_cast<directx12_compute_context*>(CurrentContext);
	CommandList->Dispatch((X + Context->BlockSizeX - 1) / Context->BlockSizeX, (Y + Context->BlockSizeY - 1) / Context->BlockSizeY, (Z + Context->BlockSizeZ - 1) / Context->BlockSizeZ);

	AttachmentImageBarriers.clear();
	AttachmentBufferBarriers.clear();
}

void directx12_command_list::
FillBuffer(buffer* Buffer, u32 Value)
{
	std::vector<u32> Fill(Buffer->Size / sizeof(u32), Value);
	Buffer->Update(Fill.data(), this);
}

void directx12_command_list::
FillTexture(texture* Texture, vec4 Value)
{
	std::vector<vec4> Fill(Texture->Size / sizeof(vec4), Value);
	Texture->Update(Fill.data(), this);
}

void directx12_command_list::
FillTexture(texture* Texture, float Depth, u32 Stencil)
{
}

void directx12_command_list::
CopyImage(texture* Dst, texture* Src)
{
}

void directx12_command_list::
SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> TransitionBarriers;
	std::vector<CD3DX12_RESOURCE_BARRIER> UavBarriers;

	for(const std::tuple<buffer*, u32, u32>& Data : BarrierData)
	{
		directx12_buffer* Buffer = static_cast<directx12_buffer*>(std::get<0>(Data));

		u32 ResourceLayoutPrev = Buffer->CurrentLayout;
		u32 ResourceLayoutNext = std::get<1>(Data);
		u32 BufferPrevShader = Buffer->PrevShader;
		u32 BufferNextShader = std::get<2>(Data);
		Buffer->PrevShader = BufferNextShader;
		Buffer->CurrentLayout = ResourceLayoutNext;

		D3D12_RESOURCE_STATES CurrState = GetDXBufferLayout(ResourceLayoutPrev, BufferPrevShader);
		D3D12_RESOURCE_STATES NextState = GetDXBufferLayout(ResourceLayoutNext, BufferNextShader);

		if(CurrState != NextState)
		{
			TransitionBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Buffer->Handle.Get(), CurrState, NextState));
			if(Buffer->WithCounter)
				TransitionBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Buffer->CounterHandle.Get(), CurrState, NextState));
		}
		else if(Buffer->Usage & RF_StorageBuffer)
		{
			UavBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer->Handle.Get()));
			if(Buffer->WithCounter)
				UavBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer->CounterHandle.Get()));
		}
	}

	if(TransitionBarriers.size())
		CommandList->ResourceBarrier(TransitionBarriers.size(), TransitionBarriers.data());
	if(UavBarriers.size())
		CommandList->ResourceBarrier(UavBarriers.size(), UavBarriers.data());
}

void directx12_command_list::
SetImageBarriers(const std::vector<std::tuple<texture*, u32, barrier_state, u32, u32>>& BarrierData)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> TransitionBarriers;
	std::vector<CD3DX12_RESOURCE_BARRIER> UavBarriers;

	for(const std::tuple<texture*, u32, barrier_state, u32, u32>& Data : BarrierData)
	{
		directx12_texture* Texture = static_cast<directx12_texture*>(std::get<0>(Data));

		u32 ResourceLayoutNext = std::get<1>(Data);
		barrier_state ResourceStateNext = std::get<2>(Data);
		u32 TexturePrevShader = Texture->PrevShader;
		u32 TextureNextShader = std::get<4>(Data);
		Texture->PrevShader = TextureNextShader;

		u32 MipToUse = std::get<3>(Data);
		if(MipToUse == SUBRESOURCES_ALL)
		{
			bool AreAllSubresourcesInSameState = true;
			for(u32 MipIdx = 1; MipIdx < Texture->Info.MipLevels; ++MipIdx)
			{
				if(Texture->CurrentLayout[0] != Texture->CurrentLayout[MipIdx])
					AreAllSubresourcesInSameState = false;
			}

			if(AreAllSubresourcesInSameState)
			{
				D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(Texture->CurrentState[0], Texture->CurrentLayout[0], TexturePrevShader);
				D3D12_RESOURCE_STATES NextState = GetDXImageLayout(ResourceStateNext, ResourceLayoutNext, TextureNextShader);

				if(CurrState != NextState)
					TransitionBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), CurrState, NextState, MipToUse));
				else if(Texture->Info.Usage & TF_Storage)
					UavBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));
			}
			else
			{
				for(u32 MipIdx = 0; MipIdx < Texture->Info.MipLevels; ++MipIdx)
				{
					D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(Texture->CurrentState[MipIdx], Texture->CurrentLayout[MipIdx], TexturePrevShader);
					D3D12_RESOURCE_STATES NextState = GetDXImageLayout(ResourceStateNext, ResourceLayoutNext, TextureNextShader);

					if(CurrState != NextState)
						TransitionBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), CurrState, NextState, MipIdx));
					else if(Texture->Info.Usage & TF_Storage)
						UavBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));
				}
			}

			std::fill(Texture->CurrentLayout.begin(), Texture->CurrentLayout.end(), ResourceLayoutNext);
			std::fill(Texture->CurrentState.begin(), Texture->CurrentState.end(), ResourceStateNext);
		}
		else
		{
			D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(Texture->CurrentState[MipToUse], Texture->CurrentLayout[MipToUse], TexturePrevShader);
			D3D12_RESOURCE_STATES NextState = GetDXImageLayout(ResourceStateNext, ResourceLayoutNext, TextureNextShader);

			if(CurrState != NextState)
				TransitionBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), CurrState, NextState, MipToUse));
			else if(Texture->Info.Usage & TF_Storage)
				UavBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));

			Texture->CurrentLayout[MipToUse] = ResourceLayoutNext;
			Texture->CurrentState[MipToUse] = ResourceStateNext;
		}
	}

	if(TransitionBarriers.size())
		CommandList->ResourceBarrier(TransitionBarriers.size(), TransitionBarriers.data());
	if(UavBarriers.size())
		CommandList->ResourceBarrier(UavBarriers.size(), UavBarriers.data());
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
	Heaps.push_back(Gfx->ImGuiResourcesHeap->Handle.Get());

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
	Type = pass_type::graphics;
	Info = InputData;

	D3D12_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	RasterDesc.CullMode = GetDXCullMode(InputData.CullMode);
	RasterDesc.FrontCounterClockwise = true;
	RasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	RasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterDesc.DepthClipEnable = InputData.UseDepth;
	//RasterDesc.MultisampleEnable = MsaaState;
	//RasterDesc.AntialiasedLineEnable = MsaaState;
	RasterDesc.ForcedSampleCount = 0;
	RasterDesc.ConservativeRaster = InputData.UseConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = InputData.UseDepth;
	DepthStencilDesc.DepthWriteMask = InputData.UseDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.DepthFunc = InputData.UseDepth ? D3D12_COMPARISON_FUNC_LESS : D3D12_COMPARISON_FUNC_ALWAYS;
	DepthStencilDesc.StencilEnable = false;
	DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
	DepthStencilDesc.BackFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDesc = {};

	std::map<u32, std::map<u32, descriptor_param>> ParameterLayoutTemp;
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
			PipelineDesc.VS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::vertex, HaveDrawID, ParameterLayoutTemp, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			Shader.replace(Shader.find("glsl"), 4, "hlsl");
			PipelineDesc.DS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_control, HaveDrawID, ParameterLayoutTemp, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			Shader.replace(Shader.find("glsl"), 4, "hlsl");
			PipelineDesc.HS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_eval, HaveDrawID, ParameterLayoutTemp, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			Shader.replace(Shader.find("glsl"), 4, "hlsl");
			PipelineDesc.GS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::geometry, HaveDrawID, ParameterLayoutTemp, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			PipelineDesc.PS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::fragment, HaveDrawID, ParameterLayoutTemp, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
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

	for(u32 LayoutIdx = 0; LayoutIdx < ParameterLayoutTemp.size(); ++LayoutIdx)
	{
		for(u32 ParamIdx = 0; ParamIdx < ParameterLayoutTemp[LayoutIdx].size(); ++ParamIdx)
		{
			ParameterLayout[LayoutIdx].push_back(ParameterLayoutTemp[LayoutIdx][ParamIdx]);
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

	PushConstantOffset = Parameters.size() - HavePushConstant;

	size_t LastSlashPos = GlobalName.find_last_of('/');
    size_t LastDotPos = GlobalName.find_last_of('.');
    Name = GlobalName = GlobalName.substr(LastSlashPos + 1, LastDotPos - LastSlashPos - 1);

	// TODO: TEMPORAL HACK. Need a better solution here for different tasks
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != 0)
	{
		//ResourceHeap = descriptor_heap(Device, Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * 16, 16384u), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		ResourceHeap = new descriptor_heap(Device, 16384u, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(ResourceHeap->Handle.Get(), (GlobalName + ".resource_heap").c_str());
		IsResourceHeapInited = true;
	}
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != 0)
	{
		//SamplersHeap = descriptor_heap(Device, Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] * 16, 2048u), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		SamplersHeap = new descriptor_heap(Device, 2048u, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(SamplersHeap->Handle.Get(), (GlobalName + ".samplers_heap").c_str());
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

	Res = Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&Pipeline));

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
	Device->CreateCommandSignature(&IndirectDesc, HaveDrawID ? RootSignatureHandle.Get() : nullptr, IID_PPV_ARGS(&IndirectSignatureHandle));
}

directx12_compute_context::
directx12_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();
	Type = pass_type::compute;

	std::map<u32, std::map<u32, descriptor_param>> ParameterLayoutTemp;
	std::unordered_map<u32, u32> DescriptorHeapSizes;
	D3D12_ROOT_PARAMETER PushConstantDesc = {};
	PushConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;

	bool HaveDrawID = false;
	D3D12_COMPUTE_PIPELINE_STATE_DESC PipelineDesc = {};
	std::map<u32, std::map<u32, u32>> NewBindings;
	PipelineDesc.CS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::compute, HaveDrawID, ParameterLayoutTemp, NewBindings, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines, &BlockSizeX, &BlockSizeY, &BlockSizeZ);

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

	for(u32 LayoutIdx = 0; LayoutIdx < ParameterLayoutTemp.size(); ++LayoutIdx)
	{
		for(u32 ParamIdx = 0; ParamIdx < ParameterLayoutTemp[LayoutIdx].size(); ++ParamIdx)
		{
			ParameterLayout[LayoutIdx].push_back(ParameterLayoutTemp[LayoutIdx][ParamIdx]);
		}
	}

	if(HavePushConstant)
	{
		PushConstantDesc.Constants.Num32BitValues = PushConstantSize / sizeof(u32);
		PushConstantDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameters.push_back(PushConstantDesc);
	}

	PushConstantOffset = Parameters.size() - HavePushConstant;

	size_t LastSlashPos = Shader.find_last_of('/');
    size_t LastDotPos = Shader.find_last_of('.');
	Name = Shader.substr(LastSlashPos + 1, LastDotPos - LastSlashPos - 1);

	// TODO: TEMPORAL HACK. Need a better solution here for different tasks
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != 0)
	{
		ResourceHeap = new descriptor_heap(Device, Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * 16, 16384u), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(ResourceHeap->Handle.Get(), (Name + ".resource_heap").c_str());
		IsResourceHeapInited = true;
	}
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != 0)
	{
		SamplersHeap = new descriptor_heap(Device, Min(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] * 16, 2048u), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(SamplersHeap->Handle.Get(), (Name + ".samplers_heap").c_str());
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
