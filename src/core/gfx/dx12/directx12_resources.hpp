#pragma once

class directx12_memory_heap : public memory_heap
{
public:
	directx12_memory_heap() = default;
	directx12_memory_heap(renderer_backend* Backend) 
	{
		CreateResource(Backend);
	}

	~directx12_memory_heap() override = default;

	void CreateResource(renderer_backend* Backend) override;

	buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, u64 DataSize, u64 Count, bool NewWithCounter, u32 Usage) override;
	buffer* PushBuffer(renderer_backend* Backend, std::string DebugName,  void* Data, u64 DataSize, u64 Count, bool NewWithCounter, u32 Usage) override;

	texture* PushTexture(renderer_backend* Backend, std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) override;
	texture* PushTexture(renderer_backend* Backend, std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) override;

	D3D12MA::Allocator* Handle;
};

struct directx12_buffer : public buffer
{
	directx12_buffer(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, void* Data, u64 NewSize, u64 Count, bool NewWithCounter, u32 Usage)
	{
		Usage |= resource_flags::RF_CopyDst;
		CreateResource(Backend, Heap, DebugName, NewSize, Count, NewWithCounter, Usage);
		Update(Backend, Data);
	}

	directx12_buffer(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewSize, u64 Count, bool NewWithCounter, u32 Usage)
	{
		CreateResource(Backend, Heap, DebugName, NewSize, Count, NewWithCounter, Usage);
	}

	~directx12_buffer() override = default;

	void Update(renderer_backend* Backend, void* Data) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_fence Fence(Gfx->Device.Get());

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempHandle->Unmap(0, 0);

