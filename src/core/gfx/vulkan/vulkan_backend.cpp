
vulkan_backend::
vulkan_backend(window* Window)
{
	volkInitialize();

	VK_CHECK(vkEnumerateInstanceVersion(&HighestUsedVulkanVersion));

	VkApplicationInfo AppInfo = {};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "3D Renderer";
	AppInfo.apiVersion = HighestUsedVulkanVersion;

	std::vector<const char*> InstanceLayers;
	std::vector<const char*> RequiredInstanceLayers = 
	{
#if _DEBUG
		"VK_LAYER_KHRONOS_validation"
#endif
	};

	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> RequiredInstanceExtensions = 
	{
#if _WIN32
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	};

#if __linux__
	u32 RequiredInstanceExtensionCount;
	const char** RequiredGLFWExtensions = glfwGetRequiredInstanceExtensions(&RequiredInstanceExtensionCount);
	for(u32 ExtIdx = 0; ExtIdx < RequiredInstanceExtensionCount; ++ExtIdx)
	{
		InstanceExtensions.push_back(RequiredGLFWExtensions[ExtIdx]);
	}
#endif

	u32 AvailableInstanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&AvailableInstanceLayerCount, nullptr);
	std::vector<VkLayerProperties> AvailableInstanceLayers(AvailableInstanceLayerCount);
	vkEnumerateInstanceLayerProperties(&AvailableInstanceLayerCount, AvailableInstanceLayers.data());

	u32 AvailableInstanceExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &AvailableInstanceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> AvailableInstanceExtensions(AvailableInstanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &AvailableInstanceExtensionCount, AvailableInstanceExtensions.data());

	for(const char* Required : RequiredInstanceLayers)
	{
		bool Found = false;
		for(const auto& Available : AvailableInstanceLayers)
		{
			if(strcmp(Required, Available.layerName) == 0)
			{
				InstanceLayers.push_back(Required);
				Found = true;
				break;
			}
		}

		if(Found)
		{
			std::cout << "[Layer] " << std::string(Required) << " is available\n";
		}
		else
		{
			std::cout << "[Layer] " << std::string(Required) << " is not available\n";
		}
	}

	for(const char* Required : RequiredInstanceExtensions)
	{
		bool Found = false;
		for(const auto& Available : AvailableInstanceExtensions)
		{
			if(strcmp(Required, Available.extensionName) == 0)
			{
				InstanceExtensions.push_back(Required);
				Found = true;
				break;
			}
		}

		if(Found)
		{
			std::cout << "[Extension] " << std::string(Required) << " is available\n";
		}
		else
		{
			std::cout << "[Extension] " << std::string(Required) << " is not available\n";
		}
	}

	VkInstanceCreateInfo InstanceCreateInfo = {};
	InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceCreateInfo.pApplicationInfo = &AppInfo;
	InstanceCreateInfo.enabledLayerCount = (u32)InstanceLayers.size();
	InstanceCreateInfo.ppEnabledLayerNames = InstanceLayers.data();
	InstanceCreateInfo.enabledExtensionCount = (u32)InstanceExtensions.size();
	InstanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

	VK_CHECK(vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance));
	volkLoadInstance(Instance);

	VkDebugReportCallbackCreateInfoEXT DebugReportCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
	DebugReportCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	DebugReportCreateInfo.pfnCallback = DebugReportCallback;
	VK_CHECK(vkCreateDebugReportCallbackEXT(Instance, &DebugReportCreateInfo, nullptr, &DebugCallback));

	u32 PhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
	vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices.data());

	PhysicalDevice = PickPhysicalDevice(PhysicalDevices, &FamilyIndex);

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);
	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

	VkSampleCountFlags SampleCount = 
		PhysicalDeviceProperties.limits.framebufferColorSampleCounts & 
		PhysicalDeviceProperties.limits.framebufferDepthSampleCounts;

	if(SampleCount & VK_SAMPLE_COUNT_64_BIT) MsaaQuality = VK_SAMPLE_COUNT_64_BIT;
	if(SampleCount & VK_SAMPLE_COUNT_32_BIT) MsaaQuality = VK_SAMPLE_COUNT_32_BIT;
	if(SampleCount & VK_SAMPLE_COUNT_16_BIT) MsaaQuality = VK_SAMPLE_COUNT_16_BIT;
	if(SampleCount & VK_SAMPLE_COUNT_8_BIT)  MsaaQuality = VK_SAMPLE_COUNT_8_BIT;
	if(SampleCount & VK_SAMPLE_COUNT_2_BIT)  MsaaQuality = VK_SAMPLE_COUNT_2_BIT;

	std::vector<const char*> DeviceLayers;
	std::vector<const char*> RequiredDeviceLayers = 
	{
	};

	std::vector<const char*> DeviceExtensions;
	std::vector<const char*> RequiredDeviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
		VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
		VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
		VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
		VK_KHR_MULTIVIEW_EXTENSION_NAME,
		"VK_KHR_dynamic_rendering",
		"VK_EXT_descriptor_indexing",
	};

	u32 DeviceExtensionsCount = 0;
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionsCount, nullptr);
	std::vector<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionsCount);
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionsCount, AvailableDeviceExtensions.data());

	for(const char* Required : RequiredDeviceExtensions)
	{
		bool Found = false;
		for(const VkExtensionProperties& Available : AvailableDeviceExtensions)
		{
			if(strcmp(Required, Available.extensionName) == 0)
			{
				if(strcmp(Required, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME) == 0) IsPushDescriptorsFeatureEnabled = true;

				DeviceExtensions.push_back(Required);
				Found = true;
				break;
			}
		}

		if(Found)
		{
			std::cout << "[Extension] " << std::string(Required) << " is available\n";
		}
		else
		{
			std::cout << "[Extension] " << std::string(Required) << " is not available\n";
		}
	}

	float QueuePriorities[] = {1.0f};
	VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {};
	DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	DeviceQueueCreateInfo.queueFamilyIndex = FamilyIndex;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;

