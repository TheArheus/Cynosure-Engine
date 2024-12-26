
#include <D3D12MemAlloc.cpp>

directx12_backend::
directx12_backend(HWND Handle, ImGuiContext* _imguiContext)
{
	Type = backend_type::directx12;

	imguiContext = _imguiContext;
	ImGui::SetCurrentContext(imguiContext);

	RECT WindowRect;
	GetClientRect(Handle, &WindowRect);
	u32 ClientWidth  = WindowRect.right - WindowRect.left;
	u32 ClientHeight = WindowRect.bottom - WindowRect.top;

	Width  = ClientWidth;
	Height = ClientHeight;

	UINT FactoryFlags = 0;
#if defined(CE_DEBUG)
	ComPtr<ID3D12Debug>  Debug;
	ComPtr<ID3D12Debug1> Debug1;
	{
		if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug))))
		{
			Debug->EnableDebugLayer();
			if(SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug1))))
			{
				Debug1->EnableDebugLayer();
				Debug1->SetEnableGPUBasedValidation(true);
				Debug1->Release();
			}
		}
	}

	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings))))
	{
		pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}

	FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	
	CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory));
	{
		GetDevice(Factory.Get(), &Adapter);
		D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	}

#if defined(CE_DEBUG)
	{
		if (SUCCEEDED(Device->QueryInterface<ID3D12InfoQueue1>(&InfoQueue)))
		{
			InfoQueue->RegisterMessageCallback(&MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &MsgCallback);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		}
	}
#endif

	Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingSupport, sizeof(TearingSupport));

	D3D12_FEATURE_DATA_FORMAT_SUPPORT DataFormatSupport = {ColorTargetFormat, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
	D3D12_FEATURE_DATA_D3D12_OPTIONS RendererOptions = {};
	Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &DataFormatSupport, sizeof(DataFormatSupport));
	Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &RendererOptions, sizeof(RendererOptions));
	if(RendererOptions.TiledResourcesTier >= 2) MinMaxFilterAvailable = true;

	Fence = new directx12_fence(Device.Get());
	CommandQueue = new directx12_command_queue(Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	{
		ComPtr<IDXGISwapChain1> _SwapChain;
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Flags = (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		SwapChainDesc.Width = ClientWidth;
		SwapChainDesc.Height = ClientHeight;
		SwapChainDesc.BufferCount = ImageCount;
		SwapChainDesc.Format = ColorTargetFormat;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
		Factory->CreateSwapChainForHwnd(CommandQueue->Handle.Get(), Handle, &SwapChainDesc, nullptr, nullptr, &_SwapChain);
		_SwapChain.As(&SwapChain);

		SwapchainImages.resize(ImageCount);
		SwapchainCurrentState.resize(ImageCount);
	}

	ColorTargetHeap  = new descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DepthStencilHeap = new descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	ResourcesHeap    = new descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	SamplersHeap     = new descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	UpdateHeap       = new descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	ImGuiResourcesHeap = new descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	NAME_DX12_OBJECT_CSTR(ColorTargetHeap->Handle.Get(), "GlobalColorTargetHeap");
	NAME_DX12_OBJECT_CSTR(DepthStencilHeap->Handle.Get(), "GlobalDepthStencilHeap");
	NAME_DX12_OBJECT_CSTR(ResourcesHeap->Handle.Get(), "GlobalResourcesHeap");
	NAME_DX12_OBJECT_CSTR(SamplersHeap->Handle.Get(), "GlobalSamplersHeap");
	NAME_DX12_OBJECT_CSTR(UpdateHeap->Handle.Get(), "GlobalUpdateHeap");

	{
		D3D12_RENDER_TARGET_VIEW_DESC ColorTargetViewDesc = {};
		ColorTargetViewDesc.Format = ColorTargetFormat;
		ColorTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

		for(u32 i = 0; i < ImageCount; i++)
		{
			SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapchainImages[i]));
			Device->CreateRenderTargetView(SwapchainImages[i].Get(), &ColorTargetViewDesc, ColorTargetHeap->GetNextCpuHandle());
		}
	}

	u32 RenderFormatRequired = D3D12_FORMAT_SUPPORT1_RENDER_TARGET | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
	bool RenderMultisampleSupport = (DataFormatSupport.Support1 & RenderFormatRequired) == RenderFormatRequired;
	u32 DepthFormatRequired = D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
	bool DepthMultisampleSupport = (DataFormatSupport.Support1 & DepthFormatRequired) == DepthFormatRequired;

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MsQualityLevels = {};
	MsQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	MsQualityLevels.Format = ColorTargetFormat;
	for (MsaaQuality = 4;
		 MsaaQuality > 1;
		 MsaaQuality--)
	{
		MsQualityLevels.SampleCount = MsaaQuality;
		if (FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &MsQualityLevels, sizeof(MsQualityLevels)))) continue;
		if (MsQualityLevels.NumQualityLevels > 0) break;
	}
	
	MsaaQuality = MsQualityLevels.NumQualityLevels;
	if (MsaaQuality >= 2) MsaaState = true;
	if (MsaaQuality < 2) RenderMultisampleSupport = false;

	ImGui_ImplDX12_Init(Device.Get(), ImageCount, ColorTargetFormat, ImGuiResourcesHeap->Handle.Get(), ImGuiResourcesHeap->CpuHandle, ImGuiResourcesHeap->GpuHandle);

	D3D12MA::ALLOCATOR_DESC AllocatorDesc = {};
	AllocatorDesc.pDevice  = Device.Get();
	AllocatorDesc.pAdapter = Adapter.Get();
	D3D12MA::CreateAllocator(&AllocatorDesc, &AllocatorHandle);
};

