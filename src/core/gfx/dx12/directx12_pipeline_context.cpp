
void directx12_global_pipeline_context::
CreateResource(renderer_backend* Backend)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();

	Fence.Init(Gfx->Device.Get());

	CommandList = Gfx->CommandQueue->AllocateCommandList();
}

void directx12_global_pipeline_context::
Begin(renderer_backend* Backend)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);

	Gfx->CommandQueue->Reset();
	CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);
}

void directx12_global_pipeline_context::
End(renderer_backend* Backend)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);

	BuffersToCommon.clear();
	Gfx->CommandQueue->Execute(CommandList);
	Fence.Flush(Gfx->CommandQueue);

	HRESULT DeviceResult = Gfx->Device->GetDeviceRemovedReason();
	if(!SUCCEEDED(DeviceResult))
	{
		printf("%#08x\n", DeviceResult);
	}
}

void directx12_global_pipeline_context::
EndOneTime(renderer_backend* Backend)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);

	BuffersToCommon.clear();
	Gfx->CommandQueue->Execute(CommandList);
	Fence.Flush(Gfx->CommandQueue);
}

void directx12_global_pipeline_context::
EmplaceColorTarget(renderer_backend* Backend, texture* RenderTexture)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	directx12_texture* Texture = static_cast<directx12_texture*>(RenderTexture);

	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), Texture->CurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(Gfx->SwapchainImages[BackBufferIndex].Get(), Gfx->SwapchainCurrentState[BackBufferIndex], D3D12_RESOURCE_STATE_COPY_DEST)
	};

	Texture->CurrentState                       = D3D12_RESOURCE_STATE_COPY_SOURCE;
	Gfx->SwapchainCurrentState[BackBufferIndex] = D3D12_RESOURCE_STATE_COPY_DEST;
	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	CommandList->CopyResource(Gfx->SwapchainImages[BackBufferIndex].Get(), Texture->Handle.Get());
}

void directx12_global_pipeline_context::
Present(renderer_backend* Backend)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);

	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(Gfx->SwapchainImages[BackBufferIndex].Get(), Gfx->SwapchainCurrentState[BackBufferIndex], D3D12_RESOURCE_STATE_COMMON)
	};
	Gfx->SwapchainCurrentState[BackBufferIndex] = D3D12_RESOURCE_STATE_COMMON;

	for(u32 Idx = 0; Idx < BuffersToCommon.size(); ++Idx)
	{
		directx12_buffer* Resource = static_cast<directx12_buffer*>(*std::next(BuffersToCommon.begin(), Idx));
		if(Resource->CurrentState == D3D12_RESOURCE_STATE_COMMON) continue;
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource->Handle.Get(), Resource->CurrentState, D3D12_RESOURCE_STATE_COMMON));
		Resource->CurrentState = D3D12_RESOURCE_STATE_COMMON;
	}

	for(u32 Idx = 0; Idx < TexturesToCommon.size(); ++Idx)
	{
		directx12_texture* Resource = static_cast<directx12_texture*>(*std::next(TexturesToCommon.begin(), Idx));
		if(Resource->CurrentState == D3D12_RESOURCE_STATE_COMMON) continue;
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource->Handle.Get(), Resource->CurrentState, D3D12_RESOURCE_STATE_COMMON));
		Resource->CurrentState = D3D12_RESOURCE_STATE_COMMON;
	}

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	Gfx->CommandQueue->Execute(CommandList);
	Fence.Flush(Gfx->CommandQueue);

	Gfx->SwapChain->Present(0, 0);
	BackBufferIndex = Gfx->SwapChain->GetCurrentBackBufferIndex();

	HRESULT DeviceResult = Gfx->Device->GetDeviceRemovedReason();
	if(!SUCCEEDED(DeviceResult))
	{
		printf("%#08x\n", DeviceResult);
	}
}

void directx12_global_pipeline_context::
FillBuffer(buffer* Buffer, u32 Value)
{
}

void directx12_global_pipeline_context::
CopyImage(texture* Dst, texture* Src)
{
}

