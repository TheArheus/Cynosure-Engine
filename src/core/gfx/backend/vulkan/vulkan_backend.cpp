
#include <Volk/volk.c>

vulkan_backend::
#if _WIN32
vulkan_backend(HINSTANCE Inst, HWND Handle, ImGuiContext* _imguiContext)
#else
vulkan_backend(GLFWwindow* Handle)
#endif
{
	Type = backend_type::vulkan;

#if _WIN32
	imguiContext = _imguiContext;
	ImGui::SetCurrentContext(imguiContext);
#endif

	volkInitialize();

	VK_CHECK(vkEnumerateInstanceVersion(&HighestUsedVulkanVersion));

	VkApplicationInfo AppInfo = {};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "3D Renderer";
	AppInfo.apiVersion = HighestUsedVulkanVersion;

	std::vector<const char*> InstanceLayers;
	std::vector<const char*> RequiredInstanceLayers = 
	{
#if defined(CE_DEBUG)
		"VK_LAYER_KHRONOS_validation"
#endif
	};

	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> RequiredInstanceExtensions = 
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#if _WIN32
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif __linux__
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		"VK_EXT_debug_utils",
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

	std::cout << "Instance:" << std::endl;
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

	vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(Instance, "vkSetDebugUtilsObjectNameEXT");

	VkDebugReportCallbackCreateInfoEXT DebugReportCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
	DebugReportCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	DebugReportCreateInfo.pfnCallback = DebugReportCallback;
	VK_CHECK(vkCreateDebugReportCallbackEXT(Instance, &DebugReportCreateInfo, nullptr, &DebugCallback));

	u32 PhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
	vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices.data());

	PhysicalDevice = PickPhysicalDevice(PhysicalDevices);

	u32 QueueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertiesCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilyProperties(QueueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertiesCount, QueueFamilyProperties.data());

	u32 PrimaryQueueIdx = 0, PrimaryQueueFlags = 0;
	u32 SecondaryQueueIdx = 0, SecondaryQueueFlags = 0;
	u32 TransferQueueIdx = 0, TransferQueueFlags = 0;

	for(u32 Idx = 0; Idx < QueueFamilyProperties.size(); ++Idx)
	{
		VkQueueFamilyProperties Property = QueueFamilyProperties[Idx];
		if((Property.queueFlags & (VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT))
		{ 
			FamilyIndices.push_back(Idx); 
			PrimaryQueueIdx = Idx;
			PrimaryQueueFlags = Property.queueFlags;
		}
		if((Property.queueFlags & (VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_COMPUTE_BIT|VK_QUEUE_TRANSFER_BIT))
		{
			if(Idx != PrimaryQueueIdx) FamilyIndices.push_back(Idx);
			SecondaryQueueIdx = Idx;
			SecondaryQueueFlags = Property.queueFlags;
		}
		if((Property.queueFlags & (VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_TRANSFER_BIT))
		{
			if(Idx != SecondaryQueueIdx && Idx != PrimaryQueueIdx) FamilyIndices.push_back(Idx);
			TransferQueueIdx = Idx;
			TransferQueueFlags = Property.queueFlags;
		}
	}

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
		"VK_EXT_shader_atomic_float",
		"VK_EXT_conservative_rasterization",
		"VK_KHR_timeline_semaphore",
	};

	u32 DeviceExtensionsCount = 0;
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionsCount, nullptr);
	std::vector<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionsCount);
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionsCount, AvailableDeviceExtensions.data());

	std::cout << "Device:" << std::endl;
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

	std::vector<std::vector<float>> QueuesPriorities(QueueFamilyPropertiesCount, std::vector<float>());
	for(u32 Idx = 0; Idx < QueueFamilyPropertiesCount; ++Idx)
	{
		u32 CurrentQueueCount = QueueFamilyProperties[Idx].queueCount;
#if 1
		for(u32 QueueIdx = 0; QueueIdx < CurrentQueueCount; QueueIdx++)
		{
			QueuesPriorities[Idx].push_back(float(CurrentQueueCount - QueueIdx) / float(CurrentQueueCount));
		}
#else
		QueuesPriorities[Idx] = std::vector<float>(CurrentQueueCount, 1.0);
#endif
	}
	std::vector<VkDeviceQueueCreateInfo> DeviceQueuesCreateInfo(QueueFamilyPropertiesCount);
	for(u32 Idx = 0; Idx < QueueFamilyPropertiesCount; ++Idx)
	{
		DeviceQueuesCreateInfo[Idx].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		DeviceQueuesCreateInfo[Idx].queueFamilyIndex = Idx;
		DeviceQueuesCreateInfo[Idx].queueCount = QueueFamilyProperties[Idx].queueCount;
		DeviceQueuesCreateInfo[Idx].pQueuePriorities = QueuesPriorities[Idx].data();
	}

	VkDeviceCreateInfo DeviceCreateInfo = {};
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = DeviceQueuesCreateInfo.size();
	DeviceCreateInfo.pQueueCreateInfos = DeviceQueuesCreateInfo.data();
	DeviceCreateInfo.enabledLayerCount = (u32)DeviceLayers.size();
	DeviceCreateInfo.ppEnabledLayerNames = DeviceLayers.data();
	DeviceCreateInfo.enabledExtensionCount = (u32)DeviceExtensions.size();
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	DeviceCreateInfo.pNext = &Features2;
	Features2 .pNext = &Features11;
	Features11.pNext = &Features12;
	Features12.pNext = &Features13;
	Features13.pNext = &AtomicFeatures;

	vkGetPhysicalDeviceFeatures2(PhysicalDevice, &Features2);
	VK_CHECK(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device), true);

	Properties2.pNext = &ConservativeRasterProps;
	ConservativeRasterProps.pNext = &PushDescProps;
	vkGetPhysicalDeviceProperties2(PhysicalDevice, &Properties2);

