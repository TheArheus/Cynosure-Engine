
vulkan_backend::
vulkan_backend(window* Window)
{
	volkInitialize();

	VK_CHECK(vkEnumerateInstanceVersion(&HighestUsedVulkanVersion));

	VkApplicationInfo AppInfo = {};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "3D Renderer";
	AppInfo.apiVersion = HighestUsedVulkanVersion;

	std::vector<const char*> Layers = 
	{
#if _DEBUG
		"VK_LAYER_KHRONOS_validation"
#endif
	};
	std::vector<const char*> Extensions = 
	{
#if _WIN32
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	};

#if __linux__
	u32 RequiredInstanceExtensionCount;
	const char** RequiredExtensions = glfwGetRequiredInstanceExtensions(&RequiredInstanceExtensionCount);
	for(u32 ExtIdx = 0; ExtIdx < RequiredInstanceExtensionCount; ++ExtIdx)
	{
		Extensions.push_back(RequiredExtensions[ExtIdx]);
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

	for(const char* Required : Layers)
	{
		bool Found = false;
		for(const auto& Available : AvailableInstanceLayers)
		{
			if(strcmp(Required, Available.layerName) == 0)
			{
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

	for(const char* Required : Extensions)
	{
		bool Found = false;
		for(const auto& Available : AvailableInstanceExtensions)
		{
			if(strcmp(Required, Available.extensionName) == 0)
			{
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
	InstanceCreateInfo.enabledLayerCount = (u32)Layers.size();
	InstanceCreateInfo.ppEnabledLayerNames = Layers.data();
	InstanceCreateInfo.enabledExtensionCount = (u32)Extensions.size();
	InstanceCreateInfo.ppEnabledExtensionNames = Extensions.data();

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

	std::vector<const char*> DeviceLayers = 
	{
	};

	std::vector<const char*> DeviceExtensions = 
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

	for(const char* Required : DeviceExtensions)
	{
		bool Found = false;
		for(const VkExtensionProperties& Available : AvailableDeviceExtensions)
		{
			if(strcmp(Required, Available.extensionName) == 0)
			{
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
	vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device);

#if _WIN32
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	SurfaceCreateInfo.hinstance = Window->WindowClass.Inst;
	SurfaceCreateInfo.hwnd = Window->Handle;
	vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, 0, &Surface);
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

VkShaderModule vulkan_backend::
LoadShaderModule(const char* Path, shader_stage ShaderType, const std::vector<shader_define>& ShaderDefines)
{
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

		glslang_input_t Input = {};
		Input.language = GLSLANG_SOURCE_GLSL;
		Input.client = GLSLANG_CLIENT_VULKAN;
		Input.target_language = GLSLANG_TARGET_SPV;
		Input.target_language_version = GLSLANG_TARGET_SPV_1_4;
		Input.code = ShaderCode.c_str();
		Input.default_version = 450;
		Input.default_profile = GLSLANG_NO_PROFILE;
		Input.force_default_version_and_profile = false;
		Input.forward_compatible = false;
		Input.messages = GLSLANG_MSG_DEFAULT_BIT;
		Input.resource = &DefaultBuiltInResource;

		switch (ShaderType)
		{
			case shader_stage::vertex:
				Input.stage = glslang_stage_t::GLSLANG_STAGE_VERTEX;
				break;
			case shader_stage::tessellation_control:
				Input.stage = glslang_stage_t::GLSLANG_STAGE_TESSCONTROL;
				break;
			case shader_stage::tessellation_eval:
				Input.stage = glslang_stage_t::GLSLANG_STAGE_TESSEVALUATION;
				break;
			case shader_stage::geometry:
				Input.stage = glslang_stage_t::GLSLANG_STAGE_GEOMETRY;
				break;
			case shader_stage::fragment:
				Input.stage = glslang_stage_t::GLSLANG_STAGE_FRAGMENT;
				break;
			case shader_stage::compute:
				Input.stage = glslang_stage_t::GLSLANG_STAGE_COMPUTE;
				break;
		}

		if(HighestUsedVulkanVersion & VK_API_VERSION_1_3)
		{
			Input.client_version = GLSLANG_TARGET_VULKAN_1_3;
		}
		else if(HighestUsedVulkanVersion & VK_API_VERSION_1_2)
		{
			Input.client_version = GLSLANG_TARGET_VULKAN_1_2;
		}
		else if(HighestUsedVulkanVersion & VK_API_VERSION_1_1)
		{
			Input.client_version = GLSLANG_TARGET_VULKAN_1_1;
		}

		glslang_initialize_process();

		glslang_shader_t* ShaderModule = glslang_shader_create( &Input );

		if (!glslang_shader_preprocess(ShaderModule, &Input))
		{
			std::cerr << std::string(glslang_shader_get_info_log(ShaderModule)) << std::endl;
			std::cerr << std::string(glslang_shader_get_info_debug_log(ShaderModule)) << std::endl;
			return VK_NULL_HANDLE;
		}

		if (!glslang_shader_parse(ShaderModule, &Input))
		{
			std::cerr << std::string(glslang_shader_get_info_log(ShaderModule)) << std::endl;
			std::cerr << std::string(glslang_shader_get_info_debug_log(ShaderModule)) << std::endl;
			return VK_NULL_HANDLE;
		}

		glslang_program_t* Program = glslang_program_create();
		glslang_program_add_shader(Program, ShaderModule);

		if (!glslang_program_link(Program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
		{
			std::cerr << std::string(glslang_shader_get_info_log(ShaderModule)) << std::endl;
			std::cerr << std::string(glslang_shader_get_info_debug_log(ShaderModule)) << std::endl;
			return VK_NULL_HANDLE;
		}

		glslang_program_SPIRV_generate(Program, Input.stage);

		if (glslang_program_SPIRV_get_messages(Program))
		{
			printf("%s", glslang_program_SPIRV_get_messages(Program));
		}

		glslang_shader_delete(ShaderModule);

		glslang_finalize_process();

		const size_t SpirvSize = glslang_program_SPIRV_get_size(Program);
		SpirvCode.resize(SpirvSize);
		glslang_program_SPIRV_get(Program, SpirvCode.data());

		VkShaderModuleCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		CreateInfo.codeSize = SpirvCode.size() * sizeof(u32);
		CreateInfo.pCode = SpirvCode.data();

		VK_CHECK(vkCreateShaderModule(Device, &CreateInfo, 0, &Result));
	}

	File.close();

	return Result;
}

void vulkan_backend::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
}