		ID3D12GraphicsCommandList* CommandList = Gfx->CommandQueue->AllocateCommandList();
		Gfx->CommandQueue->Reset();
		CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);

		CommandList->CopyResource(Handle.Get(), TempHandle.Get());

		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier);

		Gfx->CommandQueue->ExecuteAndRemove(CommandList);
		Fence.Flush(Gfx->CommandQueue);
	}

	void UpdateSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_fence Fence(Gfx->Device.Get());
		assert(UpdateByteSize <= Size);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		TempHandle->Unmap(0, 0);

		ID3D12GraphicsCommandList* CommandList = Gfx->CommandQueue->AllocateCommandList();
		Gfx->CommandQueue->Reset();
		CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);

		CommandList->CopyBufferRegion(Handle.Get(), 0, TempHandle.Get(), 0, UpdateByteSize);

		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier);

		Gfx->CommandQueue->ExecuteAndRemove(CommandList);
		Fence.Flush(Gfx->CommandQueue);
	}

	void Update(void* Data, global_pipeline_context* GlobalPipeline) override 
	{
		directx12_global_pipeline_context* PipelineContext = static_cast<directx12_global_pipeline_context*>(GlobalPipeline);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempHandle->Unmap(0, 0);

		PipelineContext->CommandList->CopyResource(Handle.Get(), TempHandle.Get());
	}

	void UpdateSize(void* Data, u32 UpdateByteSize, global_pipeline_context* GlobalPipeline) override 
	{
		directx12_global_pipeline_context* PipelineContext = static_cast<directx12_global_pipeline_context*>(GlobalPipeline);
		assert(UpdateByteSize <= Size);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, UpdateByteSize);
		TempHandle->Unmap(0, 0);

		PipelineContext->CommandList->CopyBufferRegion(Handle.Get(), 0, TempHandle.Get(), 0, UpdateByteSize);
	}

	void ReadBackSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_fence Fence(Gfx->Device.Get());

		ID3D12GraphicsCommandList* CommandList = Gfx->CommandQueue->AllocateCommandList();
		Gfx->CommandQueue->Reset();
		CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);

		CommandList->CopyBufferRegion(TempHandle.Get(), 0, Handle.Get(), 0, UpdateByteSize);

		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier);

		Gfx->CommandQueue->ExecuteAndRemove(CommandList);
		Fence.Flush(Gfx->CommandQueue);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, UpdateByteSize);
		TempHandle->Unmap(0, 0);
	}

	void ReadBackSize(void* Data, u32 UpdateByteSize, global_pipeline_context* GlobalPipeline) override 
	{
		directx12_global_pipeline_context* PipelineContext = static_cast<directx12_global_pipeline_context*>(GlobalPipeline);

		PipelineContext->CommandList->CopyBufferRegion(TempHandle.Get(), 0, Handle.Get(), 0, UpdateByteSize);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, UpdateByteSize);
		TempHandle->Unmap(0, 0);
	}

	void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewSize, u64 Count, bool NewWithCounter, u32 Usage) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_memory_heap* MemoryHeap = static_cast<directx12_memory_heap*>(Heap);
		WithCounter = NewWithCounter;

		Size = NewSize * Count + WithCounter * sizeof(u32);
		CounterOffset = NewSize * Count;

		D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
		if(Usage & image_flags::TF_Storage)
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Size, Flags);
		CD3DX12_RESOURCE_DESC CounterResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(u32), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		D3D12MA::ALLOCATION_DESC AllocDesc = {};
		AllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		MemoryHeap->Handle->CreateResource(&AllocDesc, &ResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, &Allocation, IID_PPV_ARGS(&Handle));
		MemoryHeap->Handle->CreateResource(&AllocDesc, &CounterResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, &Allocation, IID_PPV_ARGS(&CounterHandle));
		NAME_DX12_OBJECT_CSTR(Handle.Get(), DebugName.c_str());

		CD3DX12_HEAP_PROPERTIES ResourceTempType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC TempDesc = CD3DX12_RESOURCE_DESC::Buffer(Size, D3D12_RESOURCE_FLAG_NONE);
		Gfx->Device->CreateCommittedResource(&ResourceTempType, D3D12_HEAP_FLAG_NONE, &TempDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempHandle));

		{
			D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
			SrvDesc.Shader4ComponentMapping	        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SrvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
			SrvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
			SrvDesc.Buffer.Flags                    = D3D12_BUFFER_SRV_FLAG_NONE;
			SrvDesc.Buffer.FirstElement             = 0;
			SrvDesc.Buffer.NumElements              = Count;
			SrvDesc.Buffer.StructureByteStride      = NewSize;

			ShaderResourceView = Gfx->ResourcesHeap.GetNextCpuHandle();
			Gfx->Device->CreateShaderResourceView(Handle.Get(), &SrvDesc, ShaderResourceView);

			SrvDesc.Buffer.NumElements         = 1;
			SrvDesc.Buffer.StructureByteStride = sizeof(u32);
			CounterShaderResourceView = Gfx->ResourcesHeap.GetNextCpuHandle();
			Gfx->Device->CreateShaderResourceView(Handle.Get(), &SrvDesc, CounterShaderResourceView);
		}

		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
			UavDesc.Format                           = DXGI_FORMAT_UNKNOWN;
			UavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_BUFFER;
			UavDesc.Buffer.Flags                     = D3D12_BUFFER_UAV_FLAG_NONE;
			UavDesc.Buffer.FirstElement              = 0;
			UavDesc.Buffer.NumElements               = Count;
			UavDesc.Buffer.StructureByteStride       = NewSize;

			if(Usage & image_flags::TF_Storage)
			{
				UnorderedAccessView = Gfx->ResourcesHeap.GetNextCpuHandle();
				Gfx->Device->CreateUnorderedAccessView(Handle.Get(), nullptr, &UavDesc, UnorderedAccessView);
			}

			UavDesc.Buffer.NumElements         = 1;
			UavDesc.Buffer.StructureByteStride = sizeof(u32);
			CounterUnorderedAccessView = Gfx->ResourcesHeap.GetNextCpuHandle();
			Gfx->Device->CreateUnorderedAccessView(CounterHandle.Get(), nullptr, &UavDesc, CounterUnorderedAccessView);
		}
	}

	void DestroyResource() override {Handle.Reset();};

	ComPtr<ID3D12Resource> Handle;
	ComPtr<ID3D12Resource> TempHandle;
	ComPtr<ID3D12Resource> CounterHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE ShaderResourceView  = {};
	D3D12_CPU_DESCRIPTOR_HANDLE UnorderedAccessView = {};

	D3D12_CPU_DESCRIPTOR_HANDLE CounterShaderResourceView  = {};
	D3D12_CPU_DESCRIPTOR_HANDLE CounterUnorderedAccessView = {};

	ComPtr<D3D12MA::Allocation> Allocation;
	D3D12_RESOURCE_STATES CurrentState = {};
};