#if _WIN32
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	SurfaceCreateInfo.hinstance = Inst;
	SurfaceCreateInfo.hwnd = Handle;
	VK_CHECK(vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, 0, &Surface));
#elif defined(__linux__)
	VkXlibSurfaceCreateInfoKHR SurfaceCreateInfo = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
	SurfaceCreateInfo.dpy = glfwGetX11Display();
	SurfaceCreateInfo.window = glfwGetX11Window(Handle);
	VK_CHECK(vkCreateXlibSurfaceKHR(Instance, &SurfaceCreateInfo, nullptr, &Surface))
#endif

	SurfaceFormat = GetSwapchainFormat(PhysicalDevice, Surface);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);

	VkSurfaceCapabilitiesKHR ResizeCaps;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &ResizeCaps));
	Width = ResizeCaps.currentExtent.width;
	Height = ResizeCaps.currentExtent.height;
	if(ImageCount < ResizeCaps.minImageCount) ImageCount = ResizeCaps.minImageCount;

	VkCompositeAlphaFlagBitsKHR CompositeAlpha = 
		(SurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR :
		(SurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :
		(SurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

	VkSwapchainCreateInfoKHR SwapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	SwapchainCreateInfo.surface = Surface;
	SwapchainCreateInfo.minImageCount = ImageCount;
	SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
	SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapchainCreateInfo.imageExtent.width = Width;
	SwapchainCreateInfo.imageExtent.height = Height;
	SwapchainCreateInfo.imageArrayLayers = 1;
	SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	SwapchainCreateInfo.queueFamilyIndexCount = FamilyIndices.size();
	SwapchainCreateInfo.pQueueFamilyIndices = FamilyIndices.data();
	SwapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	SwapchainCreateInfo.compositeAlpha = CompositeAlpha;
	SwapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; //VK_PRESENT_MODE_FIFO_KHR;
	VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, 0, &Swapchain));

	u32 SwapchainImageCount = 0;
	vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, 0);
	SwapchainImages.resize(SwapchainImageCount);
	vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, SwapchainImages.data());

	PrimaryQueue = new vulkan_command_queue(this, PrimaryQueueIdx, PrimaryQueueFlags);
	for(u32 QueueIdx = 0; QueueIdx < QueueFamilyProperties[SecondaryQueueIdx].queueCount; ++QueueIdx)
	{
		SecondaryQueues.push_back(new vulkan_command_queue(this, SecondaryQueueIdx, SecondaryQueueFlags, QueueIdx));
	}

	{
		VmaVulkanFunctions VulkanFunctions = {};
		VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		VulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo AllocatorInfo = {};
		AllocatorInfo.vulkanApiVersion = HighestUsedVulkanVersion;
		AllocatorInfo.instance = Instance;
		AllocatorInfo.physicalDevice = PhysicalDevice;
		AllocatorInfo.device = Device;
		AllocatorInfo.pVulkanFunctions = &VulkanFunctions;
		vmaCreateAllocator(&AllocatorInfo, &AllocatorHandle);
	}

#if 0
	UpdateBuffer = new vulkan_buffer(this, "Update buffer", nullptr, MiB(64), 1, RF_CpuWrite);
	UploadBuffer = new vulkan_buffer(this, "Upload buffer", nullptr, MiB(64), 1, RF_CpuRead);
