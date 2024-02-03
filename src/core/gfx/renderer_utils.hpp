#pragma once

namespace utils
{
	namespace render_context
	{
		// TODO: Make better abstraction
		struct input_data
		{
			bool UseColor;
			bool UseDepth;
			bool UseBackFace;
			bool UseOutline;
			bool UseMultiview;
			u32  ViewMask;
		};
	};

	namespace texture
	{
		struct input_data
		{
			image_format Format;
			image_type Type;
			u32 Usage;
			u32 MipLevels;
			u32 Layers;
			bool UseStagingBuffer;
			border_color BorderColor;
			sampler_address_mode AddressMode;
			sampler_reduction_mode ReductionMode;
		};
	};
};

struct shader_define
{
	std::string Name;
	std::string Value;
};

struct renderer_backend
{
	virtual ~renderer_backend() = default;
	virtual void DestroyObject() = 0;
	virtual void RecreateSwapchain(u32 NewWidth, u32 NewHeight) = 0;

	u32 Width;
	u32 Height;
};

struct buffer;
struct texture;
struct global_pipeline_context
{
	global_pipeline_context() = default;

	virtual ~global_pipeline_context() = default;
	
	virtual void DestroyObject() = 0;

	virtual void CreateResource(renderer_backend* Backend) = 0;

	virtual void Begin(renderer_backend* Backend) = 0;

	virtual void End(renderer_backend* Backend) = 0;

	virtual void DeviceWaitIdle() = 0;

	virtual void EndOneTime(renderer_backend* Backend) = 0;

	virtual void EmplaceColorTarget(renderer_backend* Backend, texture* RenderTexture) = 0;

	virtual void Present(renderer_backend* Backend) = 0;

	virtual void FillBuffer(buffer* Buffer, u32 Value) = 0;

	virtual void CopyImage(texture* Dst, texture* Src) = 0;

	virtual void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, 
								  u32 SrcStageMask, u32 DstStageMask) = 0;

	virtual void SetBufferBarrier(const std::tuple<buffer*, u32, u32>& BarrierData, 
								  u32 SrcStageMask, u32 DstStageMask) = 0;

	virtual void SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData, 
								   u32 SrcStageMask, u32 DstStageMask) = 0;

	virtual void SetImageBarriers(const std::vector<std::tuple<texture*, u32, u32, barrier_state, barrier_state>>& BarrierData, 
								  u32 SrcStageMask, u32 DstStageMask) = 0;

	virtual void SetImageBarriers(const std::vector<std::tuple<std::vector<texture*>, u32, u32, barrier_state, barrier_state>>& BarrierData, 
								  u32 SrcStageMask, u32 DstStageMask) = 0;

	virtual void DebugGuiBegin(renderer_backend* Backend, texture* RenderTarget) = 0;
	virtual void DebugGuiEnd(renderer_backend* Backend)   = 0;

	u32 BackBufferIndex = 0;
};

class render_context
{
public:
	render_context() = default;
	virtual ~render_context() = default;

	render_context(const render_context&) = delete;
	render_context& operator=(const render_context&) = delete;

	virtual void DestroyObject() = 0;

	virtual void Begin(global_pipeline_context* GlobalPipelineContext, u32 RenderWidth, u32 RenderHeight) = 0;
	virtual void End()   = 0;
	virtual void Clear() = 0;

	virtual void SetColorTarget(u32 RenderWidth, u32 RenderHeight, const std::vector<texture*>& ColorAttachments, vec4 Clear, u32 Face = 0, bool EnableMultiview = false) = 0;
	virtual void SetDepthTarget(u32 RenderWidth, u32 RenderHeight, texture* DepthAttachment, vec2 Clear, u32 Face = 0, bool EnableMultiview = false) = 0;
	virtual void SetStencilTarget(u32 RenderWidth, u32 RenderHeight, texture* StencilAttachment, vec2 Clear, u32 Face = 0, bool EnableMultiview = false) = 0;

	virtual void StaticUpdate() = 0;