#if 0
	Features2.features.imageCubeArray = true;
	Features2.features.multiDrawIndirect = true;
	Features2.features.pipelineStatisticsQuery = true;
	Features2.features.shaderInt16 = true;
	Features2.features.shaderInt64 = true;

	Features11.storageBuffer16BitAccess = true;
	Features11.multiview = true;

	Features12.drawIndirectCount = true;
	Features12.shaderFloat16 = true;
	Features12.shaderInt8 = true;
	Features12.samplerFilterMinmax = true;
	Features12.descriptorIndexing = true;
	Features12.descriptorBindingPartiallyBound = true;

	Features13.maintenance4 = true;
	Features13.dynamicRendering = true;
#endif

	VkDeviceCreateInfo DeviceCreateInfo = {};
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;
	DeviceCreateInfo.enabledLayerCount = (u32)DeviceLayers.size();
	DeviceCreateInfo.ppEnabledLayerNames = DeviceLayers.data();
	DeviceCreateInfo.enabledExtensionCount = (u32)DeviceExtensions.size();
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	DeviceCreateInfo.pNext = &Features2;
	Features2.pNext  = &Features11;
	Features11.pNext = &Features12;
	Features12.pNext = &Features13;

	vkGetPhysicalDeviceFeatures2(PhysicalDevice, &Features2);
	VK_CHECK(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device), true);

#if _WIN32
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	SurfaceCreateInfo.hinstance = Window->WindowClass.Inst;
	SurfaceCreateInfo.hwnd = Window->Handle;
	VK_CHECK(vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, 0, &Surface));
#else
	glfwCreateWindowSurface(Instance, Window->Handle, nullptr, &Surface);