#endif

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
	InitInfo.QueueFamily = PrimaryQueueIdx;
	InitInfo.Queue = static_cast<vulkan_command_queue*>(PrimaryQueue)->Handle;
	InitInfo.MinImageCount = ImageCount;
	InitInfo.ImageCount = SwapchainImages.size();
	InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT; //MsaaQuality;
	InitInfo.UseDynamicRendering = Features13.dynamicRendering;
	InitInfo.ColorAttachmentFormat = SurfaceFormat.format;

	if(!Features13.dynamicRendering)
	{
        VkAttachmentDescription attachment = {};
        attachment.format = SurfaceFormat.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;

        vkCreateRenderPass(Device, &info, nullptr, &ImGuiRenderPass);

		ImGui_ImplVulkan_Init(&InitInfo, ImGuiRenderPass);
	}
	else
		ImGui_ImplVulkan_Init(&InitInfo, VK_NULL_HANDLE);

	ImGui_ImplVulkan_CreateFontsTexture();
}

void vulkan_backend::
DestroyObject()
{
	for(auto& Shader : CompiledShaders)
	{
		if(Shader.second.Handle)
		{
			vkDestroyShaderModule(Device, Shader.second.Handle, nullptr);
			Shader.second.Handle = VK_NULL_HANDLE;
		}
	}

#if _WIN32
	ImGui::SetCurrentContext(imguiContext);
#endif
	ImGui_ImplVulkan_Shutdown();
	imguiContext = nullptr;

    if (!Features13.dynamicRendering && ImGuiRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(Device, ImGuiRenderPass, nullptr);
        ImGuiRenderPass = VK_NULL_HANDLE;
    }

    if (ImGuiPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(Device, ImGuiPool, nullptr);
        ImGuiPool = VK_NULL_HANDLE;
    }

#if 0
	delete UpdateBuffer;
	UpdateBuffer = nullptr;
	delete UploadBuffer;
	UploadBuffer = nullptr;
#endif

	vmaDestroyAllocator(AllocatorHandle);

	for(command_queue* Queue : SecondaryQueues) delete Queue;
	SecondaryQueues.clear();

	delete PrimaryQueue;

	for (VkImageView ImageView : SwapchainImageViews)
	{
		if (ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(Device, ImageView, nullptr);
		}
	}

	vkDestroySwapchainKHR(Device, Swapchain, nullptr);
	vkDestroyDevice(Device, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDebugReportCallbackEXT(Instance, DebugCallback, nullptr);
	vkDestroyInstance(Instance, nullptr);
}

void vulkan_backend::
GetCurrentBackBufferIndex()
{
	vkAcquireNextImageKHR(Device, Swapchain, ~0ull, static_cast<vulkan_command_queue*>(PrimaryQueue)->AcquireSemaphore, VK_NULL_HANDLE, &BackBufferIndex);
}

void vulkan_backend::
Wait(const std::vector<gpu_sync*>& Syncs)
{
	if(!Syncs.size()) return;
	std::vector<u64> Values;
	std::vector<VkSemaphore> Semaphores;
	for(gpu_sync* Sync : Syncs)
	{
		u64 SignaledValue = 0;
		vkGetSemaphoreCounterValue(Device, static_cast<vulkan_gpu_sync*>(Sync)->Handle, &SignaledValue);
		if(SignaledValue < Sync->CurrentWaitValue)
		{
			Values.push_back(Sync->CurrentWaitValue);
			Semaphores.push_back(static_cast<vulkan_gpu_sync*>(Sync)->Handle);
		}
	}

	if(!Semaphores.size()) return;
	VkSemaphoreWaitInfo WaitInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
	//WaitInfo.flags = VK_SEMAPHORE_WAIT_ANY_BIT;
	WaitInfo.semaphoreCount = Semaphores.size();
	WaitInfo.pSemaphores = Semaphores.data();
	WaitInfo.pValues = Values.data();
	VK_CHECK(vkWaitSemaphoresKHR(Device, &WaitInfo, UINT64_MAX));
}

void vulkan_backend::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
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

bool GlslCompile(std::vector<u32>& SpirvOut, const std::string& Name, const std::string& ShaderCode, shader_stage ShaderType, u32 HighestUsedVulkanVersion)
{
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
		std::cerr << "Shader: " << std::string(Name) << std::endl;
		std::cerr << ShaderModule.getInfoLog() << std::endl;
		std::cerr << ShaderModule.getInfoDebugLog() << std::endl;
		return true;
	}

	glslang::TProgram Program;
	Program.addShader(&ShaderModule);

	if (!Program.link(static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules)))
	{
		std::cerr << "Shader: " << std::string(Name) << std::endl;
		std::cerr << Program.getInfoLog() << std::endl;
		std::cerr << Program.getInfoDebugLog() << std::endl;
		return true;
	}

	glslang::SpvOptions CompileOptions;