void directx12_backend::
DestroyObject() 
{
	ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplDX12_Shutdown();

	imguiContext = nullptr;

	delete ColorTargetHeap;
	ColorTargetHeap = nullptr;
	delete DepthStencilHeap;
	DepthStencilHeap = nullptr;
	delete ResourcesHeap;
	ResourcesHeap = nullptr;
	delete SamplersHeap;
	SamplersHeap = nullptr;
	delete UpdateHeap;
	UpdateHeap = nullptr;

	delete ImGuiResourcesHeap;
	ImGuiResourcesHeap = nullptr;

	AllocatorHandle->Release(); 
	AllocatorHandle = nullptr;

    delete CommandQueue;
    CommandQueue = nullptr;

	delete Fence;
	Fence = nullptr;

	SwapchainImages.clear();

    SwapChain.Reset();
    Adapter.Reset();
    Factory.Reset();

#if defined(CE_DEBUG)
	pDredSettings.Reset();
    if (Device && InfoQueue)
	{
		InfoQueue->UnregisterMessageCallback(MsgCallback);
		InfoQueue.Reset();
    }
#endif

    Device.Reset();
}

void directx12_backend::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
	Width  = NewWidth;
	Height = NewHeight;

	SwapchainImages.clear();

	SwapChain->ResizeBuffers(ImageCount, NewWidth, NewHeight, ColorTargetFormat, (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	D3D12_RENDER_TARGET_VIEW_DESC ColorTargetViewDesc = {};
	ColorTargetViewDesc.Format = ColorTargetFormat;
	ColorTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

	for(u32 i = 0; i < ImageCount; i++)
	{
		SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapchainImages[i]));
		Device->CreateRenderTargetView(SwapchainImages[i].Get(), &ColorTargetViewDesc, ColorTargetHeap->GetNextCpuHandle());
	}
}

dx12_descriptor_type GetDXSpvDescriptorType(const std::vector<op_info>& ShaderInfo, const op_info& Var, image_type& ImageType, u32 StorageClass, bool NonWritable)
{
	switch(Var.OpCode)
	{
		case SpvOpTypeStruct:
		{
			if(StorageClass == SpvStorageClassUniform || StorageClass == SpvStorageClassStorageBuffer)
				return NonWritable ? dx12_descriptor_type::shader_resource : dx12_descriptor_type::unordered_access;
			else if(StorageClass == SpvStorageClassUniformConstant)
				return dx12_descriptor_type::constant_buffer;
		} break;
		case SpvOpTypeImage:
		{
			switch(Var.Dimensionality)
			{
				case spv::Dim1D: ImageType = image_type::Texture1D; break;
				case spv::Dim2D: ImageType = image_type::Texture2D; break;
				case spv::Dim3D: ImageType = image_type::Texture3D; break;
				case spv::DimCube: ImageType = image_type::TextureCube; break;
			}
			return dx12_descriptor_type::image;
		} break;
		case SpvOpTypeSampler:
		{
			return dx12_descriptor_type::sampler;
		} break;
		case SpvOpTypeSampledImage:
		{
			const op_info& TextureInfo = ShaderInfo[Var.TypeId[0]];
			switch(TextureInfo.Dimensionality)
			{
				case spv::Dim1D: ImageType = image_type::Texture1D; break;
				case spv::Dim2D: ImageType = image_type::Texture2D; break;
				case spv::Dim3D: ImageType = image_type::Texture3D; break;
				case spv::DimCube: ImageType = image_type::TextureCube; break;
			}
			return dx12_descriptor_type::combined_image_sampler;
		} break;
	}
}