void directx12_global_pipeline_context::
SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, u32 SrcStageMask, u32 DstStageMask)
{
}

void directx12_global_pipeline_context::
SetBufferBarrier(const std::tuple<buffer*, u32, u32>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	directx12_buffer* Buffer = static_cast<directx12_buffer*>(std::get<0>(BarrierData));
	std::vector<CD3DX12_RESOURCE_BARRIER> Barrier;

	D3D12_RESOURCE_STATES CurrState = GetDXBufferLayout(std::get<1>(BarrierData), DstStageMask);
	D3D12_RESOURCE_STATES NextState = GetDXBufferLayout(std::get<2>(BarrierData), DstStageMask);

	if(Buffer->CurrentState != NextState)
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

	Buffer->CurrentState = NextState;
	if(Barrier.size())
		CommandList->ResourceBarrier(Barrier.size(), Barrier.data());
}

void directx12_global_pipeline_context::
SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData, 
				  u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	for(const std::tuple<buffer*, u32, u32>& Data : BarrierData)
	{
		directx12_buffer* Buffer = static_cast<directx12_buffer*>(std::get<0>(Data));

		D3D12_RESOURCE_STATES CurrState = GetDXBufferLayout(std::get<1>(Data), DstStageMask);
		D3D12_RESOURCE_STATES NextState = GetDXBufferLayout(std::get<2>(Data), DstStageMask);

		if(Buffer->CurrentState != NextState)
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
		Buffer->CurrentState = NextState;
	}

	if(Barriers.size())
		CommandList->ResourceBarrier(Barriers.size(), Barriers.data());
}

void directx12_global_pipeline_context::
SetImageBarriers(const std::vector<std::tuple<texture*, u32, u32, barrier_state, barrier_state, u32>>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	for(const std::tuple<texture*, u32, u32, barrier_state, barrier_state, u32>& Data : BarrierData)
	{
		directx12_texture* Texture = static_cast<directx12_texture*>(std::get<0>(Data));

		u32 SubresourceIndex = std::get<5>(Data);
		D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(std::get<3>(Data), std::get<1>(Data), DstStageMask);
		D3D12_RESOURCE_STATES NextState = GetDXImageLayout(std::get<4>(Data), std::get<2>(Data), DstStageMask);

		if(Texture->CurrentState != NextState)
			Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), Texture->CurrentState, NextState, SubresourceIndex));
		else
			Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));

		Texture->CurrentState = NextState;
	}

	if(Barriers.size())
		CommandList->ResourceBarrier(Barriers.size(), Barriers.data());
}

void directx12_global_pipeline_context::
SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, u32, barrier_state, barrier_state, u32>>& BarrierData, 
				 u32 SrcStageMask, u32 DstStageMask)
{
	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	for(const std::tuple<std::vector<texture*>, u32, u32, barrier_state, barrier_state, u32>& Data : BarrierData)
	{
		const std::vector<texture*>& Textures = std::get<0>(Data);
		u32 SubresourceIndex = std::get<5>(Data);
		if(!Textures.size()) continue;
		for(texture* TextureData : Textures)
		{
			directx12_texture* Texture = static_cast<directx12_texture*>(TextureData);

			D3D12_RESOURCE_STATES CurrState = GetDXImageLayout(std::get<3>(Data), std::get<1>(Data), DstStageMask);
			D3D12_RESOURCE_STATES NextState = GetDXImageLayout(std::get<4>(Data), std::get<2>(Data), DstStageMask);

			if(Texture->CurrentState != NextState)
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Texture->Handle.Get(), Texture->CurrentState, NextState, SubresourceIndex));
			else
				Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->Handle.Get()));

			Texture->CurrentState = NextState;
		}
	}

	if(Barriers.size())
		CommandList->ResourceBarrier(Barriers.size(), Barriers.data());
}

