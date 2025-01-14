#pragma once

// TODO: Better resource reusing
class gpu_memory_heap
{
	renderer_backend* Gfx = nullptr;

	std::unordered_map<u64, resource*> Resources;
	std::unordered_map<u64, resource_descriptor> Descriptors;
	std::vector<u64> Unused;

	u64 NextID = 0;

	buffer* AllocateBufferInternal(const resource_descriptor& Desc, command_list* CommandList = nullptr);
	texture* AllocateTextureInternal(const resource_descriptor& Desc, command_list* CommandList = nullptr);

public:
	RENDERER_API gpu_memory_heap(renderer_backend* Backend);
	RENDERER_API ~gpu_memory_heap();

	RENDERER_API resource_descriptor CreateBuffer(const std::string& Name, void* Data, u64 Size, u32 Usage);
	RENDERER_API resource_descriptor CreateBuffer(const std::string& Name, void* Data, u64 Size, u64 Count, u32 Usage);
	RENDERER_API resource_descriptor CreateBuffer(const std::string& Name, u64 Size, u32 Usage);
	RENDERER_API resource_descriptor CreateBuffer(const std::string& Name, u64 Size, u64 Count, u32 Usage);

	RENDERER_API resource_descriptor CreateTexture(const std::string& Name, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& Info = {image_format::R8G8B8A8_UINT, image_type::Texture2D, image_flags::TF_Storage, 1, 1, barrier_state::undefined, {border_color::black_opaque, sampler_address_mode::clamp_to_edge, sampler_reduction_mode::weighted_average, filter::linear, filter::linear, mipmap_mode::linear}});
	RENDERER_API resource_descriptor CreateTexture(const std::string& Name, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& Info = {image_format::R8G8B8A8_UINT, image_type::Texture2D, image_flags::TF_Storage, 1, 1, barrier_state::undefined, {border_color::black_opaque, sampler_address_mode::clamp_to_edge, sampler_reduction_mode::weighted_average, filter::linear, filter::linear, mipmap_mode::linear}});

	[[nodiscard]] RENDERER_API resource_descriptor GetResourceDescriptor(u64 ID);

	[[nodiscard]] RENDERER_API buffer* GetBuffer(const resource_descriptor& Desc);
	[[nodiscard]] RENDERER_API buffer* GetBuffer(u64 ID);
	[[nodiscard]] RENDERER_API buffer* GetBuffer(command_list* CommandList, const resource_descriptor& Desc);
	[[nodiscard]] RENDERER_API buffer* GetBuffer(command_list* CommandList, u64 ID);

	[[nodiscard]] RENDERER_API texture* GetTexture(const resource_descriptor& Desc);
	[[nodiscard]] RENDERER_API texture* GetTexture(u64 ID);
	[[nodiscard]] RENDERER_API texture* GetTexture(command_list* CommandList, const resource_descriptor& Desc);
	[[nodiscard]] RENDERER_API texture* GetTexture(command_list* CommandList, u64 ID);

	RENDERER_API void UpdateTexture(resource_descriptor& Desc, void* Data);
	RENDERER_API void UpdateBuffer(resource_descriptor& Desc, void* Data, size_t Size);
	RENDERER_API void UpdateBuffer(resource_descriptor& Desc, void* Data, size_t Size, size_t Count);

	RENDERER_API void ReleaseBuffer(const resource_descriptor& Desc);
	RENDERER_API void ReleaseTexture(const resource_descriptor& Desc);

	RENDERER_API void FreeBuffer(const resource_descriptor& Desc);
	RENDERER_API void FreeTexture(const resource_descriptor& Desc);
};