// TODO: Implement a better shader compilation
[[nodiscard]] D3D12_SHADER_BYTECODE directx12_backend::
LoadShaderModule(const char* Path, shader_stage ShaderType, bool& HaveDrawID, std::map<u32, std::map<u32, descriptor_param>>& ParameterLayout, std::map<u32, std::map<u32, u32>>& NewBindings, std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>>& ShaderRootLayout, bool& HavePushConstant, u32& PushConstantSize, std::unordered_map<u32, u32>& DescriptorHeapSizes, const std::vector<shader_define>& ShaderDefines, u32* LocalSizeX, u32* LocalSizeY, u32* LocalSizeZ)
{
	auto FoundCompiledShader = CompiledShaders.find(Path);
	if(FoundCompiledShader != CompiledShaders.end())
	{
		for (const auto& [Set, Bindings] : FoundCompiledShader->second.ParameterLayout)
		{
			for (const auto& [Binding, Param] : Bindings)
			{
				ParameterLayout[Set][Binding] = Param;
			}
		}
		NewBindings.insert(FoundCompiledShader->second.NewBindings.begin(), FoundCompiledShader->second.NewBindings.end());
		ShaderRootLayout.insert(FoundCompiledShader->second.ShaderRootLayout.begin(), FoundCompiledShader->second.ShaderRootLayout.end());
		DescriptorHeapSizes.insert(FoundCompiledShader->second.DescriptorHeapSizes.begin(), FoundCompiledShader->second.DescriptorHeapSizes.end());
		HavePushConstant = FoundCompiledShader->second.HavePushConstant;
		PushConstantSize = FoundCompiledShader->second.PushConstantSize;
		HaveDrawID       = FoundCompiledShader->second.HaveDrawID;
		LocalSizeX ? *LocalSizeX = FoundCompiledShader->second.LocalSizeX : 0;
		LocalSizeY ? *LocalSizeY = FoundCompiledShader->second.LocalSizeY : 0;
		LocalSizeZ ? *LocalSizeZ = FoundCompiledShader->second.LocalSizeZ : 0;
		return FoundCompiledShader->second.Handle;
	}

	D3D12_SHADER_BYTECODE Result = {};
	std::ifstream File(Path);
	if(File)
	{
		ComPtr<IDxcCompiler3> DxcCompiler3;
		ComPtr<IDxcCompiler> DxcCompiler;
		ComPtr<IDxcLibrary> DxcLib;
		ComPtr<IDxcUtils> DxcUtils;

		HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcCompiler3));
		bool UseDxcCompiler3 = SUCCEEDED(hr);
		if(UseDxcCompiler3)
		{
			hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcUtils));
			if(FAILED(hr))
			{
				std::cerr << "Failed to create IDxcUtils instance." << std::endl;
				return {};
			}
		}
		else
		{
			hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&DxcLib));
			if(FAILED(hr)) { /* Handle error */ }

			hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcCompiler));
			if(FAILED(hr)) { /* Handle error */ }
		}

		std::vector<std::wstring> LongArgs;
		std::vector<LPCWSTR> ShaderCompileArgs;
		ShaderCompileArgs.push_back(L"-spirv");
		ShaderCompileArgs.push_back(L"-E");
		ShaderCompileArgs.push_back(L"main");

		ShaderCompileArgs.push_back(L"-T");
		if (ShaderType == shader_stage::vertex)
			ShaderCompileArgs.push_back(L"vs_6_2");
		else if (ShaderType == shader_stage::fragment)
			ShaderCompileArgs.push_back(L"ps_6_2");
		else if (ShaderType == shader_stage::compute)
			ShaderCompileArgs.push_back(L"cs_6_2");
		else if (ShaderType == shader_stage::geometry)
			ShaderCompileArgs.push_back(L"gs_6_2");
		else if (ShaderType == shader_stage::tessellation_control)
			ShaderCompileArgs.push_back(L"hs_6_2");
		else if (ShaderType == shader_stage::tessellation_eval)
			ShaderCompileArgs.push_back(L"ds_6_2");

		const wchar_t* TargetProfile = L"vs_6_2";
		if (ShaderType == shader_stage::fragment)
			TargetProfile = L"ps_6_2";
		else if (ShaderType == shader_stage::compute)
			TargetProfile = L"cs_6_2";
		else if (ShaderType == shader_stage::geometry)
			TargetProfile = L"gs_6_2";
		else if (ShaderType == shader_stage::tessellation_control)
			TargetProfile = L"hs_6_2";
		else if (ShaderType == shader_stage::tessellation_eval)
			TargetProfile = L"ds_6_2";


		std::string ShaderDefinesResult;
		for(const shader_define& Define : ShaderDefines)
		{
			std::string  ArgParam = Define.Name + "=" + Define.Value;
			std::wstring ResParam(ArgParam.begin(), ArgParam.end());
			LongArgs.push_back(ResParam);

			ShaderCompileArgs.push_back(L"-D");
			ShaderCompileArgs.push_back(LongArgs.back().c_str());

			ShaderDefinesResult += std::string("#define " + Define.Name + " " + Define.Value + "\n");
		}

		std::string ShaderCode((std::istreambuf_iterator<char>(File)), (std::istreambuf_iterator<char>()));
		std::vector<u32> SpirvCode;

		size_t VerPos = ShaderCode.find("#version");
		if (VerPos != std::string::npos)
		{
			size_t LineEnd = ShaderCode.find_first_of("\r\n", VerPos);
			ShaderCode = ShaderCode.substr(0, LineEnd) + "\n" + ShaderDefinesResult + ShaderCode.substr(LineEnd);
		}

		if (ShaderType == shader_stage::geometry || 
			ShaderType == shader_stage::tessellation_control || 
			ShaderType == shader_stage::tessellation_eval)
		{
			ComPtr<IDxcBlobEncoding> SourceBlob;
			if (UseDxcCompiler3)
			{
				hr = DxcUtils->CreateBlobFromPinned(ShaderCode.c_str(), static_cast<UINT32>(ShaderCode.size()), CP_UTF8, &SourceBlob);
			}
			else
			{
				hr = DxcLib->CreateBlobWithEncodingFromPinned(ShaderCode.c_str(), static_cast<UINT32>(ShaderCode.size()), CP_UTF8, &SourceBlob);
			}

			if (FAILED(hr))
			{
				std::cerr << "Failed to create source blob." << std::endl;
				return {};
			}

			ComPtr<IDxcBlob> ShaderBlob;
			if(UseDxcCompiler3)
			{
				DxcBuffer SourceBuffer = {};
				SourceBuffer.Ptr = SourceBlob->GetBufferPointer();
				SourceBuffer.Size = SourceBlob->GetBufferSize();
				//SourceBuffer.Encoding = DXC_CP_UTF8;

				ComPtr<IDxcResult> CompileResult;
				hr = DxcCompiler3->Compile(&SourceBuffer, ShaderCompileArgs.data(), ShaderCompileArgs.size(), nullptr, IID_PPV_ARGS(&CompileResult));
				if (FAILED(hr)) { /* Handle error */ }

				HRESULT hrStatus;
				hr = CompileResult->GetStatus(&hrStatus);
				if (FAILED(hrStatus))
				{
					ComPtr<IDxcBlobUtf8> Errors;
					hr = CompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&Errors), nullptr);
					if (SUCCEEDED(hr) && Errors && Errors->GetStringLength() > 0)
					{
						std::cerr << "Shader compilation errors:\n" << Errors->GetStringPointer() << std::endl;
					}
					return {};
				}

				hr = CompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderBlob), nullptr);
				if (FAILED(hr) || ShaderBlob == nullptr)
				{
					std::cerr << "Failed to retrieve compiled shader code." << std::endl;
					return {};
				}
			}
			else
			{
				ComPtr<IDxcOperationResult> CompileResult;
				hr = DxcCompiler->Compile(SourceBlob.Get(), nullptr, L"main", TargetProfile, ShaderCompileArgs.data(), static_cast<UINT32>(ShaderCompileArgs.size()), nullptr, 0, nullptr, &CompileResult);

				HRESULT hrStatus;
				hr = CompileResult->GetStatus(&hrStatus);
				if (FAILED(hrStatus))
				{
					ComPtr<IDxcBlobEncoding> ErrorsBlob;
					hr = CompileResult->GetErrorBuffer(&ErrorsBlob);
					if (SUCCEEDED(hr) && ErrorsBlob)
					{
						std::cerr << "Shader compilation errors:\n" << (const char*)ErrorsBlob->GetBufferPointer() << std::endl;
					}
					return {};
				}

				hr = CompileResult->GetResult(&ShaderBlob);
				if (FAILED(hr) || ShaderBlob == nullptr)
				{
					std::cerr << "Failed to retrieve compiled shader code." << std::endl;
					return {};
				}
			}

			const void* BlobDataPtr = ShaderBlob->GetBufferPointer();
			size_t BlobDataSize = ShaderBlob->GetBufferSize();

			SpirvCode.resize(BlobDataSize / sizeof(uint32_t));
			memcpy(SpirvCode.data(), BlobDataPtr, BlobDataSize);
		}
		else
		{
			glslang::InitializeProcess();

			EShLanguage LanguageStage = EShLangVertex;
			glslang::EShTargetClientVersion ClientVersion = glslang::EShTargetVulkan_1_0;
			glslang::EShTargetLanguageVersion TargetLanguageVersion = glslang::EShTargetSpv_1_0;
			switch (ShaderType)
			{
				case shader_stage::vertex:
				{
					std::string ReplaceString = "gl_DrawID";
					size_t DrawIdPos = ShaderCode.find(ReplaceString);
					if (DrawIdPos != std::string::npos)
					{
						HaveDrawID = true;
						ShaderCode.replace(DrawIdPos, ReplaceString.length(), "0");
					}
				} break;
				case shader_stage::tessellation_control:
					LanguageStage = EShLangTessControl;
					break;
				case shader_stage::tessellation_eval:
					LanguageStage = EShLangTessEvaluation;
					break;
				case shader_stage::geometry:
					LanguageStage = EShLangGeometry;
					break;
				case shader_stage::fragment:
					LanguageStage = EShLangFragment;
					break;
				case shader_stage::compute:
					LanguageStage = EShLangCompute;
					break;
			}

			const char* GlslSource = ShaderCode.c_str();
			glslang::TShader ShaderModule(LanguageStage);
			ShaderModule.setStrings(&GlslSource, 1);

			ShaderModule.setEnvInput(glslang::EShSourceGlsl, LanguageStage, glslang::EShClientVulkan, 100);
			ShaderModule.setEnvClient(glslang::EShClientVulkan, ClientVersion);
			ShaderModule.setEnvTarget(glslang::EshTargetSpv, TargetLanguageVersion);

			const char* ShaderStrings[1];
			ShaderStrings[0] = ShaderCode.c_str();
			ShaderModule.setStrings(ShaderStrings, 1);

			TBuiltInResource DefaultBuiltInResource = GetDefaultBuiltInResource();
			if (!ShaderModule.parse(&DefaultBuiltInResource, 100, false, static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault)))
			{
				std::cerr << ShaderModule.getInfoLog() << std::endl;
				std::cerr << ShaderModule.getInfoDebugLog() << std::endl;
				return {};
			}

			glslang::TProgram Program;
			Program.addShader(&ShaderModule);

			if (!Program.link(static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules)))
			{
				std::cerr << Program.getInfoLog() << std::endl;
				std::cerr << Program.getInfoDebugLog() << std::endl;
				return {};
			}

			glslang::TIntermediate *Intermediate = Program.getIntermediate(ShaderModule.getStage());
			glslang::GlslangToSpv(*Intermediate, SpirvCode);

			glslang::FinalizeProcess();
		}

		std::vector<op_info> ShaderInfo;
		std::set<u32> DescriptorIndices;
		ParseSpirv(SpirvCode, ShaderInfo, DescriptorIndices, nullptr, nullptr, nullptr, LocalSizeX, LocalSizeY, LocalSizeZ);

		CompiledShaders[Path].LocalSizeX = LocalSizeX ? *LocalSizeX : 0;
		CompiledShaders[Path].LocalSizeY = LocalSizeY ? *LocalSizeY : 0;
		CompiledShaders[Path].LocalSizeZ = LocalSizeZ ? *LocalSizeZ : 0;

		std::map<u32, std::map<u32, u32>> ShaderToUse;
		std::map<u32, std::map<u32, u32>> ParameterOffsets;
		std::map<u32, std::map<u32, descriptor_param>> ParameterLayoutTemp;
		for(u32 VariableIdx = 0; VariableIdx < ShaderInfo.size(); VariableIdx++)
		{
			const op_info& Var = ShaderInfo[VariableIdx];
			if (Var.OpCode == SpvOpVariable && Var.StorageClass == SpvStorageClassPushConstant)
			{
				const op_info& VariableType = ShaderInfo[Var.TypeId[0]];
				const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];
				HavePushConstant = true;

				PushConstantSize += GetSpvVariableSize(ShaderInfo, PointerType);
			}
			else if (Var.OpCode == SpvOpVariable && Var.IsDescriptor)
			{
				const op_info& VariableType = ShaderInfo[Var.TypeId[0]];

				u32 Size = ~0u;
				u32 StorageClass = Var.StorageClass;
				bool NonWritable = false;
				dx12_descriptor_type DescriptorType;
				image_type& ImageType = ParameterLayoutTemp[Var.Set][Var.Binding].ImageType;
				if(VariableType.OpCode == SpvOpTypePointer)
				{
					const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];

					if(PointerType.OpCode == SpvOpTypeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];
						const op_info& SizeInfo  = ShaderInfo[PointerType.SizeId];

						Size = SizeInfo.Constant;
						NonWritable = ArrayInfo.NonWritable;

						DescriptorType = GetDXSpvDescriptorType(ShaderInfo, ArrayInfo, ImageType, StorageClass, NonWritable);
					}
					else if(PointerType.OpCode == SpvOpTypeRuntimeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];

						NonWritable = ArrayInfo.NonWritable;

						DescriptorType = GetDXSpvDescriptorType(ShaderInfo, ArrayInfo, ImageType, StorageClass, NonWritable);
					}
					else
					{
						NonWritable = PointerType.NonWritable;
						DescriptorType = GetDXSpvDescriptorType(ShaderInfo, PointerType, ImageType, StorageClass, NonWritable);
					}
				}
				else
				{
					NonWritable = VariableType.NonWritable;
					DescriptorType = GetDXSpvDescriptorType(ShaderInfo, VariableType, ImageType, StorageClass, NonWritable);
				}

				if(DescriptorType == dx12_descriptor_type::unordered_access)
				{
					ParameterLayoutTemp[Var.Set][Var.Binding].Type = resource_type::buffer_storage;
					ParameterLayoutTemp[Var.Set][Var.Binding].IsWritable = true;
				}
				else if(DescriptorType == dx12_descriptor_type::shader_resource || DescriptorType == dx12_descriptor_type::constant_buffer)
				{
					ParameterLayoutTemp[Var.Set][Var.Binding].Type = resource_type::buffer_uniform;
					ParameterLayoutTemp[Var.Set][Var.Binding].IsWritable = false;
				}
				else if(DescriptorType == dx12_descriptor_type::image)
				{
					ParameterLayoutTemp[Var.Set][Var.Binding].Type = resource_type::texture_storage;
					ParameterLayoutTemp[Var.Set][Var.Binding].IsWritable = true;
				}
				else if(DescriptorType == dx12_descriptor_type::sampler)
				{
					ParameterLayoutTemp[Var.Set][Var.Binding].Type = resource_type::texture_sampler;
					ParameterLayoutTemp[Var.Set][Var.Binding].IsWritable = false;
				}
				else if(DescriptorType == dx12_descriptor_type::combined_image_sampler)
				{
					ParameterLayoutTemp[Var.Set][Var.Binding].Type = resource_type::texture_sampler;
					ParameterLayoutTemp[Var.Set][Var.Binding].IsWritable = false;
				}

				if((DescriptorType == dx12_descriptor_type::image || 
					DescriptorType == dx12_descriptor_type::sampler || 
					DescriptorType == dx12_descriptor_type::combined_image_sampler) && Size == ~0u)
					Size = 1;

				if(Size != ~0u)
				{
					ParameterOffsets[Var.Set][Var.Binding] = Size;
					D3D12_DESCRIPTOR_RANGE* ParameterRange = PushStruct(D3D12_DESCRIPTOR_RANGE);
					switch(DescriptorType)
					{
					case dx12_descriptor_type::shader_resource:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += Size;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::shader_read;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderRead;
					} break;
					case dx12_descriptor_type::image:
					case dx12_descriptor_type::unordered_access:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += Size;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::general;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderWrite;
					} break;
					case dx12_descriptor_type::constant_buffer:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += Size;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::shader_read;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderRead;
					} break;
					case dx12_descriptor_type::combined_image_sampler:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						D3D12_DESCRIPTOR_RANGE* ParameterRange1 = PushStruct(D3D12_DESCRIPTOR_RANGE);
						ParameterRange1->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						ParameterRange1->BaseShaderRegister = Var.Binding;
						ParameterRange1->NumDescriptors = Size;
						ParameterRange1->RegisterSpace = Var.Set;
						ParameterRange1->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						D3D12_ROOT_PARAMETER& Parameter1 = ShaderRootLayout[Var.Set][Var.Binding][1];
						Parameter1.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
						Parameter1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //GetDXVisibility(ShaderType);
						Parameter1.DescriptorTable  = {1, ParameterRange1};

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += Size;
						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]     += Size;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::shader_read;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderRead;
					} break;
					case dx12_descriptor_type::sampler:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]     += Size;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::shader_read;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderRead;
					} break;
					}

					D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[Var.Set][Var.Binding][0];
					Parameter.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //GetDXVisibility(ShaderType);
					Parameter.DescriptorTable  = {1, ParameterRange};

					ParameterLayoutTemp[Var.Set][Var.Binding].Count = Size;
					//ParameterLayoutTemp[Var.Set][Var.Binding].ImageType = TextureType;
					ParameterLayoutTemp[Var.Set][Var.Binding].ShaderToUse |= GetShaderFlag(ShaderType);
				}
				else
				{
					ParameterOffsets[Var.Set][Var.Binding] = 0;
					D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[Var.Set][Var.Binding][0];
					switch(DescriptorType)
					{
					case dx12_descriptor_type::shader_resource:
					{
						Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::shader_read;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderRead;
					} break;
					case dx12_descriptor_type::unordered_access:
					{
						Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;

						ParameterLayoutTemp[Var.Set][Var.Binding].BarrierState = barrier_state::general;
						ParameterLayoutTemp[Var.Set][Var.Binding].AspectMask = AF_ShaderWrite;
					} break;
					case dx12_descriptor_type::constant_buffer:
					{
						Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;

						ParameterLayout[Var.Set][Var.Binding].BarrierState = barrier_state::shader_read;
						ParameterLayout[Var.Set][Var.Binding].AspectMask = AF_ShaderRead;
					} break;
					}
					DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += 1;
					Parameter.Descriptor.ShaderRegister = Var.Binding;
					Parameter.Descriptor.RegisterSpace  = Var.Set;
					Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //GetDXVisibility(ShaderType);

					ParameterLayoutTemp[Var.Set][Var.Binding].Count = 1;
					ParameterLayoutTemp[Var.Set][Var.Binding].ShaderToUse |= GetShaderFlag(ShaderType);
				}
			}
		}

		for (const auto& [Set, Bindings] : ParameterLayoutTemp)
		{
			for (const auto& [Binding, Param] : Bindings)
			{
				ParameterLayout[Set][Binding] = Param;
			}
		}
		CompiledShaders[Path].ParameterLayout.insert(ParameterLayoutTemp.begin(), ParameterLayoutTemp.end());
		CompiledShaders[Path].ShaderRootLayout.insert(ShaderRootLayout.begin(), ShaderRootLayout.end());
		CompiledShaders[Path].DescriptorHeapSizes.insert(DescriptorHeapSizes.begin(), DescriptorHeapSizes.end());
		CompiledShaders[Path].HavePushConstant = HavePushConstant;
		CompiledShaders[Path].PushConstantSize = PushConstantSize;
		CompiledShaders[Path].HaveDrawID = HaveDrawID;

		std::string HlslCode;
		if (ShaderType != shader_stage::geometry &&
			ShaderType != shader_stage::tessellation_control &&
			ShaderType != shader_stage::tessellation_eval)
		{
			try
			{
				spirv_cross::CompilerHLSL Compiler(SpirvCode.data(), SpirvCode.size());
				auto HlslResources = Compiler.get_shader_resources();

				for (auto &SetBinding : NewBindings)
				{
					uint32_t Set = SetBinding.first;
					uint32_t Offs = 0;
					uint32_t Curr = 0;

					for (auto &Binding : SetBinding.second)
					{
						uint32_t Old = Binding.first;

						if (ParameterOffsets[Set].find(Old) != ParameterOffsets[Set].end())
						{
							uint32_t Size = ParameterOffsets[Set][Old];
							Binding.second = Offs;
							Offs += (Size > 0) ? Size : 1;
						}
						else
						{
							Offs += 1;
						}
						Curr += 1;
					}
				}

				for (const auto &SetBinding : ParameterOffsets)
				{
					uint32_t Set = SetBinding.first;
					if (NewBindings.find(Set) == NewBindings.end())
					{
						NewBindings[Set] = std::map<u32, u32>();
					}
					
					uint32_t Offs = NewBindings[Set].size();
					for (const auto &Binding : SetBinding.second)
					{
						uint32_t Old = Binding.first;
						uint32_t Size = Binding.second;

						if (NewBindings[Set].find(Old) == NewBindings[Set].end())
						{
							NewBindings[Set][Old] = Offs;
							Offs += (Size > 0) ? Size : 1;
						}
					}
				}

				for (const auto &resource : HlslResources.sampled_images) 
				{
					uint32_t Set = Compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					uint32_t Binding = Compiler.get_decoration(resource.id, spv::DecorationBinding);
					Compiler.set_decoration(resource.id, spv::DecorationBinding, NewBindings[Set][Binding]);
				}

				for (const auto &resource : HlslResources.storage_images) 
				{
					uint32_t Set = Compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					uint32_t Binding = Compiler.get_decoration(resource.id, spv::DecorationBinding);
					Compiler.set_decoration(resource.id, spv::DecorationBinding, NewBindings[Set][Binding]);
				}

				for (const auto &resource : HlslResources.storage_buffers) 
				{
					uint32_t Set = Compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					uint32_t Binding = Compiler.get_decoration(resource.id, spv::DecorationBinding);
					Compiler.set_decoration(resource.id, spv::DecorationBinding, NewBindings[Set][Binding]);
				}

				spirv_cross::CompilerGLSL::Options    CommonOptions;
				spirv_cross::CompilerHLSL::Options    HlslOptions;
				spirv_cross::HLSLVertexAttributeRemap HlslAttribs;

				HlslOptions.shader_model = 62;
				HlslOptions.use_entry_point_name = true;
				HlslOptions.enable_16bit_types   = true;
				CommonOptions.vertex.flip_vert_y = true;

				Compiler.set_hlsl_options(HlslOptions);
				Compiler.set_common_options(CommonOptions);
				Compiler.add_vertex_attribute_remap(HlslAttribs);

				HlslCode = Compiler.compile();

				CompiledShaders[Path].NewBindings = NewBindings;
			}
			catch(const std::exception& e)
			{
				std::cout << e.what() << std::endl;
				return {};
			}
		}
		else
		{
			HlslCode.insert(0, ShaderCode);
			HlslCode.insert(0, ShaderDefinesResult);
		} 

		if(HaveDrawID) 
		{ 
			HlslCode.insert(0, "cbuffer root_constant\n{\n\tuint RootDrawID : register(c0, space"+std::to_string(HavePushConstant)+");\n};\n\n"); 
			std::string ReplaceString = "DrawID = 0u";
			size_t DrawIdPos = HlslCode.find(ReplaceString);
			if (DrawIdPos != std::string::npos)
			{
				HlslCode.replace(DrawIdPos, ReplaceString.length(), "DrawID = RootDrawID");
			}
		}

        ComPtr<IDxcBlobEncoding> SourceBlob;
        if (UseDxcCompiler3)
        {
            hr = DxcUtils->CreateBlobFromPinned(HlslCode.c_str(), static_cast<UINT32>(HlslCode.size()), CP_UTF8, &SourceBlob);
        }
        else
        {
            hr = DxcLib->CreateBlobWithEncodingFromPinned(HlslCode.c_str(), static_cast<UINT32>(HlslCode.size()), CP_UTF8, &SourceBlob);
        }

        if (FAILED(hr))
        {
            std::cerr << "Failed to create source blob." << std::endl;
            return {};
        }

		std::vector<LPCWSTR> Arguments;
		Arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#if defined(CE_DEBUG)
		//Arguments.push_back(L"-Zi");
		//Arguments.push_back(L"-Qembed_debug");
		//Arguments.push_back(L"-Qstrip_debug");
		//Arguments.push_back(L"-Qstrip_reflect");