void directx12_global_pipeline_context::
DebugGuiBegin(renderer_backend* Backend, texture* RenderTarget)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	directx12_texture* Clr = static_cast<directx12_texture*>(RenderTarget);

	std::vector<CD3DX12_RESOURCE_BARRIER> Barriers;
	if(Clr->CurrentState != D3D12_RESOURCE_STATE_RENDER_TARGET) 
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(Clr->Handle.Get(), Clr->CurrentState, D3D12_RESOURCE_STATE_RENDER_TARGET));
	else
		Barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(Clr->Handle.Get()));

	CommandList->ResourceBarrier(Barriers.size(), Barriers.data());

	Clr->CurrentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ImGui_ImplDX12_NewFrame();

	std::vector<ID3D12DescriptorHeap*> Heaps;
	Heaps.push_back(Gfx->ImGuiResourcesHeap.Handle.Get());

	CommandList->SetDescriptorHeaps(Heaps.size(), Heaps.data());
	CommandList->OMSetRenderTargets(1, &Clr->RenderTargetViews[0], false, nullptr);
}

void directx12_global_pipeline_context::
DebugGuiEnd(renderer_backend* Backend) 
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);
}

directx12_render_context::
directx12_render_context(renderer_backend* Backend, load_op NewLoadOp, store_op NewStoreOp, std::initializer_list<const std::string> ShaderList, 
						 const std::vector<texture*>& ColorTargets, const utils::render_context::input_data& InputData, const std::vector<shader_define>& ShaderDefines)
	: LoadOp(NewLoadOp), StoreOp(NewStoreOp)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();
	Info = InputData;

	D3D12_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	RasterDesc.CullMode = InputData.UseBackFace ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_FRONT;
	RasterDesc.FrontCounterClockwise = true;
	RasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	RasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterDesc.DepthClipEnable = true;
	//RasterDesc.MultisampleEnable = MsaaState;
	//RasterDesc.AntialiasedLineEnable = MsaaState;
	RasterDesc.ForcedSampleCount = 0;
	RasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

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
	D3D12_ROOT_PARAMETER PushConstantDesc = {};

	std::string GlobalName;
	std::vector<ComPtr<ID3DBlob>> ShadersBlob;
	for(const std::string Shader : ShaderList)
	{
		GlobalName = Shader.substr(Shader.find("."));
		if(Shader.find(".vert.") != std::string::npos)
		{
			PipelineDesc.VS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::vertex, HaveDrawID, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			PipelineDesc.DS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_control, HaveDrawID, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			PipelineDesc.HS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::tessellation_eval, HaveDrawID, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			PipelineDesc.GS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::geometry, HaveDrawID, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			PipelineDesc.PS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::fragment, HaveDrawID, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);
		}
	}

	std::vector<D3D12_ROOT_PARAMETER> Parameters;

	if(HaveDrawID)
	{
		PushConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		PushConstantDesc.Constants.Num32BitValues = 1;
		Parameters.push_back(PushConstantDesc);
	}

	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			for(u32 ParamIdx = 0; ParamIdx < ShaderRootLayout[LayoutIdx][BindingIdx].size(); ++ParamIdx)
			{
				Parameters.push_back(ShaderRootLayout[LayoutIdx][BindingIdx][ParamIdx]);
			}
		}
	}

	if(HavePushConstant)
	{
		PushConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		PushConstantDesc.Constants.Num32BitValues = PushConstantSize / sizeof(u32);
		Parameters.push_back(PushConstantDesc);
	}

	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != 0)
	{
		ResourceHeap = descriptor_heap(Gfx->Device.Get(), DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(ResourceHeap.Handle.Get(), (GlobalName + ".resource_heap").c_str());
		IsResourceHeapInited = true;
	}
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != 0)
	{
		SamplersHeap = descriptor_heap(Gfx->Device.Get(), DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
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
	for(u32 FormatIdx = 0; FormatIdx < ColorTargets.size(); ++FormatIdx) PipelineDesc.RTVFormats[FormatIdx] = GetDXFormat(ColorTargets[FormatIdx]->Info.Format);

	PipelineDesc.RasterizerState = RasterDesc;
	PipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PipelineDesc.DepthStencilState = DepthStencilDesc;
	PipelineDesc.SampleMask = UINT_MAX;
	PipelineDesc.PrimitiveTopologyType = InputData.UseOutline ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PipelineTopology = InputData.UseOutline ? D3D_PRIMITIVE_TOPOLOGY_LINELIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PipelineDesc.NumRenderTargets = ColorTargets.size();
	PipelineDesc.SampleDesc.Count = 1; //MsaaState ? MsaaQuality : 1;
	PipelineDesc.SampleDesc.Quality = 0; //MsaaState ? (MsaaQuality - 1) : 0;
	PipelineDesc.DSVFormat = InputData.UseDepth ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;
	PipelineDesc.SampleDesc.Count = 1;

	Res = Gfx->Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&Pipeline));

	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> IndirectArgs;
	D3D12_INDIRECT_ARGUMENT_DESC IndirectArg = {};
	IndirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	IndirectArg.Constant.RootParameterIndex = 0;
	IndirectArg.Constant.DestOffsetIn32BitValues = 0;
	IndirectArg.Constant.Num32BitValuesToSet = 1;
	if(HaveDrawID) IndirectArgs.push_back(IndirectArg);
	IndirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	IndirectArgs.push_back(IndirectArg);

	D3D12_COMMAND_SIGNATURE_DESC IndirectDesc = {};
	IndirectDesc.ByteStride = sizeof(indirect_draw_indexed_command);
	IndirectDesc.NumArgumentDescs = IndirectArgs.size();
	IndirectDesc.pArgumentDescs = IndirectArgs.data();
	Gfx->Device->CreateCommandSignature(&IndirectDesc, HaveDrawID ? RootSignatureHandle.Get() : nullptr, IID_PPV_ARGS(&IndirectSignatureHandle));
}