#endif

	SurfaceFormat = GetSwapchainFormat(PhysicalDevice, Surface);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);

	VkSurfaceCapabilitiesKHR ResizeCaps;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &ResizeCaps));
	Width = ResizeCaps.currentExtent.width;
	Height = ResizeCaps.currentExtent.height;

	VkCompositeAlphaFlagBitsKHR CompositeAlpha = 
		(SurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR :
		(SurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :
		(SurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

	VkSwapchainCreateInfoKHR SwapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	SwapchainCreateInfo.surface = Surface;
	SwapchainCreateInfo.minImageCount = 2;
	SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
	SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapchainCreateInfo.imageExtent.width = Width;
	SwapchainCreateInfo.imageExtent.height = Height;
	SwapchainCreateInfo.imageArrayLayers = 1;
	SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	SwapchainCreateInfo.queueFamilyIndexCount = 1;
	SwapchainCreateInfo.pQueueFamilyIndices = &FamilyIndex;
	SwapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	SwapchainCreateInfo.compositeAlpha = CompositeAlpha;
	SwapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, 0, &Swapchain));

	u32 SwapchainImageCount = 0;
	vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, 0);
	SwapchainImages.resize(SwapchainImageCount);
	vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, SwapchainImages.data());

	CommandQueue = new vulkan_command_queue(Device, FamilyIndex);

	VkDescriptorPoolSize ImGuiPoolSizes[] = 
	{
		{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
	};

	VkDescriptorPoolCreateInfo ImGuiPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	ImGuiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	ImGuiPoolInfo.maxSets = 1000;
	ImGuiPoolInfo.poolSizeCount = std::size(ImGuiPoolSizes);
	ImGuiPoolInfo.pPoolSizes = ImGuiPoolSizes;

	VK_CHECK(vkCreateDescriptorPool(Device, &ImGuiPoolInfo, nullptr, &ImGuiPool));

	ImGui_ImplVulkan_InitInfo InitInfo = {};
	InitInfo.Instance = Instance;
	InitInfo.PhysicalDevice = PhysicalDevice;
	InitInfo.Device = Device;
	InitInfo.DescriptorPool = ImGuiPool;
	InitInfo.QueueFamily = FamilyIndex;
	InitInfo.Queue = CommandQueue->Handle;
	InitInfo.MinImageCount = 2;
	InitInfo.ImageCount = SwapchainImages.size();
	InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT; //MsaaQuality;
	InitInfo.UseDynamicRendering = true;
	InitInfo.ColorAttachmentFormat = SurfaceFormat.format;
	ImGui_ImplVulkan_Init(&InitInfo, VK_NULL_HANDLE);

	ImGui_ImplVulkan_CreateFontsTexture();
}

