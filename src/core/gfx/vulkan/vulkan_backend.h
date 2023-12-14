#pragma once

struct vulkan_backend : public renderer_backend
{
	vulkan_backend(window* Window);
	virtual ~vulkan_backend() override = default;
	void DestroyObject() override;

	[[nodiscard]] VkShaderModule LoadShaderModule(const char* Path);
	void RecreateSwapchain(u32 NewWidth, u32 NewHeight) override;

	u32 FamilyIndex = 0;

	VkInstance Instance;
	VkPhysicalDevice PhysicalDevice;
	VkDebugReportCallbackEXT DebugCallback = 0;

	VkSurfaceKHR Surface;
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;

	VkPhysicalDeviceProperties PhysicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties MemoryProperties;

	VkSampleCountFlagBits MsaaQuality = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;

	VkDevice Device;
	VkSwapchainKHR Swapchain;
	VkSurfaceFormatKHR SurfaceFormat;
	VkDescriptorPool ImGuiPool;
};
