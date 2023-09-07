
// TODO: use vkAllocateDescriptorSets and vkCmdBindDescriptorSets
class shader_input
{
	std::map<u32, std::vector<VkDescriptorSetLayoutBinding>> Parameters;
	std::vector<VkDescriptorSetLayout> Layouts;
	u32 GlobalOffset = 0;
	VkDevice Device;

public:
	shader_input() = default;
	~shader_input()
	{
		vkDestroyPipelineLayout(Device, Handle, nullptr);
	}

	shader_input* PushStorageBuffer(u32 Register, u32 Count = 1, u32 Space = 0, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = Flags;
		Parameter.binding = Register;
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		Parameter.descriptorCount = 1;
		Parameters[Space].push_back(Parameter);

		return this;
	}

	shader_input* PushUniformBuffer(u32 Register, u32 Count = 1, u32 Space = 0, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = Flags;
		Parameter.binding = Register;
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		Parameter.descriptorCount = 1;
		Parameters[Space].push_back(Parameter);

		return this;
	}

	shader_input* PushSampler(u32 Register, u32 Count = 1, u32 Space = 0, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = Flags;
		Parameter.binding = Register;
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);

		return this;
	}

	shader_input* PushSampledImage(u32 Register, u32 Count = 1, u32 Space = 0, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = Flags;
		Parameter.binding = Register;
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);

		return this;
	}

	shader_input* PushStorageImage(u32 Register, u32 Count = 1, u32 Space = 0, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = Flags;
		Parameter.binding = Register;
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);

		return this;
	}

	shader_input* PushImageSampler(u32 Register, u32 Count = 1, u32 Space = 0, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkDescriptorSetLayoutBinding Parameter = {};
		Parameter.stageFlags = Flags;
		Parameter.binding = Register;
		Parameter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		Parameter.descriptorCount = Count;
		Parameters[Space].push_back(Parameter);

		return this;
	}

	shader_input* PushConstant(u32 Size, VkShaderStageFlagBits Flags = VK_SHADER_STAGE_ALL)
	{
		VkPushConstantRange ConstantRange = {};
		ConstantRange.stageFlags = Flags;
		ConstantRange.offset = GlobalOffset;
		ConstantRange.size   = Size;
		PushConstants.push_back(ConstantRange);
		GlobalOffset += Size;

		return this;
	}

	template<class backend>
	void Build(std::unique_ptr<backend>& Gfx)
	{
		Device = Gfx->Device;
		for(u32 LayoutIndex = 0;
			LayoutIndex < Parameters.size();
			++LayoutIndex)
		{
			VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; 
			DescriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			DescriptorSetLayoutCreateInfo.bindingCount = Parameters[LayoutIndex].size();
			DescriptorSetLayoutCreateInfo.pBindings = Parameters[LayoutIndex].data();

			VkDescriptorSetLayout DescriptorSetLayout;
			VK_CHECK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, 0, &DescriptorSetLayout));
			Layouts.push_back(DescriptorSetLayout);
		}

		VkPipelineLayoutCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		CreateInfo.pSetLayouts = Layouts.data();
		CreateInfo.setLayoutCount = Layouts.size();
		CreateInfo.pushConstantRangeCount = PushConstants.size();
		CreateInfo.pPushConstantRanges = PushConstants.data();

		VK_CHECK(vkCreatePipelineLayout(Device, &CreateInfo, nullptr, &Handle));
	}

	VkPipelineLayout Handle;
	std::vector<VkPushConstantRange> PushConstants;
};
