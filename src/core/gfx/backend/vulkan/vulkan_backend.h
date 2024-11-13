#pragma once

class vulkan_memory_heap : public memory_heap
{
public:
	vulkan_memory_heap() = default;
	vulkan_memory_heap(renderer_backend* Backend) 
	{
		CreateResource(Backend);
	}

	void CreateResource(renderer_backend* Backend) override;

	buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, u64 DataSize, u64 Count, u32 Flags) override;
	buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, void* Data, u64 DataSize, u64 Count, u32 Flags) override;

	texture* PushTexture(renderer_backend* Backend, std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) override;
	texture* PushTexture(renderer_backend* Backend, std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) override;

	void AllocateBuffer();
	void AllocateTexture();

	VmaAllocator Handle;
};


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

	vulkan_backend(window* Window);
	~vulkan_backend() override = default;
	void DestroyObject() override;

	[[nodiscard]] VkShaderModule LoadShaderModule(const char* Path, shader_stage ShaderType, std::map<u32, std::map<u32, image_type>>& TextureTypes, std::map<u32, std::map<u32, VkDescriptorSetLayoutBinding>>& ShaderRootLayout, std::map<VkDescriptorType, u32>& DescriptorTypeCounts, bool& HavePushConstant, u32& PushConstantSize, const std::vector<shader_define>& ShaderDefines = {}, u32* LocalSizeX = nullptr, u32* LocalSizeY = nullptr, u32* LocalSizeZ = nullptr);
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

	VkPhysicalDeviceShaderAtomicFloatFeaturesEXT AtomicFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT };

	VkPhysicalDeviceProperties2 Properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	VkPhysicalDeviceConservativeRasterizationPropertiesEXT ConservativeRasterProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };

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
	texture* NullTexture1D;
	texture* NullTexture2D;
	texture* NullTexture3D;
	texture* NullTextureCube;
};
