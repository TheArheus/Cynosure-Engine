#pragma once


struct vulkan_backend : public renderer_backend
{
	struct compiled_shader_info
	{
		std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> ShaderRootLayout;
		std::map<VkDescriptorType, u32> DescriptorTypeCounts;
		VkShaderModule Handle;
		u32 LocalSizeX;
		u32 LocalSizeY;
		u32 LocalSizeZ;
		u32 PushConstantSize;
		bool HavePushConstant;
	};

#if _WIN32
	vulkan_backend(HINSTANCE Inst, HWND Handle, ImGuiContext* imguiContext);
#else
	vulkan_backend(GLFWwindow* Handle);
#endif

	~vulkan_backend() override { DestroyObject(); };
	void DestroyObject() override;
	void ImGuiNewFrame() override { ImGui_ImplVulkan_NewFrame(); }

	void GetCurrentBackBufferIndex() override;
	void Wait(const std::vector<gpu_sync*>& Syncs) override;

	[[nodiscard]] VkShaderModule LoadShaderModule(const char* Path, shader_stage ShaderType, std::map<u32, std::map<u32, bool>>& IsWritable, std::map<u32, std::map<u32, image_type>>& TextureTypes, std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>>& ShaderRootLayout, std::map<VkDescriptorType, u32>& DescriptorTypeCounts, bool& HavePushConstant, u32& PushConstantSize, const std::vector<shader_define>& ShaderDefines = {}, u32* LocalSizeX = nullptr, u32* LocalSizeY = nullptr, u32* LocalSizeZ = nullptr);
	void RecreateSwapchain(u32 NewWidth, u32 NewHeight) override;

	u32 HighestUsedVulkanVersion;
	bool IsPushDescriptorsFeatureEnabled;

	VmaAllocator AllocatorHandle;

	VkInstance Instance;
	VkPhysicalDevice PhysicalDevice;
	VkDebugReportCallbackEXT DebugCallback = 0;

	VkSurfaceKHR Surface;
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;

	VkPhysicalDeviceProperties PhysicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties MemoryProperties;

	VkPhysicalDeviceFeatures2		 Features2  = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	VkPhysicalDeviceVulkan11Features Features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	VkPhysicalDeviceVulkan12Features Features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceVulkan13Features Features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

	VkPhysicalDeviceShaderAtomicFloatFeaturesEXT AtomicFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT };

	VkPhysicalDeviceProperties2 Properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	VkPhysicalDeviceConservativeRasterizationPropertiesEXT ConservativeRasterProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };
	VkPhysicalDevicePushDescriptorPropertiesKHR PushDescProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR };

	VkSampleCountFlagBits MsaaQuality = VK_SAMPLE_COUNT_1_BIT;

	std::vector<u32> FamilyIndices;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;

	std::unordered_map<std::string, compiled_shader_info> CompiledShaders;

	VkDevice Device;
	VkSwapchainKHR Swapchain;
	VkSurfaceFormatKHR SurfaceFormat;
	VkDescriptorPool ImGuiPool;
	VkRenderPass ImGuiRenderPass;

	ImGuiContext* imguiContext = nullptr;
};
