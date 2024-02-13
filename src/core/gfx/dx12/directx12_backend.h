#ifndef RENDERER_DIRECTX_12_H_


// TODO: Move this to pipeline bindings
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
	directx12_backend(window* Window);
	~directx12_backend() override = default;
	void DestroyObject() override;

	[[nodiscard]] D3D12_SHADER_BYTECODE LoadShaderModule(const char* Path, shader_stage ShaderType, bool& HaveDrawID, u32* ArgumentsCount, std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>>& ShaderRootLayout, bool& HavePushConstant, u32& PushConstantSize, std::unordered_map<u32, u32>& DescriptorHeapSizes, const std::vector<shader_define>& ShaderDefines = {});

	void RecreateSwapchain(u32 NewWidth, u32 NewHeight) override;

	u32  MsaaQuality;
	b32  TearingSupport = false;
	bool MsaaState = false;
	DWORD MsgCallback = 0;

	const DXGI_FORMAT ColorTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

	ComPtr<IDXGIFactory6> Factory;
	descriptor_heap ColorTargetHeap;
	descriptor_heap DepthStencilHeap;
	descriptor_heap ResourcesHeap;
	descriptor_heap SamplersHeap;

	std::vector<ComPtr<ID3D12Resource>> SwapchainImages;
	std::vector<D3D12_RESOURCE_STATES> SwapchainCurrentState;

	directx12_command_queue* CommandQueue;
	directx12_command_queue* CmpCommandQueue;

	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<IDXGIAdapter1> Adapter;
	ComPtr<ID3D12Device6> Device;
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
};

#define RENDERER_DIRECTX_12_H_
#endif
