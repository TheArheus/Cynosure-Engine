#pragma once

// TODO: use vkAllocateDescriptorSets and vkCmdBindDescriptorSets and vkUpdateDescriptorSets
// TODO: use make it so I could create both push descriptors sets and ordinary descriptor sets
class vulkan_shader_input : public shader_input
{
	std::vector<VkDescriptorSetLayout> Layouts;
	std::map<VkDescriptorType, u32> DescriptorTypeCounts;
	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;
	std::map<u32, std::vector<VkDescriptorBindingFlagsEXT>>  ParametersFlags;
	std::map<u32, u32> SetIndices;
	u32 GlobalOffset = 0;
	u32 PushDescriptorsCount = 0;
	VkDevice Device;
	VkDescriptorPool Pool;

	vulkan_shader_input(const vulkan_shader_input&) = delete;
	vulkan_shader_input& operator=(const vulkan_shader_input&) = delete;

public:
	vulkan_shader_input() = default;
	~vulkan_shader_input() override
	{
		vkDestroyPipelineLayout(Device, Handle, nullptr);
	}

	vulkan_shader_input(vulkan_shader_input&& other) noexcept :
		Parameters(std::move(other.Parameters)),
		SetIndices(std::move(other.SetIndices)),
		Layouts(std::move(other.Layouts)),
		GlobalOffset(std::move(other.GlobalOffset)),
		Device(std::move(other.Device)),
		Handle(other.Handle),  // Add this line
		IsSetPush(std::move(other.IsSetPush)),
		Sets(std::move(other.Sets)),
		PushConstants(std::move(other.PushConstants))
	{}

	vulkan_shader_input& operator=(vulkan_shader_input&& other) noexcept
	{
		if (this != &other)
		{
			std::swap(Parameters, other.Parameters);
			std::swap(SetIndices, other.SetIndices);
			std::swap(Layouts, other.Layouts);
			std::swap(GlobalOffset, other.GlobalOffset);
			std::swap(Device, other.Device);
			std::swap(Handle, other.Handle);
			std::swap(IsSetPush, other.IsSetPush);
			std::swap(Sets, other.Sets);
			std::swap(PushConstants, other.PushConstants);
		}
		return *this;
	}