#ifdef CE_DEBUG
	CompileOptions.generateDebugInfo = true;
	CompileOptions.stripDebugInfo = false;
#endif
	glslang::TIntermediate *Intermediate = Program.getIntermediate(ShaderModule.getStage());
	glslang::GlslangToSpv(*Intermediate, SpirvOut, &CompileOptions);

	glslang::FinalizeProcess();

	return false;
}

// TODO: Implement a better shader compilation(SIMPLIFY IT!)
// TODO: Maybe handle OpTypeRuntimeArray in the future if it will be possible
VkShaderModule vulkan_backend::
LoadShaderModule(const char* Path, shader_stage ShaderType, std::map<u32, std::map<u32, bool>>& IsWritable, std::map<u32, std::map<u32, image_type>>& TextureTypes, std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>>& ShaderRootLayout, std::map<VkDescriptorType, u32>& DescriptorTypeCounts, bool& HavePushConstant, u32& PushConstantSize, const std::vector<shader_define>& ShaderDefines, u32* LocalSizeX, u32* LocalSizeY, u32* LocalSizeZ)
{
	auto FoundCompiledShader = CompiledShaders.find(Path);
	if(FoundCompiledShader != CompiledShaders.end())
	{
		ShaderRootLayout.insert(FoundCompiledShader->second.ShaderRootLayout.begin(), FoundCompiledShader->second.ShaderRootLayout.end());
		DescriptorTypeCounts.insert(FoundCompiledShader->second.DescriptorTypeCounts.begin(), FoundCompiledShader->second.DescriptorTypeCounts.end());
		HavePushConstant = FoundCompiledShader->second.HavePushConstant;
		PushConstantSize = FoundCompiledShader->second.PushConstantSize;
		LocalSizeX ? *LocalSizeX = FoundCompiledShader->second.LocalSizeX : 0;
		LocalSizeY ? *LocalSizeY = FoundCompiledShader->second.LocalSizeY : 0;
		LocalSizeZ ? *LocalSizeZ = FoundCompiledShader->second.LocalSizeZ : 0;
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

		if(GlslCompile(SpirvCode, Path, ShaderCode, ShaderType, HighestUsedVulkanVersion))
			return VK_NULL_HANDLE;

		u32 LocalSizeIdX;
		u32 LocalSizeIdY;
		u32 LocalSizeIdZ;
		std::vector<op_info> ShaderInfo;
		std::set<u32> DescriptorIndices;
		ParseSpirv(SpirvCode, ShaderInfo, DescriptorIndices, &LocalSizeIdX, &LocalSizeIdY, &LocalSizeIdZ, nullptr, nullptr, nullptr);

		LocalSizeX ? *LocalSizeX = ShaderInfo[LocalSizeIdX].Constant : 0;
		LocalSizeY ? *LocalSizeY = ShaderInfo[LocalSizeIdY].Constant : 0;
		LocalSizeZ ? *LocalSizeZ = ShaderInfo[LocalSizeIdZ].Constant : 0;
		CompiledShaders[Path].LocalSizeX = LocalSizeX ? *LocalSizeX : 0;
		CompiledShaders[Path].LocalSizeY = LocalSizeY ? *LocalSizeY : 0;
		CompiledShaders[Path].LocalSizeZ = LocalSizeZ ? *LocalSizeZ : 0;

		VkShaderModuleCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		CreateInfo.codeSize = SpirvCode.size() * sizeof(u32);
		CreateInfo.pCode = SpirvCode.data();

		VK_CHECK(vkCreateShaderModule(Device, &CreateInfo, 0, &Result));
		CompiledShaders[Path].Handle = Result;

		for(u32 VariableIdx = 0; VariableIdx < ShaderInfo.size(); VariableIdx++)
		{
			const op_info& Var = ShaderInfo[VariableIdx];
			if (Var.OpCode == SpvOpVariable && Var.StorageClass == SpvStorageClassPushConstant)
			{
				const op_info& VariableType = ShaderInfo[Var.TypeId[0]];
				const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];

				CompiledShaders[Path].HavePushConstant = true;
				HavePushConstant = true;

				PushConstantSize += GetSpvVariableSize(ShaderInfo, PointerType);
			}
			else if (Var.OpCode == SpvOpVariable && Var.IsDescriptor)
			{
				const op_info& VariableType = ShaderInfo[Var.TypeId[0]];

				u32 Size = ~0u;
				u32 StorageClass = Var.StorageClass;
				bool NonWritable = false;
				image_type TextureType = image_type::unknown;
				VkDescriptorType DescriptorType;
				if(VariableType.OpCode == SpvOpTypePointer)
				{
					const op_info& PointerType = ShaderInfo[VariableType.TypeId[0]];

					if(PointerType.OpCode == SpvOpTypeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];
						const op_info& SizeInfo  = ShaderInfo[PointerType.SizeId];

						Size = SizeInfo.Constant;
						NonWritable = ArrayInfo.NonWritable;

						DescriptorType = GetVkSpvDescriptorType(ArrayInfo.OpCode, StorageClass);

						if (ArrayInfo.OpCode == SpvOpTypeImage)
						{
							switch(ArrayInfo.Dimensionality)
							{
								case SpvDim1D: TextureType = image_type::Texture1D; break;
								case SpvDim2D: TextureType = image_type::Texture2D; break;
								case SpvDim3D: TextureType = image_type::Texture3D; break;
								case SpvDimCube: TextureType = image_type::TextureCube; break;
							}
						}
						else if(ArrayInfo.OpCode == SpvOpTypeSampledImage)
						{
							const op_info& ImageType = ShaderInfo[ArrayInfo.TypeId[0]];

							switch(ImageType.Dimensionality)
							{
								case SpvDim1D: TextureType = image_type::Texture1D; break;
								case SpvDim2D: TextureType = image_type::Texture2D; break;
								case SpvDim3D: TextureType = image_type::Texture3D; break;
								case SpvDimCube: TextureType = image_type::TextureCube; break;
							}
						}
					}
					else if(PointerType.OpCode == SpvOpTypeRuntimeArray)
					{
						const op_info& ArrayInfo = ShaderInfo[PointerType.TypeId[0]];

						NonWritable = ArrayInfo.NonWritable;

						DescriptorType = GetVkSpvDescriptorType(ArrayInfo.OpCode, StorageClass);

						if (ArrayInfo.OpCode == SpvOpTypeImage)
						{
							switch(ArrayInfo.Dimensionality)
							{
								case SpvDim1D: TextureType = image_type::Texture1D; break;
								case SpvDim2D: TextureType = image_type::Texture2D; break;
								case SpvDim3D: TextureType = image_type::Texture3D; break;
								case SpvDimCube: TextureType = image_type::TextureCube; break;
							}
						}
						else if(ArrayInfo.OpCode == SpvOpTypeSampledImage)
						{
							const op_info& ImageType = ShaderInfo[ArrayInfo.TypeId[0]];

							switch(ImageType.Dimensionality)
							{
								case SpvDim1D: TextureType = image_type::Texture1D; break;
								case SpvDim2D: TextureType = image_type::Texture2D; break;
								case SpvDim3D: TextureType = image_type::Texture3D; break;
								case SpvDimCube: TextureType = image_type::TextureCube; break;
							}
						}
					}
					else
					{
						NonWritable = PointerType.NonWritable;
						DescriptorType = GetVkSpvDescriptorType(PointerType.OpCode, StorageClass);
					}

					if (PointerType.OpCode == SpvOpTypeImage)
					{
						switch(PointerType.Dimensionality)
						{
							case SpvDim1D: TextureType = image_type::Texture1D; break;
							case SpvDim2D: TextureType = image_type::Texture2D; break;
							case SpvDim3D: TextureType = image_type::Texture3D; break;
							case SpvDimCube: TextureType = image_type::TextureCube; break;
						}
					}
					else if(PointerType.OpCode == SpvOpTypeSampledImage)
					{
						const op_info& ImageType = ShaderInfo[PointerType.TypeId[0]];

						switch(ImageType.Dimensionality)
						{
							case SpvDim1D: TextureType = image_type::Texture1D; break;
							case SpvDim2D: TextureType = image_type::Texture2D; break;
							case SpvDim3D: TextureType = image_type::Texture3D; break;
							case SpvDimCube: TextureType = image_type::TextureCube; break;
						}
					}
				}
				else
				{
					NonWritable = VariableType.NonWritable;
					DescriptorType = GetVkSpvDescriptorType(VariableType.OpCode, StorageClass);
				}

				TextureTypes[Var.Set][Var.Binding] = TextureType;
				IsWritable[Var.Set][Var.Binding] = !NonWritable;
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
