
directx12_backend::
directx12_backend(window* Window)
{
	RECT WindowRect;
	GetClientRect(Window->Handle, &WindowRect);
	u32 ClientWidth  = WindowRect.right - WindowRect.left;
	u32 ClientHeight = WindowRect.bottom - WindowRect.top;

	Width  = ClientWidth;
	Height = ClientHeight;

	UINT FactoryFlags = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug>  Debug;
	ComPtr<ID3D12Debug1> Debug1;
	{
		D3D12GetDebugInterface(IID_PPV_ARGS(&Debug));
		Debug->EnableDebugLayer();
		if(SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug1))))
		{
			FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			Debug1->EnableDebugLayer();
			Debug1->SetEnableGPUBasedValidation(TRUE);  // enable GPU-based validation
			Debug1->Release();
		}
	}

	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings))))
	{
		pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
#endif
	
	CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory));

#if 0
	if (UseWarp)
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));
		D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	}
	else
#endif
	{
		GetDevice(&Device, &Adapter);
	}

#if _DEBUG
	{
		ID3D12InfoQueue1* InfoQueue = nullptr;
		if (SUCCEEDED(Device->QueryInterface<ID3D12InfoQueue1>(&InfoQueue)))
		{
			InfoQueue->RegisterMessageCallback(&MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &MsgCallback);
		}
	}
#endif

	Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingSupport, sizeof(TearingSupport));

	D3D12_FEATURE_DATA_FORMAT_SUPPORT DataFormatSupport = {ColorTargetFormat, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
	Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &DataFormatSupport, sizeof(DataFormatSupport));

	CommandQueue = new directx12_command_queue(Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	//CmpCommandQueue = new directx12_command_queue(Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);

	{
		ComPtr<IDXGISwapChain1> _SwapChain;
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Flags = (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		SwapChainDesc.Width = ClientWidth;
		SwapChainDesc.Height = ClientHeight;
		SwapChainDesc.BufferCount = 2;
		SwapChainDesc.Format = ColorTargetFormat;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
		Factory->CreateSwapChainForHwnd(CommandQueue->Handle.Get(), Window->Handle, &SwapChainDesc, nullptr, nullptr, _SwapChain.GetAddressOf());
		_SwapChain.As(&SwapChain);

		SwapchainImages.resize(2);
		SwapchainCurrentState.resize(2);
	}

	ColorTargetHeap  = descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DepthStencilHeap = descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	ResourcesHeap    = descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	SamplersHeap     = descriptor_heap(Device.Get(), DX12_RESOURCE_LIMIT, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	NAME_DX12_OBJECT_CSTR(ColorTargetHeap.Handle.Get(), "GlobalColorTargetHeap");
	NAME_DX12_OBJECT_CSTR(DepthStencilHeap.Handle.Get(), "GlobalDepthStencilHeap");
	NAME_DX12_OBJECT_CSTR(ResourcesHeap.Handle.Get(), "GlobalResourcesHeap");
	NAME_DX12_OBJECT_CSTR(SamplersHeap.Handle.Get(), "GlobalSamplersHeap");

	{
		D3D12_RENDER_TARGET_VIEW_DESC ColorTargetViewDesc = {};
		ColorTargetViewDesc.Format = ColorTargetFormat;
		ColorTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

		SwapChain->GetBuffer(0, IID_PPV_ARGS(&SwapchainImages[0]));
		Device->CreateRenderTargetView(SwapchainImages[0].Get(), &ColorTargetViewDesc, ColorTargetHeap.GetNextCpuHandle());
		SwapChain->GetBuffer(1, IID_PPV_ARGS(&SwapchainImages[1]));
		Device->CreateRenderTargetView(SwapchainImages[1].Get(), &ColorTargetViewDesc, ColorTargetHeap.GetNextCpuHandle());
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
};

void directx12_backend::
DestroyObject()
{
}

void directx12_backend::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
#if 0
	Fence.Flush(CommandQueue);
	ID3D12GraphicsCommandList* CommandList = CommandQueue->AllocateCommandList();

	heap_alloc RtvHeapMap = RtvHeap.Allocate(2);
	heap_alloc DsvHeapMap = DsvHeap.Allocate(1);
	RtvHeap.Reset();
	DsvHeap.Reset();

	Width  = NewWidth;
	Height = NewHeight;
	Viewport.Width  = (r32)NewWidth;
	Viewport.Height = (r32)NewHeight;
	Rect = { 0, 0, (LONG)NewWidth, (LONG)NewHeight };

	CommandQueue->Reset();
	CommandList->Reset(CommandQueue->CommandAlloc.Get(), nullptr);

	for (u32 Idx = 0;
		Idx < 2;
		++Idx)
	{
		BackBuffers[Idx].Handle.Reset();
	}
	DepthStencilBuffer.Handle.Reset();

	SwapChain->ResizeBuffers(2, NewWidth, NewHeight, ColorTargetFormat, (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	for (u32 Idx = 0;
		Idx < 2;
		++Idx)
	{
		SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&BackBuffers[Idx].Handle));
		Device->CreateRenderTargetView(BackBuffers[Idx].Handle.Get(), nullptr, RtvHeapMap.GetNextCpuHandle());
	}

	D3D12_RESOURCE_DESC DepthStencilDesc = {};
	DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	DepthStencilDesc.Width = NewWidth;
	DepthStencilDesc.Height = NewHeight;
	DepthStencilDesc.DepthOrArraySize = 1;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.Format = DepthBufferFormat;
	DepthStencilDesc.SampleDesc.Count = MsaaState ? MsaaQuality : 1;
	DepthStencilDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
	DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE Clear = {};
	Clear.Format = DepthBufferFormat;
	Clear.DepthStencil.Depth = 1.0f;
	Clear.DepthStencil.Stencil = 0;
	auto HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer.Handle));

	Device->CreateDepthStencilView(DepthStencilBuffer.Handle.Get(), nullptr, DsvHeapMap.GetNextCpuHandle());

	CommandQueue.ExecuteAndRemove(CommandList);
	Fence.Flush(CommandQueue);
#endif
}