	vulkan_shader_input* PushStorageBuffer(u32 Count = 1, u32 Space = 0, bool IsPartiallyBound = false, shader_stage Stage = shader_stage::all) override
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = GetVKShaderStage(Stage);
		Parameter.binding = SetIndices[Space];
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);
		DescriptorTypeCounts[Parameter.descriptorType] += Count;
		ParametersFlags[Space].push_back(IsPartiallyBound * VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);
		SetIndices[Space] += 1;

		return this;
	}

	vulkan_shader_input* PushUniformBuffer(u32 Count = 1, u32 Space = 0, bool IsPartiallyBound = false, shader_stage Stage = shader_stage::all) override
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = GetVKShaderStage(Stage);
		Parameter.binding = SetIndices[Space];
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);
		DescriptorTypeCounts[Parameter.descriptorType] += Count;
		ParametersFlags[Space].push_back(IsPartiallyBound * VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);
		SetIndices[Space] += 1;

		return this;
	}

	vulkan_shader_input* PushSampler(u32 Count = 1, u32 Space = 0, bool IsPartiallyBound = false, shader_stage Stage = shader_stage::all) override
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = GetVKShaderStage(Stage);
		Parameter.binding = SetIndices[Space];
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);
		DescriptorTypeCounts[Parameter.descriptorType] += Count;
		ParametersFlags[Space].push_back(IsPartiallyBound * VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);
		SetIndices[Space] += 1;

		return this;
	}

	vulkan_shader_input* PushSampledImage(u32 Count = 1, u32 Space = 0, bool IsPartiallyBound = false, shader_stage Stage = shader_stage::all) override
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = GetVKShaderStage(Stage);
		Parameter.binding = SetIndices[Space];
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);
		DescriptorTypeCounts[Parameter.descriptorType] += Count;
		ParametersFlags[Space].push_back(IsPartiallyBound * VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);
		SetIndices[Space] += 1;

		return this;
	}

	vulkan_shader_input* PushStorageImage(u32 Count = 1, u32 Space = 0, bool IsPartiallyBound = false, shader_stage Stage = shader_stage::all) override
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = GetVKShaderStage(Stage);
		Parameter.binding = SetIndices[Space];
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);
		DescriptorTypeCounts[Parameter.descriptorType] += Count;
		ParametersFlags[Space].push_back(IsPartiallyBound * VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);
		SetIndices[Space] += 1;

		return this;
	}

	vulkan_shader_input* PushImageSampler(u32 Count = 1, u32 Space = 0, bool IsPartiallyBound = false, shader_stage Stage = shader_stage::all) override
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = GetVKShaderStage(Stage);
		Parameter.binding = SetIndices[Space];
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);
		DescriptorTypeCounts[Parameter.descriptorType] += Count;
		ParametersFlags[Space].push_back(IsPartiallyBound * VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT);
		SetIndices[Space] += 1;

		return this;
	}

	vulkan_shader_input* PushConstant(u32 Size, shader_stage Stage = shader_stage::all) override
	{
		VkPushConstantRange ConstantRange = {};
		ConstantRange.stageFlags = GetVKShaderStage(Stage);
		ConstantRange.offset = GlobalOffset;
		ConstantRange.size   = Size;
		PushConstants.push_back(ConstantRange);
		GlobalOffset += Size;

		return this;
	}

	vulkan_shader_input* Update(renderer_backend* Backend, u32 Space = 0, bool IsPush = false) override
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		Device = Gfx->Device;
		vkDestroyDescriptorSetLayout(Device, Layouts[Space], nullptr);

		IsPush = (Gfx->IsPushDescriptorsFeatureEnabled & IsPush);

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
		DescriptorSetLayoutCreateInfo.flags = IsPush * VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		DescriptorSetLayoutCreateInfo.bindingCount = Parameters[Space].size();
		DescriptorSetLayoutCreateInfo.pBindings = Parameters[Space].data();

		VkDescriptorSetLayout DescriptorSetLayout;
		VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
		Layouts[Space] = DescriptorSetLayout;

		return this;
	}

	void UpdateAll(renderer_backend* Backend) override
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		Device = Gfx->Device;

		if (Handle != VK_NULL_HANDLE) 
		{
			vkDestroyPipelineLayout(Device, Handle, nullptr);
			Handle = VK_NULL_HANDLE;
		}

		VkPipelineLayoutCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		CreateInfo.pSetLayouts = Layouts.data();
		CreateInfo.setLayoutCount = Layouts.size();
		CreateInfo.pushConstantRangeCount = PushConstants.size();
		CreateInfo.pPushConstantRanges = PushConstants.data();

		VK_CHECK(vkCreatePipelineLayout(Device, &CreateInfo, nullptr, &Handle));
	}

	vulkan_shader_input* Build(renderer_backend* Backend, u32 Space = 0, bool IsPush = false) override
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		Device = Gfx->Device;

		IsPush = (Gfx->IsPushDescriptorsFeatureEnabled & IsPush);

		if(IsPush)
		{
			PushDescriptorsCount++;
			PushDescriptorSetIdx = Space;
		}
		assert(PushDescriptorsCount <= 1);

		if (Layouts.size() <= Space) {
			Layouts.resize(Space + 1, VK_NULL_HANDLE);
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT BindingFlagsCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT};
		BindingFlagsCreateInfo.bindingCount = static_cast<uint32_t>(ParametersFlags[Space].size());
		BindingFlagsCreateInfo.pBindingFlags = ParametersFlags[Space].data();

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
		DescriptorSetLayoutCreateInfo.flags = IsPush * VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		DescriptorSetLayoutCreateInfo.bindingCount = Parameters[Space].size();
		DescriptorSetLayoutCreateInfo.pBindings = Parameters[Space].data();
		DescriptorSetLayoutCreateInfo.pNext = &BindingFlagsCreateInfo;

		VkDescriptorSetLayout DescriptorSetLayout;
		VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
		Layouts[Space] = DescriptorSetLayout;
		IsSetPush[Space] = IsPush;

		return this;
	}

	void BuildAll(renderer_backend* Backend) override
	{
		vulkan_backend* Gfx = static_cast<vulkan_backend*>(Backend);

		Device = Gfx->Device;

		std::vector<VkDescriptorPoolSize> PoolSizes;
		for (const auto& DescriptorType : DescriptorTypeCounts) {
			VkDescriptorPoolSize PoolSize = {};
			PoolSize.type = DescriptorType.first;
			PoolSize.descriptorCount = DescriptorType.second;
			PoolSizes.push_back(PoolSize);
		}

		Sets.resize(Layouts.size(), VK_NULL_HANDLE);
		VkDescriptorPoolCreateInfo PoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		PoolInfo.poolSizeCount = PoolSizes.size();
		PoolInfo.pPoolSizes = PoolSizes.data();
		PoolInfo.maxSets = Layouts.size();

		VK_CHECK(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &Pool));

		for(u32 LayoutIdx = 0; LayoutIdx < Layouts.size(); LayoutIdx++)
		{
			if(IsSetPush[LayoutIdx]) continue;
			VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
			AllocInfo.descriptorPool = Pool;
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &Layouts[LayoutIdx];

			VkDescriptorSet DescriptorSet;
			VK_CHECK(vkAllocateDescriptorSets(Device, &AllocInfo, &DescriptorSet));
			Sets[LayoutIdx] = DescriptorSet;
		}

		VkPipelineLayoutCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		CreateInfo.pSetLayouts = Layouts.data();
		CreateInfo.setLayoutCount = Layouts.size();
		CreateInfo.pushConstantRangeCount = PushConstants.size();
		CreateInfo.pPushConstantRanges = PushConstants.data();

		VK_CHECK(vkCreatePipelineLayout(Device, &CreateInfo, nullptr, &Handle));
	}

	VkPipelineLayout Handle;
	u32 PushDescriptorSetIdx = 0;
	std::map<u32, bool> IsSetPush;
	std::vector<VkDescriptorSet> Sets;
	std::vector<VkPushConstantRange> PushConstants;
};