void directx12_render_context::
Begin(global_pipeline_context* GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight)  
{
	PipelineContext = static_cast<directx12_global_pipeline_context*>(GlobalPipelineContext);

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
				PipelineContext->CommandList->SetGraphicsRootDescriptorTable(HaveDrawID + BindingDesc.Idx, BindingDesc.TableBegin);
			} break;
			case dx12_descriptor_type::shader_resource:
			{
				PipelineContext->CommandList->SetGraphicsRootShaderResourceView(HaveDrawID + BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
			case dx12_descriptor_type::unordered_access:
			{
				PipelineContext->CommandList->SetGraphicsRootUnorderedAccessView(HaveDrawID + BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
			case dx12_descriptor_type::constant_buffer:
			{
				PipelineContext->CommandList->SetGraphicsRootConstantBufferView(HaveDrawID + BindingDesc.Idx, BindingDesc.ResourceBegin);
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
	BuffersToCommon.clear();
	TexturesToCommon.clear();
}

void directx12_render_context::
Clear()
{
	SetIndices.clear();
	ColorTargets.clear();
	BindingDescriptions.clear();
	ResourceHeap.Reset();
	SamplersHeap.Reset();
	ResourceBindingIdx = 0;
	SamplersBindingIdx = 0;
	RootResourceBindingIdx = 0;
	RootSamplersBindingIdx = 0;
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

void directx12_render_context::
SetStencilTarget(u32 RenderWidth, u32 RenderHeight, texture* StencilAttachment, vec2 Clear, u32 Face, bool EnableMultiview)  
{
}

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

	PipelineContext->CommandList->OMSetRenderTargets(ColorTargets.size(), ColorTargets.data(), Info.UseDepth, &DepthStencilTarget);
	PipelineContext->CommandList->IASetIndexBuffer(&IndexBufferView);
	PipelineContext->CommandList->ExecuteIndirect(IndirectSignatureHandle.Get(), ObjectDrawCount, Indirect->Handle.Get(), !HaveDrawID * 4, Indirect->CounterHandle.Get(), 0);

	SetIndices.clear();

	PipelineContext->TexturesToCommon.insert(TexturesToCommon.begin(), TexturesToCommon.end());
	PipelineContext->BuffersToCommon.insert(BuffersToCommon.begin(), BuffersToCommon.end());
	PipelineContext->BuffersToCommon.insert(IndirectCommands);
}

void directx12_render_context::
SetConstant(void* Data, size_t Size)  
{
	PipelineContext->CommandList->SetGraphicsRoot32BitConstants(HaveDrawID + RootResourceBindingIdx + RootSamplersBindingIdx, Size / sizeof(u32), Data, 0);
}

// NOTE: If with counter, then it is using 2 bindings instead of 1
void directx12_render_context::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);

	auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
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
	SetIndices[Set] += 1;

	if(UseCounter && Buffer->WithCounter)
	{
		DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
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
		SetIndices[Set] += 1;
	}
}

// TODO: Implement constant buffer views
void directx12_render_context::
SetUniformBufferView(buffer* Buffer, bool UseCounter, u32 Set)  
{
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);
	SetIndices[Set] += 1;
}

void directx12_render_context::
SetSampledImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx, u32 Set)  
{
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][1].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap.GetGpuHandle(SamplersBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap.GetCpuHandle(SamplersBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap.Type);
	}
	SamplersBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_render_context::
SetStorageImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx, u32 Set)  
{
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][0].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::unordered_access_table, ResourceHeap.GetGpuHandle(ResourceBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->UnorderedAccessViews[ViewIdx], ResourceHeap.Type);
	}
	ResourceBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_render_context::
SetImageSampler(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx, u32 Set)  
{
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][0].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::shader_resource_table, ResourceHeap.GetGpuHandle(ResourceBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->ShaderResourceViews[ViewIdx], ResourceHeap.Type);
	}
	ResourceBindingIdx += (BindingSize - Textures.size());

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap.GetGpuHandle(SamplersBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		auto DescriptorHandle = SamplersHeap.GetCpuHandle(SamplersBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap.Type);
	}
	SamplersBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
}

directx12_compute_context::
directx12_compute_context(renderer_backend* Backend, const std::string& Shader, const std::vector<shader_define>& ShaderDefines)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
	Device = Gfx->Device.Get();

	std::unordered_map<u32, u32> DescriptorHeapSizes;
	D3D12_ROOT_PARAMETER PushConstantDesc = {};

	bool HaveDrawID = false;
	D3D12_COMPUTE_PIPELINE_STATE_DESC PipelineDesc = {};
	PipelineDesc.CS = Gfx->LoadShaderModule(Shader.c_str(), shader_stage::compute, HaveDrawID, ShaderRootLayout, HavePushConstant, PushConstantSize, DescriptorHeapSizes, ShaderDefines);

	std::vector<D3D12_ROOT_PARAMETER> Parameters;
	for(u32 LayoutIdx = 0; LayoutIdx < ShaderRootLayout.size(); LayoutIdx++)
	{
		for(u32 BindingIdx = 0; BindingIdx < ShaderRootLayout[LayoutIdx].size(); ++BindingIdx)
		{
			for(u32 ParamIdx = 0; ParamIdx < ShaderRootLayout[LayoutIdx][BindingIdx].size(); ++ParamIdx)
			{
				Parameters.push_back(ShaderRootLayout[LayoutIdx][BindingIdx][ParamIdx]);
			}
		}
	}

	if(HavePushConstant)
	{
		PushConstantDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		PushConstantDesc.Constants.Num32BitValues = PushConstantSize / sizeof(u32);
		DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += PushConstantDesc.Constants.Num32BitValues;
		Parameters.push_back(PushConstantDesc);
	}

	std::string GlobalName = Shader.substr(Shader.find("."));
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != 0)
	{
		ResourceHeap = descriptor_heap(Gfx->Device.Get(), DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		NAME_DX12_OBJECT_CSTR(ResourceHeap.Handle.Get(), (GlobalName + ".resource_heap").c_str());
		IsResourceHeapInited = true;
	}
	if(DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != 0)
	{
		SamplersHeap = descriptor_heap(Gfx->Device.Get(), DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
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

	int fin = 5;
}

void directx12_compute_context::
Begin(global_pipeline_context* GlobalPipelineContext)  
{
	PipelineContext = static_cast<directx12_global_pipeline_context*>(GlobalPipelineContext);

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
			case dx12_descriptor_type::constant_buffer:
			{
				PipelineContext->CommandList->SetComputeRootConstantBufferView(BindingDesc.Idx, BindingDesc.ResourceBegin);
			} break;
		}
	}
}