struct directx12_texture : public texture
{
	directx12_texture(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, const utils::texture::input_data& InputData = {image_format::R8G8B8A8_UINT, image_type::Texture2D, image_flags::TF_Storage, 1, 1, false, border_color::black_transparent, sampler_address_mode::clamp_to_edge, sampler_reduction_mode::weighted_average})
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		Device = Gfx->Device.Get();

		CreateResource(Backend, Heap, DebugName, NewWidth, NewHeight, DepthOrArraySize, InputData);
		if(Data || Info.UseStagingBuffer)
		{
			CreateStagingResource();
			Update(Backend, Data);
		}
		if(Data && !Info.UseStagingBuffer)
			DestroyStagingResource();

        D3D12_SAMPLER_DESC SamplerDesc = {};
        SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = GetDXAddressMode(InputData.AddressMode);
        SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
        SamplerDesc.Filter = D3D12_ENCODE_BASIC_FILTER(GetDXFilter(filter::linear), GetDXFilter(filter::linear), GetDXFilter(filter::linear), GetDXSamplerReductionMode(Info.ReductionMode));
        SamplerDesc.MaxLOD = InputData.MipLevels;

        switch(InputData.BorderColor)
		{
			case border_color::black_transparent:
				SamplerDesc.BorderColor[0] = 0.0f;
				SamplerDesc.BorderColor[1] = 0.0f;
				SamplerDesc.BorderColor[2] = 0.0f;
				SamplerDesc.BorderColor[3] = 0.0f;
				break;
			case border_color::black_opaque:
				SamplerDesc.BorderColor[0] = 0.0f;
				SamplerDesc.BorderColor[1] = 0.0f;
				SamplerDesc.BorderColor[2] = 0.0f;
				SamplerDesc.BorderColor[3] = 1.0f;
				break;
			case border_color::white_opaque:
				SamplerDesc.BorderColor[0] = 1.0f;
				SamplerDesc.BorderColor[1] = 1.0f;
				SamplerDesc.BorderColor[2] = 1.0f;
				SamplerDesc.BorderColor[3] = 1.0f;
				break;
		};