dx12_descriptor_type GetDXSpvDescriptorType(u32 OpCode, u32 StorageClass, bool NonWritable)
{
	switch(OpCode)
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
			return dx12_descriptor_type::image;
		} break;
		case SpvOpTypeSampler:
		{
			return dx12_descriptor_type::sampler;
		} break;
		case SpvOpTypeSampledImage:
		{
			return dx12_descriptor_type::combined_image_sampler;
		} break;
	}
}

[[nodiscard]] D3D12_SHADER_BYTECODE directx12_backend::
LoadShaderModule(const char* Path, shader_stage ShaderType, bool& HaveDrawID, u32* ArgumentsCount, std::map<u32, std::map<u32, std::map<u32, D3D12_ROOT_PARAMETER>>>& ShaderRootLayout, bool& HavePushConstant, u32& PushConstantSize, std::unordered_map<u32, u32>& DescriptorHeapSizes, const std::vector<shader_define>& ShaderDefines)
{
	D3D12_SHADER_BYTECODE Result = {};

	std::ifstream File(Path);
	if(File)
	{
		std::string ShaderDefinesResult;
		for(const shader_define& Define : ShaderDefines)
		{
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

		glslang::InitializeProcess();

		EShLanguage LanguageStage = EShLangVertex;
		glslang::EShTargetClientVersion ClientVersion = glslang::EShTargetVulkan_1_0;
		glslang::EShTargetLanguageVersion TargetLanguageVersion = glslang::EShTargetSpv_1_0;
		switch (ShaderType)
		{
			case shader_stage::vertex:
			{
#if 0
				size_t MainPos = ShaderCode.find("void main");
				if (MainPos != std::string::npos)
				{
					ShaderCode = ShaderCode.substr(0, MainPos) + "\n" + "uint GetDrawID() { return 0; }\n\n" + ShaderCode.substr(MainPos);
				}
#endif

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

		spirv_cross::CompilerHLSL Compiler(SpirvCode.data(), SpirvCode.size());

		std::vector<op_info> ShaderInfo;
		std::set<u32> DescriptorIndices;
		ParseSpirv(SpirvCode, ShaderInfo, DescriptorIndices);

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
				dx12_descriptor_type DescriptorType;
				if(VariableType.OpCode == SpvOpTypePointer)
				{
					const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];

					if(PointerType.OpCode == SpvOpTypeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];
						const op_info& SizeInfo  = ShaderInfo[PointerType.SizeId];

						Size = SizeInfo.Constant;

						DescriptorType = GetDXSpvDescriptorType(ArrayInfo.OpCode, StorageClass, ArrayInfo.NonWritable);
					}
					else if(PointerType.OpCode == SpvOpTypeRuntimeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];

						DescriptorType = GetDXSpvDescriptorType(ArrayInfo.OpCode, StorageClass, ArrayInfo.NonWritable);
					}
					else
					{
						DescriptorType = GetDXSpvDescriptorType(PointerType.OpCode, StorageClass, PointerType.NonWritable);
					}
				}
				else
				{
					DescriptorType = GetDXSpvDescriptorType(VariableType.OpCode, StorageClass, VariableType.NonWritable);
				}

				if((DescriptorType == dx12_descriptor_type::image || DescriptorType == dx12_descriptor_type::sampler || DescriptorType == dx12_descriptor_type::combined_image_sampler) && Size == ~0u)
					Size = 1;

				if(ArgumentsCount) (*ArgumentsCount)++;
				if(Size != ~0u)
				{
					D3D12_DESCRIPTOR_RANGE* ParameterRange = new D3D12_DESCRIPTOR_RANGE;
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
					} break;
					case dx12_descriptor_type::constant_buffer:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += Size;
					} break;
					case dx12_descriptor_type::combined_image_sampler:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						D3D12_DESCRIPTOR_RANGE* ParameterRange1 = new D3D12_DESCRIPTOR_RANGE;
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
					} break;
					case dx12_descriptor_type::sampler:
					{
						ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						ParameterRange->BaseShaderRegister = Var.Binding;
						ParameterRange->NumDescriptors = Size;
						ParameterRange->RegisterSpace = Var.Set;
						ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]     += Size;
					} break;
					}

					D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[Var.Set][Var.Binding][0];
					Parameter.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //GetDXVisibility(ShaderType);
					Parameter.DescriptorTable  = {1, ParameterRange};
				}
				else
				{
					D3D12_ROOT_PARAMETER& Parameter = ShaderRootLayout[Var.Set][Var.Binding][0];
					switch(DescriptorType)
					{
					case dx12_descriptor_type::shader_resource:
					{
						Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
					} break;
					case dx12_descriptor_type::unordered_access:
					{
						Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
					} break;
					case dx12_descriptor_type::constant_buffer:
					{
						Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					} break;
					}
					DescriptorHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += 1;
					Parameter.Descriptor.ShaderRegister = Var.Binding;
					Parameter.Descriptor.RegisterSpace  = Var.Set;
					Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //GetDXVisibility(ShaderType);
				}
			}
		}

		auto HlslResources = Compiler.get_shader_resources();

		spirv_cross::CompilerHLSL::Options    HlslOptions;
		spirv_cross::HLSLVertexAttributeRemap HlslAttribs;

		HlslOptions.shader_model = 66; // SM6_6

		Compiler.set_hlsl_options(HlslOptions);
		Compiler.add_vertex_attribute_remap(HlslAttribs);

		std::string HlslCode = Compiler.compile();

		if(HaveDrawID)
		{
			HlslCode.insert(0, "cbuffer root_constant\n{\n\tuint RootDrawID : register(c0);\n};\n\n");

			std::string ReplaceString = "DrawID = 0u";
			size_t DrawIdPos = HlslCode.find(ReplaceString);
			if (DrawIdPos != std::string::npos)
			{
				HlslCode.replace(DrawIdPos, ReplaceString.length(), "DrawID = RootDrawID");
			}
		}

		ComPtr<IDxcCompiler> DxcCompiler;
		ComPtr<IDxcLibrary> DxcLib;
		ComPtr<IDxcUtils> DxcUtils;
		HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcCompiler));
				hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&DxcLib));
		        hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DxcUtils));

		u32 CodePage = CP_UTF8;
		ComPtr<IDxcBlobEncoding> SourceBlob;
		DxcLib->CreateBlobWithEncodingFromPinned(HlslCode.c_str(), static_cast<u32>(HlslCode.size()), CodePage, &SourceBlob);

		const wchar_t* TargetProfile = L"vs_6_6";
		if (ShaderType == shader_stage::fragment)
			TargetProfile = L"ps_6_6";
		else if (ShaderType == shader_stage::compute)
			TargetProfile = L"cs_6_6";
		else if (ShaderType == shader_stage::geometry)
			TargetProfile = L"gs_6_6";
		else if (ShaderType == shader_stage::tessellation_control)
			TargetProfile = L"hs_6_6";
		else if (ShaderType == shader_stage::tessellation_eval)
			TargetProfile = L"ds_6_6";

		DxcBuffer SourceBuffer = {};
		SourceBuffer.Ptr  = SourceBlob->GetBufferPointer();
		SourceBuffer.Size = SourceBlob->GetBufferSize();

		std::vector<LPCWSTR> Arguments;

		Arguments.push_back(L"-T");
		Arguments.push_back(TargetProfile);
		//Arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
		//Arguments.push_back(L"-HV 2021");