void directx12_compute_context::
End()
{
	BuffersToCommon.clear();
	TexturesToCommon.clear();
}

void directx12_compute_context::
Clear()
{
	SetIndices.clear();
	BindingDescriptions.clear();
	ResourceHeap.Reset();
	SamplersHeap.Reset();
	ResourceBindingIdx = 0;
	SamplersBindingIdx = 0;
	RootResourceBindingIdx = 0;
	RootSamplersBindingIdx = 0;
}

void directx12_compute_context::
Execute(u32 X, u32 Y, u32 Z)
{
	PipelineContext->CommandList->Dispatch((X + 31) / 32, (Y + 31) / 32, (Z + 31) / 32);
	PipelineContext->TexturesToCommon.insert(TexturesToCommon.begin(), TexturesToCommon.end());
	PipelineContext->BuffersToCommon.insert(BuffersToCommon.begin(), BuffersToCommon.end());
	SetIndices.clear();
}

void directx12_compute_context::
SetConstant(void* Data, size_t Size)  
{
	PipelineContext->CommandList->SetComputeRoot32BitConstants(RootResourceBindingIdx + RootSamplersBindingIdx, Size / sizeof(u32), Data, 0);
}

void directx12_compute_context::
SetStorageBufferView(buffer* Buffer, bool UseCounter, u32 Set)
{
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);

	auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
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
	SetIndices[Set] += 1;

	if(UseCounter && Buffer->WithCounter)
	{
		DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
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
		SetIndices[Set] += 1;
	}
}

// TODO: Implement constant buffer views
void directx12_compute_context::
SetUniformBufferView(buffer* Buffer, bool UseCounter, u32 Set)  
{
	directx12_buffer* ToBind = static_cast<directx12_buffer*>(Buffer);
	BuffersToCommon.insert(Buffer);
	SetIndices[Set] += 1;
}

void directx12_compute_context::
SetSampledImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx, u32 Set)
{
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][1].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap.GetGpuHandle(SamplersBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap.GetCpuHandle(SamplersBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap.Type);
	}
	SamplersBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_compute_context::
SetStorageImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx, u32 Set)  
{
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][0].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::unordered_access_table, ResourceHeap.GetGpuHandle(ResourceBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->UnorderedAccessViews[ViewIdx], ResourceHeap.Type);
	}
	ResourceBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
}

void directx12_compute_context::
SetImageSampler(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx, u32 Set)  
{
	u32 BindingSize = ShaderRootLayout[Set][SetIndices[Set]][0].DescriptorTable.pDescriptorRanges[0].NumDescriptors;

	BindingDescriptions.push_back({dx12_descriptor_type::shader_resource_table, ResourceHeap.GetGpuHandle(ResourceBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootResourceBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);
		TexturesToCommon.insert(Textures[Idx]);

		auto DescriptorHandle = ResourceHeap.GetCpuHandle(ResourceBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->ShaderResourceViews[ViewIdx], ResourceHeap.Type);
	}
	ResourceBindingIdx += (BindingSize - Textures.size());

	BindingDescriptions.push_back({dx12_descriptor_type::sampler, SamplersHeap.GetGpuHandle(SamplersBindingIdx), {}, RootResourceBindingIdx + RootSamplersBindingIdx, (u32)Textures.size()});
	RootSamplersBindingIdx += 1;

	for(u32 Idx = 0; Idx < Textures.size(); ++Idx)
	{
		directx12_texture* ToBind = static_cast<directx12_texture*>(Textures[Idx]);

		auto DescriptorHandle = SamplersHeap.GetCpuHandle(SamplersBindingIdx++);
		Device->CopyDescriptorsSimple(1, DescriptorHandle, ToBind->Sampler, SamplersHeap.Type);
	}
	SamplersBindingIdx += (BindingSize - Textures.size());
	SetIndices[Set] += 1;
}