		Sampler = Gfx->SamplersHeap.GetNextCpuHandle();
        Gfx->Device->CreateSampler(&SamplerDesc, Sampler);
	}

	~directx12_texture() override = default;

	void Update(renderer_backend* Backend, void* Data) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_fence Fence(Gfx->Device.Get());

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, Size);
		TempHandle->Unmap(0, 0);

		ID3D12GraphicsCommandList* CommandList = Gfx->CommandQueue->AllocateCommandList();
		Gfx->CommandQueue->Reset();
		CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);

		D3D12_SUBRESOURCE_FOOTPRINT SubresourceDesc = {};
		SubresourceDesc.Format   = GetDXFormat(Info.Format);
		SubresourceDesc.Width    = Width;
		SubresourceDesc.Height   = Height;
		SubresourceDesc.Depth    = Depth;
		SubresourceDesc.RowPitch = AlignUp(Width * GetPixelSize(Info.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT TextureFootprint = {};
		TextureFootprint.Footprint = SubresourceDesc;

		auto DstCopyLocation = CD3DX12_TEXTURE_COPY_LOCATION(Handle.Get(), 0);
		auto SrcCopyLocation = CD3DX12_TEXTURE_COPY_LOCATION(TempHandle.Get(), TextureFootprint);
		CommandList->CopyTextureRegion(&DstCopyLocation, 
									   0, 0, 0, 
									   &SrcCopyLocation, 
									   nullptr);

		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier);

		Gfx->CommandQueue->ExecuteAndRemove(CommandList);
		Fence.Flush(Gfx->CommandQueue);
	}

	void Update(void* Data, global_pipeline_context* GlobalPipeline) override 
	{
		directx12_global_pipeline_context* PipelineContext = static_cast<directx12_global_pipeline_context*>(GlobalPipeline);

		D3D12_SUBRESOURCE_FOOTPRINT SubresourceDesc = {};
		SubresourceDesc.Format   = GetDXFormat(Info.Format);
		SubresourceDesc.Width    = Width;
		SubresourceDesc.Height   = Height;
		SubresourceDesc.Depth    = Depth;
		SubresourceDesc.RowPitch = AlignUp(Width * GetPixelSize(Info.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT TextureFootprint = {};
		TextureFootprint.Footprint = SubresourceDesc;

		auto DstCopyLocation = CD3DX12_TEXTURE_COPY_LOCATION(Handle.Get(), 0);
		auto SrcCopyLocation = CD3DX12_TEXTURE_COPY_LOCATION(TempHandle.Get(), TextureFootprint);
		PipelineContext->CommandList->CopyTextureRegion(&DstCopyLocation, 
									   0, 0, 0, 
									   &SrcCopyLocation, 
									   nullptr);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, Size);
		TempHandle->Unmap(0, 0);
	}

	void ReadBack(renderer_backend* Backend, void* Data) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_fence Fence(Gfx->Device.Get());

		ID3D12GraphicsCommandList* CommandList = Gfx->CommandQueue->AllocateCommandList();
		Gfx->CommandQueue->Reset();
		CommandList->Reset(Gfx->CommandQueue->CommandAlloc.Get(), nullptr);

		CommandList->CopyResource(TempHandle.Get(), Handle.Get());

		CD3DX12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		CommandList->ResourceBarrier(1, &Barrier);

		Gfx->CommandQueue->ExecuteAndRemove(CommandList);
		Fence.Flush(Gfx->CommandQueue);

		void* CpuPtr;
		TempHandle->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, Size);
		TempHandle->Unmap(0, 0);
	}

	void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const utils::texture::input_data& InputData) override 
	{
		directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);
		directx12_memory_heap* MemoryHeap = static_cast<directx12_memory_heap*>(Heap);

		Width  = NewWidth;
		Height = NewHeight;
		Depth  = DepthOrArraySize;
		Info   = InputData;

		D3D12_CLEAR_VALUE* Clear = nullptr;
        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Width = Width;
        ResourceDesc.Height = Height;
        ResourceDesc.DepthOrArraySize = Depth;
        ResourceDesc.MipLevels = Info.MipLevels;
        ResourceDesc.SampleDesc.Count = 1;
        ResourceDesc.SampleDesc.Quality = 0;
        ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		ResourceDesc.Format = GetDXFormat(Info.Format);

        if (Info.Type == image_type::Texture1D)
            ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        else if (Info.Type == image_type::Texture2D)
            ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        else if (Info.Type == image_type::Texture3D)
            ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

        if ((Info.Usage & image_flags::TF_DepthTexture) || (Info.Usage & image_flags::TF_StencilTexture))
		{
			Clear = new D3D12_CLEAR_VALUE;
			Clear->Format = GetDXFormat(Info.Format);
			Clear->DepthStencil.Depth   = 1.0f;
			Clear->DepthStencil.Stencil = 0;
            ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

        if ((Info.Usage & image_flags::TF_ColorAttachment))
		{
			vec4 ClearColor = vec4(vec3(0), 1);
			Clear = new D3D12_CLEAR_VALUE;
			Clear->Format = GetDXFormat(Info.Format);
			memcpy((void*)Clear->Color, (void*)ClearColor.E, sizeof(float)*4);
            ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if(Info.Usage & image_flags::TF_Storage)
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if (!(Info.Usage & image_flags::TF_Sampled) && !(Info.Usage & image_flags::TF_Storage))
            ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		D3D12MA::ALLOCATION_DESC AllocDesc = {};
		AllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		MemoryHeap->Handle->CreateResource(&AllocDesc, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, Clear, &Allocation, IID_PPV_ARGS(&Handle));
		Size = Allocation->GetSize();
		NAME_DX12_OBJECT_CSTR(Handle.Get(), DebugName.c_str());

        if (Info.Usage & image_flags::TF_Sampled)
        {
			// SRV

            D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
            SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            SrvDesc.Format = GetDXDepthTargetShaderResourceViewFormat(Info.Format); //ResourceDesc.Format;

			for(u32 MipIdx = 0;
				MipIdx < Info.MipLevels;
				++MipIdx)
			{
				if (Info.Usage & image_flags::TF_CubeMap)
				{
					SrvDesc.ViewDimension               = D3D12_SRV_DIMENSION_TEXTURECUBE;
					SrvDesc.TextureCube.MipLevels       = 1;
					SrvDesc.TextureCube.MostDetailedMip = MipIdx;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers == 1)
				{
					SrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE1D;
					SrvDesc.Texture1D.MipLevels       = 1;
					SrvDesc.Texture1D.MostDetailedMip = MipIdx;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers > 1)
				{
					SrvDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					SrvDesc.Texture1DArray.MipLevels       = 1;
					SrvDesc.Texture1DArray.MostDetailedMip = MipIdx;
					SrvDesc.Texture1DArray.ArraySize       = Info.Layers;
					SrvDesc.Texture1DArray.FirstArraySlice = 0;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers == 1)
				{
					SrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
					SrvDesc.Texture2D.MipLevels       = 1;
					SrvDesc.Texture2D.MostDetailedMip = MipIdx;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers > 1)
				{
					SrvDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					SrvDesc.Texture2DArray.MipLevels       = 1;
					SrvDesc.Texture2DArray.MostDetailedMip = MipIdx;
					SrvDesc.Texture2DArray.ArraySize       = Info.Layers;
					SrvDesc.Texture2DArray.FirstArraySlice = 0;
				}
				else if (Info.Type == image_type::Texture3D)
				{
					SrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
					SrvDesc.Texture3D.MipLevels       = Info.MipLevels;
					SrvDesc.Texture3D.MostDetailedMip = MipIdx;
				}

				auto ShaderResourceView = Gfx->ResourcesHeap.GetNextCpuHandle();
				Gfx->Device->CreateShaderResourceView(Handle.Get(), &SrvDesc, ShaderResourceView);
				ShaderResourceViews.push_back(ShaderResourceView);
			}
        }

        if (Info.Usage & image_flags::TF_Storage)
        {
			// UAV

            D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
            UavDesc.Format = GetDXDepthTargetShaderResourceViewFormat(Info.Format); //ResourceDesc.Format;

			for(u32 MipIdx = 0;
				MipIdx < Info.MipLevels;
				++MipIdx)
			{
				if (Info.Type == image_type::Texture1D && Info.Layers == 1)
				{
					UavDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
					UavDesc.Texture1D.MipSlice = MipIdx;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers > 1)
				{
					UavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
					UavDesc.Texture1DArray.MipSlice	       = MipIdx;
					UavDesc.Texture1DArray.ArraySize       = Info.Layers;
					UavDesc.Texture1DArray.FirstArraySlice = 0;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers == 1)
				{
					UavDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
					UavDesc.Texture2D.MipSlice = MipIdx;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers > 1)
				{
					UavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					UavDesc.Texture2DArray.MipSlice        = MipIdx;
					UavDesc.Texture2DArray.ArraySize       = Info.Layers;
					UavDesc.Texture2DArray.FirstArraySlice = 0;
				}
				else if (Info.Type == image_type::Texture3D)
				{
					UavDesc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
					UavDesc.Texture3D.MipSlice    = MipIdx;
					UavDesc.Texture3D.FirstWSlice = 0;
					UavDesc.Texture3D.WSize       = Info.Layers;
				}

				auto UnorderedAccessView = Gfx->ResourcesHeap.GetNextCpuHandle();
				Gfx->Device->CreateUnorderedAccessView(Handle.Get(), nullptr, &UavDesc, UnorderedAccessView);
				UnorderedAccessViews.push_back(UnorderedAccessView);
			}
        }

        if (Info.Usage & image_flags::TF_ColorAttachment)
        {
            // RTV

            D3D12_RENDER_TARGET_VIEW_DESC RtvDesc = {};
            RtvDesc.Format = GetDXDepthTargetShaderResourceViewFormat(Info.Format); //ResourceDesc.Format;

			for(u32 MipIdx = 0;
				MipIdx < Info.MipLevels;
				++MipIdx)
			{
				auto RenderTargetView = Gfx->ColorTargetHeap.GetNextCpuHandle();

				if (Info.Usage & image_flags::TF_CubeMap)
				{
					for(u32 FaceIdx = 0; FaceIdx < 6; ++FaceIdx)
					{
						RtvDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						RtvDesc.Texture2DArray.ArraySize       = 1;
						RtvDesc.Texture2DArray.FirstArraySlice = FaceIdx;
						RtvDesc.Texture2DArray.MipSlice        = MipIdx;

						Gfx->Device->CreateRenderTargetView(Handle.Get(), &RtvDesc, RenderTargetView);
						RenderTargetViews.push_back(RenderTargetView);
						RenderTargetView = Gfx->ColorTargetHeap.GetNextCpuHandle();
					}
					continue;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers == 1)
				{
					RtvDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
					RtvDesc.Texture1D.MipSlice = MipIdx;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers > 1)
				{
					RtvDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
					RtvDesc.Texture1DArray.ArraySize       = Info.Layers;
					RtvDesc.Texture1DArray.FirstArraySlice = 0;
					RtvDesc.Texture1DArray.MipSlice        = MipIdx;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers == 1)
				{
					RtvDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE2D;
					RtvDesc.Texture2D.MipSlice = MipIdx;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers > 1)
				{
					RtvDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					RtvDesc.Texture2DArray.ArraySize       = Info.Layers;
					RtvDesc.Texture2DArray.FirstArraySlice = 0;
					RtvDesc.Texture2DArray.MipSlice        = MipIdx;
				}
				else if (Info.Type == image_type::Texture3D)
				{
					RtvDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE3D;
					RtvDesc.Texture3D.MipSlice = MipIdx;
				}

				Gfx->Device->CreateRenderTargetView(Handle.Get(), &RtvDesc, RenderTargetView);
				RenderTargetViews.push_back(RenderTargetView);
			}
        }

        if ((Info.Usage & image_flags::TF_DepthTexture) || (Info.Usage & image_flags::TF_StencilTexture))
        {
            // DSV

            D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilDesc = {};
            DepthStencilDesc.Format = ResourceDesc.Format;

			for(u32 MipIdx = 0;
				MipIdx < Info.MipLevels;
				++MipIdx)
			{
				auto DepthStencilView = Gfx->DepthStencilHeap.GetNextCpuHandle();

				if (Info.Usage & image_flags::TF_CubeMap)
				{
					for(u32 FaceIdx = 0; FaceIdx < 6; ++FaceIdx)
					{
						DepthStencilDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						DepthStencilDesc.Texture2DArray.ArraySize       = Info.Layers;
						DepthStencilDesc.Texture2DArray.FirstArraySlice = FaceIdx;
						DepthStencilDesc.Texture2DArray.MipSlice        = 1;

						Gfx->Device->CreateDepthStencilView(Handle.Get(), &DepthStencilDesc, DepthStencilView);
						DepthStencilViews.push_back(DepthStencilView);
						DepthStencilView = Gfx->DepthStencilHeap.GetNextCpuHandle();
					}
					continue;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers == 1)
				{
					DepthStencilDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
					DepthStencilDesc.Texture1D.MipSlice = MipIdx;
				}
				else if (Info.Type == image_type::Texture1D && Info.Layers > 1)
				{
					DepthStencilDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
					DepthStencilDesc.Texture1DArray.ArraySize       = Info.Layers;
					DepthStencilDesc.Texture1DArray.FirstArraySlice = 0;
					DepthStencilDesc.Texture1DArray.MipSlice        = MipIdx;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers == 1)
				{
					DepthStencilDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
					DepthStencilDesc.Texture2D.MipSlice = MipIdx;
				}
				else if (Info.Type == image_type::Texture2D && Info.Layers > 1)
				{
					DepthStencilDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
					DepthStencilDesc.Texture2DArray.ArraySize       = Info.Layers;
					DepthStencilDesc.Texture2DArray.FirstArraySlice = 0;
					DepthStencilDesc.Texture2DArray.MipSlice        = MipIdx;
				}

				Gfx->Device->CreateDepthStencilView(Handle.Get(), &DepthStencilDesc, DepthStencilView);
				DepthStencilViews.push_back(DepthStencilView);
			}
        }

		if(Clear)
			delete Clear;
	}

	void CreateStagingResource() override 
	{
		CD3DX12_HEAP_PROPERTIES ResourceTempType = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC TempDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		Device->CreateCommittedResource(&ResourceTempType, D3D12_HEAP_FLAG_NONE, &TempDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempHandle));
	}

	void DestroyResource() override {Handle.Reset();}
	void DestroyStagingResource() override {TempHandle.Reset();}


	ID3D12Device6* Device;
	ComPtr<ID3D12Resource> Handle;
	ComPtr<ID3D12Resource> TempHandle;
	ComPtr<D3D12MA::Allocation> Allocation;
	D3D12_RESOURCE_STATES CurrentState = {};

	D3D12_CPU_DESCRIPTOR_HANDLE Sampler = {};

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ShaderResourceViews;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> UnorderedAccessViews;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RenderTargetViews;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DepthStencilViews;
};

