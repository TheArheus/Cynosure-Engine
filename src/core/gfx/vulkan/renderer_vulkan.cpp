
renderer_backend::
renderer_backend(window* Window)
{
	volkInitialize();

	VkApplicationInfo AppInfo = {};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "3D Renderer";
	AppInfo.apiVersion = VK_API_VERSION_1_3;

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

#if _DEBUG
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
#endif

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

	float QueuePriorities[] = {1.0f};
	VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {};
	DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	DeviceQueueCreateInfo.queueFamilyIndex = FamilyIndex;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;

	VkPhysicalDeviceFeatures2 Features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	Features2.features.imageCubeArray = true;
	Features2.features.multiDrawIndirect = true;
	Features2.features.pipelineStatisticsQuery = true;
	Features2.features.shaderInt16 = true;
	Features2.features.shaderInt64 = true;

	VkPhysicalDeviceVulkan11Features Features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	Features11.storageBuffer16BitAccess = true;
	Features11.multiview = true;

	VkPhysicalDeviceVulkan12Features Features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	Features12.drawIndirectCount = true;
	Features12.shaderFloat16 = true;
	Features12.shaderInt8 = true;
	Features12.samplerFilterMinmax = true;
	Features12.descriptorIndexing = true;
	Features12.descriptorBindingPartiallyBound = true;

	VkPhysicalDeviceVulkan13Features Features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	Features13.maintenance4 = true;
	Features13.dynamicRendering = true;

	VkDeviceCreateInfo DeviceCreateInfo = {};
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;
	DeviceCreateInfo.enabledLayerCount = (u32)DeviceLayers.size();
	DeviceCreateInfo.ppEnabledLayerNames = DeviceLayers.data();
	DeviceCreateInfo.enabledExtensionCount = (u32)DeviceExtensions.size();
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	DeviceCreateInfo.pNext = &Features2;
	Features2.pNext = &Features11;
	Features11.pNext = &Features12;
	Features12.pNext = &Features13;

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

	CommandQueue.Init(this);

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
	InitInfo.Queue = CommandQueue.Handle;
	InitInfo.MinImageCount = 2;
	InitInfo.ImageCount = SwapchainImageCount;
	InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT; //MsaaQuality;
	InitInfo.UseDynamicRendering = true;
	InitInfo.ColorAttachmentFormat = SurfaceFormat.format;
	ImGui_ImplVulkan_Init(&InitInfo, VK_NULL_HANDLE);

	ImGui_ImplVulkan_CreateFontsTexture();
}

void renderer_backend::
DestroyObject()
{
	vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
	vkDestroySwapchainKHR(Device, Swapchain, nullptr);
	CommandQueue.DestroyObject();
	vkDestroyDevice(Device, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDebugReportCallbackEXT(Instance, DebugCallback, nullptr);
	vkDestroyInstance(Instance, nullptr);
}

VkShaderModule renderer_backend::
LoadShaderModule(const char* Path)
{
	VkShaderModule Result = 0;
	FILE* File = fopen(Path, "rb");
	if(File)
	{
		fseek(File, 0, SEEK_END);
		long FileLength = ftell(File);
		fseek(File, 0, SEEK_SET);

		char* Buffer = (char*)malloc(FileLength);
		assert(Buffer);

		size_t ReadSize = fread(Buffer, 1, FileLength, File);
		assert(ReadSize == size_t(FileLength));
		assert(FileLength % 4 == 0);

		VkShaderModuleCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		CreateInfo.codeSize = FileLength;
		CreateInfo.pCode = reinterpret_cast<const u32*>(Buffer);

		VK_CHECK(vkCreateShaderModule(Device, &CreateInfo, 0, &Result));

		fclose(File);
	}
	return Result;
}

VkPipeline renderer_backend::
CreateGraphicsPipeline(VkPipelineLayout RootSignature, const std::vector<VkPipelineShaderStageCreateInfo>& Stages, const std::vector<VkFormat>& ColorAttachmentFormats, bool UseColor, bool UseDepth, bool BackFaceCull, bool UseOutline, u8 ViewMask, bool UseMultiview)
{
	VkGraphicsPipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

	CreateInfo.layout = RootSignature;
	CreateInfo.pStages = Stages.data();
	CreateInfo.stageCount = Stages.size();

	VkPipelineRenderingCreateInfoKHR PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	PipelineRenderingCreateInfo.colorAttachmentCount    = ColorAttachmentFormats.size();
	PipelineRenderingCreateInfo.pColorAttachmentFormats = UseColor ? ColorAttachmentFormats.data() : nullptr;
	PipelineRenderingCreateInfo.depthAttachmentFormat   = UseDepth ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_UNDEFINED;
	PipelineRenderingCreateInfo.viewMask = UseMultiview * ViewMask;

	VkPipelineVertexInputStateCreateInfo VertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	InputAssemblyState.topology = UseOutline ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	std::vector<VkPipelineColorBlendAttachmentState> ColorAttachmentState(ColorAttachmentFormats.size());
	for(u32 Idx = 0; Idx < ColorAttachmentFormats.size(); ++Idx)
	{
		ColorAttachmentState[Idx].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
#if DEBUG_COLOR_BLEND
		ColorAttachmentState[Idx].blendEnable = true;
		ColorAttachmentState[Idx].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorAttachmentState[Idx].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
#endif
	}

	VkPipelineColorBlendStateCreateInfo ColorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	ColorBlendState.pAttachments = ColorAttachmentState.data();
	ColorBlendState.attachmentCount = ColorAttachmentState.size();

	VkPipelineDepthStencilStateCreateInfo DepthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	DepthStencilState.depthTestEnable = true;
	DepthStencilState.depthWriteEnable = true;
	DepthStencilState.minDepthBounds = 0.0f;
	DepthStencilState.maxDepthBounds = 1.0f;
	DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	ViewportState.viewportCount = 1;
	ViewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo RasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	RasterizationState.lineWidth = 1.0f;
	RasterizationState.cullMode  = BackFaceCull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT;
	RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineDynamicStateCreateInfo DynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	VkDynamicState DynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	DynamicState.pDynamicStates = DynamicStates;
	DynamicState.dynamicStateCount = 2;

	VkPipelineMultisampleStateCreateInfo MultisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //MsaaQuality;

	VkPipelineTessellationStateCreateInfo TessellationState = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

	CreateInfo.pColorBlendState = &ColorBlendState;
	CreateInfo.pDepthStencilState = &DepthStencilState;
	CreateInfo.pDynamicState = &DynamicState;
	CreateInfo.pInputAssemblyState = &InputAssemblyState;
	CreateInfo.pMultisampleState = &MultisampleState;
	CreateInfo.pRasterizationState = &RasterizationState;
	CreateInfo.pTessellationState = &TessellationState;
	CreateInfo.pVertexInputState = &VertexInputState;
	CreateInfo.pViewportState = &ViewportState;
	CreateInfo.pNext = &PipelineRenderingCreateInfo;

	VkPipeline Result = 0;
	VK_CHECK(vkCreateGraphicsPipelines(Device, nullptr, 1, &CreateInfo, 0, &Result));

	return Result;
}

VkPipeline renderer_backend::
CreateComputePipeline(VkPipelineLayout RootSignature, const VkPipelineShaderStageCreateInfo& ComputeShader)
{
	VkComputePipelineCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    CreateInfo.stage  = ComputeShader;
    CreateInfo.layout = RootSignature;

	VkPipeline Result = 0;
	VK_CHECK(vkCreateComputePipelines(Device, nullptr, 1, &CreateInfo, 0, &Result));
	return Result;
}