void vulkan_backend::
DestroyObject()
{
	vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
	vkDestroySwapchainKHR(Device, Swapchain, nullptr);
	vkDestroyDevice(Device, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDebugReportCallbackEXT(Instance, DebugCallback, nullptr);
	vkDestroyInstance(Instance, nullptr);
}

#if 0
#endif

struct op_info
{
	u32 Set;
	u32 Binding;

	u32 OpCode;
	std::vector<u32> TypeId;
	u32 SizeId;
	u32 StorageClass;
	u32 Constant;

	u32 Width;

	bool IsDescriptor;
	bool IsPushConstant;
	bool IsSigned;
};

void ParseSpirv(const std::vector<u32>& Binary, std::vector<op_info>& Info, std::set<u32>& DescriptorIndices)
{
	assert(Binary[0] == SpvMagicNumber);

	Info.resize(Binary[3]);

	u32 Offset = 5;
	u32 OpCode = 0;
	u32 OpSize = 0;
	u32 ResultId = 0;
	while(Offset != Binary.size())
	{
		OpCode = static_cast<u16>(Binary[Offset] & 0xffff);
		OpSize = static_cast<u16>(Binary[Offset] >> 16u);

		switch(OpCode)
		{
			case SpvOpDecorate:
			{
				ResultId = Binary[Offset + 1];

				Info[ResultId].OpCode = OpCode;

				switch(Binary[Offset + 2])
				{
					case SpvDecorationBinding:
					{
						Info[ResultId].Binding = Binary[Offset + 3];
						Info[ResultId].IsDescriptor = true;
						DescriptorIndices.insert(ResultId);
					} break;

					case SpvDecorationDescriptorSet:
					{
						Info[ResultId].Set = Binary[Offset + 3];
						Info[ResultId].IsDescriptor = true;
						DescriptorIndices.insert(ResultId);
					} break;
				}

			} break;

			case SpvOpVariable:
			{
				u32 TypeId      = Binary[Offset + 1];
				    ResultId    = Binary[Offset + 2];
				u32 StorageType = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].StorageClass = StorageType;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeVoid:
			{
				    ResultId = Binary[Offset + 1];
				Info[ResultId].OpCode = OpCode;
			} break;
			case SpvOpTypeBool:
			{
				    ResultId = Binary[Offset + 1];
				Info[ResultId].OpCode = OpCode;
			} break;
			case SpvOpTypeInt:
			{
				    ResultId = Binary[Offset + 1];
				u32 Width    = Binary[Offset + 2];
				u32 IsSigned = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Width;
				Info[ResultId].IsSigned = IsSigned;
			} break;
			case SpvOpTypeFloat:
			{
				    ResultId = Binary[Offset + 1];
				u32 Width    = Binary[Offset + 2];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Width;
			} break;
			case SpvOpTypeVector:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 Count    = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Count;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeMatrix:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 Count    = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Count;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeStruct:
			{
				    ResultId  = Binary[Offset + 1];
				u32 TypeCount = OpSize - 2;

				for(u32 TypeIdIdx = 0; TypeIdIdx < TypeCount; ++TypeIdIdx)
					Info[ResultId].TypeId.push_back(Binary[Offset + 2 + TypeIdIdx]);

				Info[ResultId].OpCode = OpCode;
			} break;
			case SpvOpTypeImage:
			case SpvOpTypeSampler:
			case SpvOpTypeSampledImage:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				Info[ResultId].OpCode = OpCode;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypePointer:
			{
				    ResultId    = Binary[Offset + 1];
				u32 StorageType = Binary[Offset + 2];
				u32 TypeId      = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].TypeId.push_back(TypeId);
				Info[ResultId].StorageClass = StorageType;
			} break;
			case SpvOpTypeArray:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 SizeId   = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].SizeId = SizeId;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeRuntimeArray:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				Info[ResultId].OpCode = OpCode;
				Info[ResultId].SizeId = ~0u;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpConstant:
			{
				u32 TypeId   = Binary[Offset + 1];
				    ResultId = Binary[Offset + 2];
				u32 Constant = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Constant = Constant;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
		}

		assert(Offset < Binary.size());
		Offset += OpSize;
	}
}

VkDescriptorType GetVkSpvDescriptorType(u32 OpCode, u32 StorageClass)
{
	switch(OpCode)
	{
		case SpvOpTypeStruct:
		{
			if(StorageClass == SpvStorageClassUniform || StorageClass == SpvStorageClassUniformConstant)
			{
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}
			else if(StorageClass == SpvStorageClassStorageBuffer)
			{
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}
		} break;
		case SpvOpTypeImage:
		{
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		} break;
		case SpvOpTypeSampler:
		{
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			//return VK_DESCRIPTOR_TYPE_SAMPLER;
		} break;
		case SpvOpTypeSampledImage:
		{
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		} break;
	}
}