void directx12_memory_heap::
CreateResource(renderer_backend* Backend)
{
	directx12_backend* Gfx = static_cast<directx12_backend*>(Backend);

	D3D12MA::ALLOCATOR_DESC AllocatorDesc = {};
	AllocatorDesc.pDevice  = Gfx->Device.Get();
	AllocatorDesc.pAdapter = Gfx->Adapter.Get();
	HRESULT hr = D3D12MA::CreateAllocator(&AllocatorDesc, &Handle);
}

buffer* directx12_memory_heap::
PushBuffer(renderer_backend* Backend, std::string DebugName, u64 DataSize, u64 Count, bool NewWithCounter, u32 Usage)
{
	return new directx12_buffer(Backend, this, DebugName, DataSize, Count, NewWithCounter, Usage);
}

buffer* directx12_memory_heap::
PushBuffer(renderer_backend* Backend, std::string DebugName, void* Data, u64 DataSize, u64 Count, bool NewWithCounter, u32 Usage)
{
	return new directx12_buffer(Backend, this, DebugName, Data, DataSize, Count, NewWithCounter, Usage);
}

texture* directx12_memory_heap::
PushTexture(renderer_backend* Backend, std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
{
	return new directx12_texture(Backend, this, DebugName, nullptr, Width, Height, Depth, InputData);
}

texture* directx12_memory_heap::
PushTexture(renderer_backend* Backend, std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
{
	return new directx12_texture(Backend, this, DebugName, Data, Width, Height, Depth, InputData);
}
