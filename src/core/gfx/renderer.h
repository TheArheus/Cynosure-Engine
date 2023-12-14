
class global_graphics_context
{
	memory_heap GlobalHeap;

	global_graphics_context(const global_graphics_context&) = delete;
	global_graphics_context& operator=(const global_graphics_context&) = delete;

public:
	global_graphics_context() = default;
	global_graphics_context(renderer_backend* NewBackend);

	global_graphics_context(global_graphics_context&& Oth) noexcept;
	global_graphics_context& operator=(global_graphics_context&& Oth) noexcept;

	buffer PushBuffer(u64 DataSize, bool NewWithCounter, VkBufferUsageFlags Flags)
	{
		return GlobalHeap.PushBuffer(Backend, CommandQueue, DataSize, NewWithCounter, Flags);
	}

	buffer PushBuffer(void* Data, u64 DataSize, bool NewWithCounter, VkBufferUsageFlags Flags)
	{
		return GlobalHeap.PushBuffer(Backend, CommandQueue, Data, DataSize, NewWithCounter, Flags);
	}

	texture PushTexture(u32 Width, u32 Height, u32 Depth, const texture::input_data& InputData, 
						 VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, 
						 VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		return GlobalHeap.PushTexture(Backend, CommandQueue, Width, Height, Depth, InputData, ReductionMode, AddressMode);
	}

	texture PushTexture(void* Data, u32 Width, u32 Height, u32 Depth, const texture::input_data& InputData, 
						 VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, 
						 VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
	{
		return GlobalHeap.PushTexture(Backend, CommandQueue, Data, Width, Height, Depth, InputData, ReductionMode, AddressMode);
	}

	renderer_backend* Backend;
	command_queue CommandQueue;

	texture GfxColorTarget;
	texture GfxDepthTarget;
	texture DebugCameraViewDepthTarget;

	std::vector<texture> GlobalShadow;
	std::vector<texture> GBuffer;

	texture AmbientOcclusionData;
	texture DepthPyramid;
	texture RandomAnglesTexture;
	texture NoiseTexture;

	buffer PoissonDiskBuffer;
	buffer RandomSamplesBuffer;

	std::vector<render_context> CubeMapShadowContexts;
	render_context GfxContext;
	render_context CascadeShadowContext;
	render_context ShadowContext;
	render_context DebugCameraViewContext;
	render_context DebugContext;

	compute_context ColorPassContext;
	compute_context AmbientOcclusionContext;
	compute_context ShadowComputeContext;
	compute_context FrustCullingContext;
	compute_context OcclCullingContext;
	compute_context DepthReduceContext;
	compute_context BlurContext;
	compute_context DebugComputeContext;
};