// TODO: Parse for push constant sizes
// TODO: Maybe handle OpTypeRuntimeArray in the future if it will be possible
VkShaderModule vulkan_backend::
LoadShaderModule(const char* Path, shader_stage ShaderType, std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>>& ShaderRootLayout, std::map<VkDescriptorType, u32>& DescriptorTypeCounts, bool& HavePushConstant, u32& PushConstantSize, const std::vector<shader_define>& ShaderDefines)
{
	auto FoundCompiledShader = CompiledShaders.find(Path);
	if(FoundCompiledShader != CompiledShaders.end())
	{
		ShaderRootLayout.insert(FoundCompiledShader->second.ShaderRootLayout.begin(), FoundCompiledShader->second.ShaderRootLayout.end());
		DescriptorTypeCounts.insert(FoundCompiledShader->second.DescriptorTypeCounts.begin(), FoundCompiledShader->second.DescriptorTypeCounts.end());
		HavePushConstant = FoundCompiledShader->second.HavePushConstant;
		PushConstantSize = FoundCompiledShader->second.PushConstantSize;
		return FoundCompiledShader->second.Handle;
	}

	VkShaderModule Result = 0;
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

		if(HighestUsedVulkanVersion >= VK_API_VERSION_1_3)
		{
			ClientVersion = glslang::EShTargetVulkan_1_3;
			TargetLanguageVersion = glslang::EShTargetSpv_1_6;
		}
		else if(HighestUsedVulkanVersion >= VK_API_VERSION_1_2)
		{
			ClientVersion = glslang::EShTargetVulkan_1_2;
			TargetLanguageVersion = glslang::EShTargetSpv_1_5;
		}
		else if(HighestUsedVulkanVersion >= VK_API_VERSION_1_1)
		{
			ClientVersion = glslang::EShTargetVulkan_1_1;
			TargetLanguageVersion = glslang::EShTargetSpv_1_3;
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
			return VK_NULL_HANDLE;
		}

		glslang::TProgram Program;
		Program.addShader(&ShaderModule);

		if (!Program.link(static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules)))
		{
			std::cerr << Program.getInfoLog() << std::endl;
			std::cerr << Program.getInfoDebugLog() << std::endl;
			return VK_NULL_HANDLE;
		}

		glslang::TIntermediate *Intermediate = Program.getIntermediate(ShaderModule.getStage());
		glslang::GlslangToSpv(*Intermediate, SpirvCode);

		glslang::FinalizeProcess();

		std::vector<op_info> ShaderInfo;
		std::set<u32> DescriptorIndices;

		VkShaderModuleCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		CreateInfo.codeSize = SpirvCode.size() * sizeof(u32);
		CreateInfo.pCode = SpirvCode.data();

		VK_CHECK(vkCreateShaderModule(Device, &CreateInfo, 0, &Result));
		CompiledShaders[Path].Handle = Result;

		ParseSpirv(SpirvCode, ShaderInfo, DescriptorIndices);

		for(u32 VariableIdx = 0; VariableIdx < ShaderInfo.size(); VariableIdx++)
		{
			const op_info& Var = ShaderInfo[VariableIdx];
			// TODO: Better type size calculation
			if (Var.OpCode == SpvOpVariable && Var.StorageClass == SpvStorageClassPushConstant)
			{
				const op_info& VariableType = ShaderInfo[Var.TypeId[0]];

				CompiledShaders[Path].HavePushConstant = true;
				HavePushConstant = true;
				PushConstantSize = 0;

				u32 NewSize = 0;
				if(VariableType.OpCode == SpvOpTypePointer)
				{
					const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];

					switch(PointerType.OpCode)
					{
					case SpvOpTypeStruct:
					{
						for(u32 TypeIdIdx = 0; TypeIdIdx < PointerType.TypeId.size(); TypeIdIdx++)
						{
							const op_info& StructType = ShaderInfo[PointerType.TypeId[TypeIdIdx]];

							switch(StructType.OpCode)
							{
							case SpvOpTypeArray:
							{
								const op_info& ArrayInfo = ShaderInfo[StructType.TypeId[0]];
								const op_info& SizeInfo  = ShaderInfo[StructType.SizeId];
								switch(ArrayInfo.OpCode)
								{
								case SpvOpTypeMatrix:
								{
									NewSize = ArrayInfo.Width;
									const op_info& MatrixType = ShaderInfo[ArrayInfo.TypeId[0]];
									const op_info& VectorType = ShaderInfo[MatrixType.TypeId[0]];
									if(VectorType.OpCode == SpvOpTypeInt || VectorType.OpCode == SpvOpTypeFloat)
									{
										NewSize *= VectorType.Width / 8;
									}
								} break;
								case SpvOpTypeVector:
								{
									NewSize = ArrayInfo.Width;
									const op_info& VectorType = ShaderInfo[ArrayInfo.TypeId[0]];
									if(VectorType.OpCode == SpvOpTypeInt || VectorType.OpCode == SpvOpTypeFloat)
									{
										NewSize *= VectorType.Width / 8;
									}
								} break;
								case SpvOpTypeInt:
								case SpvOpTypeFloat:
								{
									NewSize = ArrayInfo.Width / 8;
								} break;
								}
								PushConstantSize += NewSize * SizeInfo.Constant;
							} break;
							case SpvOpTypeMatrix:
							{
								const op_info& MatrixType = ShaderInfo[StructType.TypeId[0]];
								const op_info& VectorType = ShaderInfo[MatrixType.TypeId[0]];
								if(VectorType.OpCode == SpvOpTypeInt || VectorType.OpCode == SpvOpTypeFloat)
								{
									PushConstantSize += StructType.Width * MatrixType.Width * VectorType.Width / 8;
								}
							} break;
							case SpvOpTypeVector:
							{
								const op_info& VectorType = ShaderInfo[StructType.TypeId[0]];
								if(VectorType.OpCode == SpvOpTypeInt || VectorType.OpCode == SpvOpTypeFloat)
								{
									PushConstantSize += StructType.Width * VectorType.Width / 8;
								}
							} break;
							case SpvOpTypeInt:
							case SpvOpTypeFloat:
							{
								PushConstantSize += StructType.Width / 8;
							} break;
							}
						}
					} break;
					case SpvOpTypeMatrix:
					{
						NewSize = PointerType.Width;
						const op_info& MatrixType = ShaderInfo[PointerType.TypeId[0]];
						const op_info& VectorType = ShaderInfo[MatrixType.TypeId[0]];
						if(VectorType.OpCode == SpvOpTypeInt || VectorType.OpCode == SpvOpTypeFloat)
						{
							PushConstantSize += NewSize * VectorType.Width / 8;
						}
					} break;
					case SpvOpTypeVector:
					{
						NewSize = PointerType.Width;
						const op_info& VectorType = ShaderInfo[PointerType.TypeId[0]];
						if(VectorType.OpCode == SpvOpTypeInt || VectorType.OpCode == SpvOpTypeFloat)
						{
							PushConstantSize += NewSize * VectorType.Width / 8;
						}
					} break;
					case SpvOpTypeInt:
					case SpvOpTypeFloat:
					{
						PushConstantSize += PointerType.Width / 8;
					} break;
					}
				}
			}
			else if (Var.OpCode == SpvOpVariable && Var.IsDescriptor)
			{
				const op_info& VariableType = ShaderInfo[Var.TypeId[0]];

				u32 Size = ~0u;
				u32 StorageClass = Var.StorageClass;
				VkDescriptorType DescriptorType;
				if(VariableType.OpCode == SpvOpTypePointer)
				{
					const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];

					if(PointerType.OpCode == SpvOpTypeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];
						const op_info& SizeInfo  = ShaderInfo[PointerType.SizeId];

						Size = SizeInfo.Constant;

						DescriptorType = GetVkSpvDescriptorType(ArrayInfo.OpCode, StorageClass);
					}
					else if(PointerType.OpCode == SpvOpTypeRuntimeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];

						DescriptorType = GetVkSpvDescriptorType(ArrayInfo.OpCode, StorageClass);
					}
					else
					{
						DescriptorType = GetVkSpvDescriptorType(PointerType.OpCode, StorageClass);
					}
				}
				else
				{
					DescriptorType = GetVkSpvDescriptorType(VariableType.OpCode, StorageClass);
				}

				VkDescriptorSetLayoutBinding& DescriptorTypeInfoUpdate = ShaderRootLayout[Var.Set][Var.Binding];
				DescriptorTypeInfoUpdate.stageFlags |= GetVKShaderStage(ShaderType);
				DescriptorTypeInfoUpdate.binding = Var.Binding;
				DescriptorTypeInfoUpdate.descriptorType = DescriptorType;
				DescriptorTypeInfoUpdate.descriptorCount = Size == ~0u ? 1 : Size;
				DescriptorTypeCounts[DescriptorType] += DescriptorTypeInfoUpdate.descriptorCount;
				CompiledShaders[Path].DescriptorTypeCounts[DescriptorType] += DescriptorTypeInfoUpdate.descriptorCount;
			}
		}

		CompiledShaders[Path].PushConstantSize = PushConstantSize;
		CompiledShaders[Path].ShaderRootLayout.insert(ShaderRootLayout.begin(), ShaderRootLayout.end());
	}

	File.close();

	return Result;
}

void vulkan_backend::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
}
