#pragma once

struct vulkan_backend : public renderer_backend
{
	struct compiled_shader_info
	{
		std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>> ShaderRootLayout;
		std::map<VkDescriptorType, u32> DescriptorTypeCounts;
		VkShaderModule Handle;
		u32 PushConstantSize;
		bool HavePushConstant;
	};

	vulkan_backend(window* Window);
	~vulkan_backend() override = default;
	void DestroyObject() override;

	[[nodiscard]] VkShaderModule LoadShaderModule(const char* Path, shader_stage ShaderType, std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>>& ShaderRootLayout, std::map<VkDescriptorType, u32>& DescriptorTypeCounts, bool& HavePushConstant, u32& PushConstantSize, const std::vector<shader_define>& ShaderDefines = {});
	void RecreateSwapchain(u32 NewWidth, u32 NewHeight) override;

	u32 HighestUsedVulkanVersion;
	u32 FamilyIndex = 0;
	bool IsPushDescriptorsFeatureEnabled;

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

	VkSampleCountFlagBits MsaaQuality = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;

	std::unordered_map<std::string, compiled_shader_info> CompiledShaders;

	VkDevice Device;
	VkSwapchainKHR Swapchain;
	VkSurfaceFormatKHR SurfaceFormat;
	VkDescriptorPool ImGuiPool;
	VkRenderPass ImGuiRenderPass;

	vulkan_command_queue* CommandQueue;
};