	// TODO: Remove this draw
	virtual void Draw(buffer* VertexBuffer, u32 FirstVertex, u32 VertexCount) = 0;

	virtual void DrawIndexed(buffer* IndexBuffer, u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance = 0, u32 InstanceCount = 1) = 0;
	virtual void DrawIndirect(u32 ObjectDrawCount, buffer* IndexBuffer, buffer* IndirectCommands, u32 CommandStructureSize) = 0;

	virtual void SetConstant(void* Data, size_t Size) = 0;

	// NOTE: If with counter, then it is using 2 bindings instead of 1
	virtual void SetStorageBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) = 0;
	virtual void SetUniformBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) = 0;

	// TODO: Remove image layouts and move them inside texture structure
	virtual void SetSampledImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetStorageImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetImageSampler(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;

	utils::render_context::input_data Info;
};

class compute_context
{
public:
	compute_context() = default;
	virtual ~compute_context() = default;

	compute_context(const compute_context&) = delete;
	compute_context& operator=(const compute_context&) = delete;

	virtual void DestroyObject() = 0;

	virtual void Begin(global_pipeline_context* GlobalPipelineContext) = 0;
	virtual void End()   = 0;
	virtual void Clear() = 0;

	virtual void StaticUpdate() = 0;

	virtual void Execute(u32 X = 1, u32 Y = 1, u32 Z = 1) = 0;

	virtual void SetConstant(void* Data, size_t Size) = 0;

	virtual void SetStorageBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) = 0;
	virtual void SetUniformBufferView(buffer* Buffer, bool UseCounter = true, u32 Set = 0) = 0;

	// TODO: Remove image layouts and move them inside texture structure
	virtual void SetSampledImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetStorageImage(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetImageSampler(const std::vector<texture*>& Textures, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
};

class memory_heap
{
public:
	memory_heap() = default;
	virtual ~memory_heap() = default;

	virtual void CreateResource(renderer_backend* Backend) = 0;

	virtual buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, u64 DataSize, u64 Count, bool NewWithCounter, u32 Flags) = 0;
	virtual buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, void* Data, u64 DataSize, u64 Count, bool NewWithCounter, u32 Flags) = 0;

	virtual texture* PushTexture(renderer_backend* Backend, std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) = 0;
	virtual texture* PushTexture(renderer_backend* Backend, std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) = 0;

	u64 Size = 0;
	u64 TotalSize = 0;
	u64 BeginData = 0;
	u64 Alignment = 0;
};

struct buffer
{
	buffer() = default;
	virtual ~buffer() = default;

	virtual void Update(renderer_backend* Backend, void* Data) = 0;
	virtual void UpdateSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) = 0;
	virtual void Update(void* Data, global_pipeline_context* PipelineContext) = 0;
	virtual void UpdateSize(void* Data, u32 UpdateByteSize, global_pipeline_context* PipelineContext) = 0;

	virtual void ReadBackSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) = 0;
	virtual void ReadBackSize(void* Data, u32 UpdateByteSize, global_pipeline_context* PipelineContext) = 0;

	virtual void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewSize, u64 Count, bool NewWithCounter, u32 Flags) = 0;
	virtual void DestroyResource() = 0;

	std::string Name;
	u64  Size          = 0;
	u64  Alignment     = 0;
	u32  CounterOffset = 0;
	bool WithCounter   = false;
};

struct texture
{
	texture() = default;
	virtual ~texture() = default;

	virtual void Update(renderer_backend* Backend, void* Data) = 0;
	virtual void Update(void* Data, global_pipeline_context* PipelineContext) = 0;
	virtual void ReadBack(renderer_backend* Backend, void* Data) = 0;

	virtual void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const utils::texture::input_data& InputData) = 0;
	virtual void CreateStagingResource() = 0;
	virtual void DestroyResource() = 0;
	virtual void DestroyStagingResource() = 0;

	std::string Name;
	u64 Width;
	u64 Height;
	u64 Depth;
	u64 Size;

	utils::texture::input_data Info;
};
