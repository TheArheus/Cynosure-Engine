#ifndef RENDERER_DIRECTX_12_H_


class descriptor_heap
{
	u32 Size = 0;
	u32 Next = 0;

public:
	descriptor_heap() = default;

	descriptor_heap(ID3D12Device1* Device, u32 NumberOfDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE NewType, D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
	{
		Init(Device, NumberOfDescriptors, NewType, Flags);
	}

	~descriptor_heap()
	{
		if(Handle)
		{
			Handle->Release();
		}
	}

    descriptor_heap(descriptor_heap&& Other) noexcept
        : Size(Other.Size),
          Next(Other.Next),
          AllocInc(Other.AllocInc),
          GpuHandle(Other.GpuHandle),
          GpuBegin(Other.GpuBegin),
          CpuHandle(Other.CpuHandle),
          CpuBegin(Other.CpuBegin),
          Handle(std::move(Other.Handle)),
          Type(Other.Type)
    {
        Other.Reset();
    }

    descriptor_heap& operator=(descriptor_heap&& Other) noexcept
    {
        if (this != &Other)
        {
            if (Handle)
            {
                Handle->Release();
            }

            Size = Other.Size;
            Next = Other.Next;
            AllocInc = Other.AllocInc;
            GpuHandle = Other.GpuHandle;
            GpuBegin = Other.GpuBegin;
            CpuHandle = Other.CpuHandle;
            CpuBegin = Other.CpuBegin;
            Handle = std::move(Other.Handle);
            Type = Other.Type;

            Other.Reset();
        }
        return *this;
    }

    descriptor_heap(const descriptor_heap&) = delete;
    descriptor_heap& operator=(const descriptor_heap&) = delete;

	void Init(ID3D12Device1* Device, u32 NumberOfDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE NewType, D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors = Size = NumberOfDescriptors;
		HeapDesc.Type  = Type = NewType;
		HeapDesc.Flags = Flags;
		HRESULT hr = Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Handle));

		CpuBegin = CpuHandle = Handle->GetCPUDescriptorHandleForHeapStart();
		if(Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
			GpuHandle = GpuBegin = Handle->GetGPUDescriptorHandleForHeapStart();

		AllocInc = Device->GetDescriptorHandleIncrementSize(Type);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(u32 Idx)
	{
		assert(Idx < Size);
		D3D12_GPU_DESCRIPTOR_HANDLE Result = {GpuBegin.ptr + Idx * AllocInc};
		return Result;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetNextGpuHandle()
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Result = {GpuBegin.ptr + Next * AllocInc};
		assert(Next < Size);
		Next++;
		return Result;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(u32 Idx)
	{
		assert(Idx < Size);
		D3D12_CPU_DESCRIPTOR_HANDLE Result = {CpuBegin.ptr + Idx * AllocInc};
		return Result;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetNextCpuHandle()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE Result = {CpuBegin.ptr + Next * AllocInc};
		assert(Next < Size);
		Next++;
		return Result;
	}

	void Reset()
	{
		CpuBegin = CpuHandle;
		GpuBegin = GpuHandle;
		Next = 0;
	}

	u32 AllocInc;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuBegin;

	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE CpuBegin;

	ComPtr<ID3D12DescriptorHeap> Handle;
	D3D12_DESCRIPTOR_HEAP_TYPE Type;
};

struct directx12_backend : public renderer_backend
{
	struct compiled_shader_info
	{
		std::map<u32, std::map<u32, u32>> NewBindings;
		std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>> ShaderRootLayout;
		std::unordered_map<u32, u32> DescriptorHeapSizes;
		D3D12_SHADER_BYTECODE Handle;
		u32  PushConstantSize;
		u32 LocalSizeX;
		u32 LocalSizeY;
		u32 LocalSizeZ;
		bool HavePushConstant;
		bool HaveDrawID;
	};

	directx12_backend(window* Window);
	~directx12_backend() override { DestroyObject(); };
	void DestroyObject() override;

	[[nodiscard]] D3D12_SHADER_BYTECODE LoadShaderModule(const char* Path, shader_stage ShaderType, bool& HaveDrawID, std::map<u32, std::map<u32, descriptor_param>>& ParameterLayout, std::map<u32, std::map<u32, u32>>& NewBindings, std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>>& ShaderRootLayout, bool& HavePushConstant, u32& PushConstantSize, std::unordered_map<u32, u32>& DescriptorHeapSizes, const std::vector<shader_define>& ShaderDefines = {}, u32* LocalSizeX = nullptr, u32* LocalSizeY = nullptr, u32* LocalSizeZ = nullptr);

	void RecreateSwapchain(u32 NewWidth, u32 NewHeight) override;

	u32  MsaaQuality;
	b32  TearingSupport = false;
	bool MsaaState = false;
	bool MinMaxFilterAvailable = false;
	DWORD MsgCallback = 0;

	const DXGI_FORMAT ColorTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

	D3D12MA::Allocator* AllocatorHandle = nullptr;

	ComPtr<IDXGIFactory6> Factory;
	descriptor_heap* ColorTargetHeap;
	descriptor_heap* DepthStencilHeap;
	descriptor_heap* ResourcesHeap;
	descriptor_heap* SamplersHeap;

	descriptor_heap* ImGuiResourcesHeap;

	std::vector<ComPtr<ID3D12Resource>> SwapchainImages;
	std::vector<D3D12_RESOURCE_STATES> SwapchainCurrentState;

	std::unordered_map<std::string, compiled_shader_info> CompiledShaders;

	directx12_fence* Fence;
	directx12_command_queue* CommandQueue;

	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<IDXGIAdapter1> Adapter;
	ComPtr<ID3D12Device6> Device;
#if defined(CE_DEBUG)
	ComPtr<ID3D12InfoQueue1> InfoQueue;
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
#endif
};

#define RENDERER_DIRECTX_12_H_
#endif