#if 0
		Arguments.push_back(DXC_ARG_DEBUG);
		Arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
		Arguments.push_back(DXC_ARG_PREFER_FLOW_CONTROL);
		Arguments.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE);
#endif
		//Arguments.push_back(L"-Qstrip_debug");
		//Arguments.push_back(L"-Qstrip_reflect");
		Arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);

		ComPtr<IDxcBlobUtf8> Errors;
		ComPtr<IDxcBlobUtf8> DebugData;
		ComPtr<IDxcBlobUtf16> DebugPath;
		ComPtr<IDxcResult> CompileResult;

		{
			IDxcOperationResult* OperationResult = nullptr;
			DxcCompiler->Compile(static_cast<IDxcBlob*>(SourceBlob.Get()), nullptr, L"main", TargetProfile, Arguments.data(), Arguments.size(), nullptr, 0, nullptr, &OperationResult);

			CompileResult.Attach(static_cast<IDxcResult*>(OperationResult));
		}

		CompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(Errors.GetAddressOf()), nullptr);
		CompileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(DebugData.GetAddressOf()), DebugPath.GetAddressOf());

		if(DebugData)
		{
			std::wstring OutputPath(reinterpret_cast<const wchar_t*>(DebugPath->GetBufferPointer()));
			std::wofstream PDBFile(OutputPath, std::ios::binary);
			PDBFile.write(reinterpret_cast<const wchar_t*>(DebugData->GetBufferPointer()), DebugData->GetBufferSize());
			PDBFile.close();
		}

		if (Errors && Errors->GetStringLength() > 0)
		{
			std::cerr << (const char*)Errors->GetStringPointer() << std::endl;
		}

		CompileResult->GetStatus(&hr);

		if(FAILED(hr))
		{
			if(CompileResult)
			{
				ComPtr<IDxcBlobEncoding> ErrorsBlob;
				hr = CompileResult->GetErrorBuffer(&ErrorsBlob);
				if (SUCCEEDED(hr) && ErrorsBlob)
				{
					std::cerr << (const char*)ErrorsBlob->GetBufferPointer() << std::endl;
					return {};
				}
			}
		}

		ComPtr<IDxcBlob> Code;
		CompileResult->GetResult(&Code);

		if (Code.Get() != NULL)
		{
			Result.BytecodeLength  = Code->GetBufferSize();
			Result.pShaderBytecode = new u8[Result.BytecodeLength];
			memcpy((void*)Result.pShaderBytecode, Code->GetBufferPointer(), Result.BytecodeLength);
			Code->Release();
		}
		else
		{
			std::cerr << "Failed to compile shader source" << std::endl;
			return {};
		}

		SourceBlob->Release();
	}

	File.close();
	return Result;
}