#endif
		Arguments.push_back(L"-T");
		if (ShaderType == shader_stage::vertex)
			Arguments.push_back(L"vs_6_2");
		else if (ShaderType == shader_stage::fragment)
			Arguments.push_back(L"ps_6_2");
		else if (ShaderType == shader_stage::compute)
			Arguments.push_back(L"cs_6_2");
		else if (ShaderType == shader_stage::geometry)
			Arguments.push_back(L"gs_6_2");
		else if (ShaderType == shader_stage::tessellation_control)
			Arguments.push_back(L"hs_6_2");
		else if (ShaderType == shader_stage::tessellation_eval)
			Arguments.push_back(L"ds_6_2");

		ComPtr<IDxcBlob> Code;
		if(UseDxcCompiler3)
		{
			DxcBuffer SourceBuffer = {};
			SourceBuffer.Ptr = SourceBlob->GetBufferPointer();
			SourceBuffer.Size = SourceBlob->GetBufferSize();
			//SourceBuffer.Encoding = DXC_CP_UTF8;

			ComPtr<IDxcResult> CompileResult;
			hr = DxcCompiler3->Compile(&SourceBuffer, Arguments.data(), Arguments.size(), nullptr, IID_PPV_ARGS(&CompileResult));
			if (FAILED(hr)) { /* Handle error */ }

			HRESULT hrStatus;
            hr = CompileResult->GetStatus(&hrStatus);
            if (FAILED(hrStatus))
            {
				ComPtr<IDxcBlobUtf8> Errors;
                hr = CompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&Errors), nullptr);
                if (SUCCEEDED(hr) && Errors && Errors->GetStringLength() > 0)
                {
                    std::cerr << "Shader compilation errors:\n" << Errors->GetStringPointer() << std::endl;
                }
                return {};
            }

            hr = CompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&Code), nullptr);
            if (FAILED(hr) || Code == nullptr)
            {
                std::cerr << "Failed to retrieve compiled shader code." << std::endl;
                return {};
            }
		}
		else
		{
			ComPtr<IDxcOperationResult> CompileResult;
            hr = DxcCompiler->Compile(SourceBlob.Get(), nullptr, L"main", TargetProfile, Arguments.data(), static_cast<UINT32>(Arguments.size()), nullptr, 0, nullptr, &CompileResult);

            HRESULT hrStatus;
            hr = CompileResult->GetStatus(&hrStatus);
            if (FAILED(hrStatus))
            {
                ComPtr<IDxcBlobEncoding> ErrorsBlob;
                hr = CompileResult->GetErrorBuffer(&ErrorsBlob);
                if (SUCCEEDED(hr) && ErrorsBlob)
                {
                    std::cerr << "Shader compilation errors:\n" << (const char*)ErrorsBlob->GetBufferPointer() << std::endl;
                }
                return {};
            }

            hr = CompileResult->GetResult(&Code);
            if (FAILED(hr) || Code == nullptr)
            {
                std::cerr << "Failed to retrieve compiled shader code." << std::endl;
                return {};
            }
		}

		Result.BytecodeLength  = Code->GetBufferSize();
		Result.pShaderBytecode = PushArray(u8, Result.BytecodeLength);
		memcpy((void*)Result.pShaderBytecode, Code->GetBufferPointer(), Result.BytecodeLength);

		CompiledShaders[Path].Handle = Result;
	}

	File.close();
	return Result;
}
