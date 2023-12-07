#pragma once

class window;
class renderer_backend
{
	friend shader_input;
	friend command_queue;
	friend buffer;
	friend texture;
	friend memory_heap;
	friend render_context;

	u32 FamilyIndex = 0;

	VkInstance Instance;
	VkPhysicalDevice PhysicalDevice;
	VkDebugReportCallbackEXT DebugCallback = 0;

	VkSurfaceKHR Surface;
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;

	VkPhysicalDeviceProperties PhysicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties MemoryProperties;

	VkSampleCountFlagBits MsaaQuality = VK_SAMPLE_COUNT_1_BIT;

public:
	renderer_backend(window* Window);
	void DestroyObject();

	VkShaderModule LoadShaderModule(const char* Path);
	void RecreateSwapchain(u32 NewWidth, u32 NewHeight);

	VkPipeline CreateGraphicsPipeline(VkPipelineLayout RootSignature, const std::vector<VkPipelineShaderStageCreateInfo>& Stages, const std::vector<VkFormat>& ColorAttachmentFormats, bool UseColor, bool UseDepth, bool BackFaceCull, bool UseOutline, u8 ViewMask, bool UseMultiview);
	VkPipeline CreateComputePipeline(VkPipelineLayout RootSignature, const VkPipelineShaderStageCreateInfo& ComputeShader);

	void Present();

	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;

	VkDevice Device;
	VkSwapchainKHR Swapchain;
	VkSurfaceFormatKHR SurfaceFormat;
	VkDescriptorPool ImGuiPool;

	command_queue CommandQueue;

	u32 Width;
	u32 Height;
};
