#pragma once

// NOTE: https://godbolt.org/z/brdzsc8qT
template <typename T>
concept aggregate = std::is_aggregate_v<T>;

template <typename T, typename... Args>
concept aggregate_initializable = aggregate<T> 
    && requires { T{std::declval<Args>()...}; };

namespace detail
{
    struct any
    { 
        template <typename T> constexpr operator T&() const noexcept;
        template <typename T> constexpr operator T&&() const noexcept;
    };
}

namespace detail
{
    template <std::size_t I>
    using indexed_any = any;

    template <aggregate T, typename Indices>
    struct aggregate_initializable_from_indices;

    template <aggregate T, std::size_t... Indices>
    struct aggregate_initializable_from_indices<T, std::index_sequence<Indices...>>
        : std::bool_constant<aggregate_initializable<T, indexed_any<Indices>...>> {};
}

template <typename T, std::size_t N>
concept aggregate_initializable_with_n_args = aggregate<T> &&
    detail::aggregate_initializable_from_indices<T, std::make_index_sequence<N>>::value;

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
struct binary_search;

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
using binary_search_base = 
    std::conditional_t<(End - Beg <= 1), std::integral_constant<std::size_t, Beg>,
        std::conditional_t<Predicate<(Beg + End) / 2>::value,
            binary_search<Predicate, (Beg + End) / 2, End>,
            binary_search<Predicate, Beg, (Beg + End) / 2>>>;

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
struct binary_search : binary_search_base<Predicate, Beg, End> {};

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
constexpr std::size_t binary_search_v = binary_search<Predicate, Beg, End>::value;

template <template<std::size_t> typename Predicate, std::size_t N>
struct forward_search : std::conditional_t<Predicate<N>::value,
    std::integral_constant<std::size_t, N>,
    forward_search<Predicate, N+1>> {};

template <template<std::size_t> typename Predicate, std::size_t N>
struct backward_search : std::conditional_t<Predicate<N>::value,
    std::integral_constant<std::size_t, N>,
    backward_search<Predicate, N-1>> {};

template <template<std::size_t> typename Predicate>
struct backward_search<Predicate, (std::size_t)-1> {};

namespace detail
{
    template <typename T> requires std::is_aggregate_v<T>
    struct aggregate_inquiry
    {
        template <std::size_t N>
        struct initializable : std::bool_constant<aggregate_initializable_with_n_args<T, N>> {};
    };

    template <aggregate T>
    struct minimum_initialization : forward_search<
        detail::aggregate_inquiry<T>::template initializable, 0> {};

    template <aggregate T>
    constexpr auto minimum_initialization_v = minimum_initialization<T>::value;
}

template <aggregate T>
struct num_aggregate_fields : binary_search<
    detail::aggregate_inquiry<T>::template initializable,
    detail::minimum_initialization_v<T>, 8 * sizeof(T) + 1> {};

template <aggregate T>
constexpr std::size_t num_aggregate_fields_v = num_aggregate_fields<T>::value;

namespace detail
{
    template <aggregate T, typename Indices, typename FieldIndices>
    struct aggregate_with_indices_initializable_with;

    template <aggregate T, std::size_t... Indices, std::size_t... FieldIndices>
    struct aggregate_with_indices_initializable_with<T, 
        std::index_sequence<Indices...>, std::index_sequence<FieldIndices...>>
        : std::bool_constant<requires
        { 
            T{std::declval<indexed_any<Indices>>()...,
                {std::declval<indexed_any<FieldIndices>>()...}};
        }> {};
}

template <typename T, std::size_t N, std::size_t M>
concept aggregate_field_n_initializable_with_m_args = aggregate<T> &&
    detail::aggregate_with_indices_initializable_with<T,
        std::make_index_sequence<N>,
        std::make_index_sequence<M>>::value;

namespace detail
{
    template <aggregate T, std::size_t N>
    struct aggregate_field_inquiry1
    {
        template <std::size_t M>
        struct initializable : std::bool_constant<
            aggregate_field_n_initializable_with_m_args<T, N, M>> {};
    };
}

template <aggregate T, std::size_t N>
struct num_fields_on_aggregate_field1 : binary_search<
    detail::aggregate_field_inquiry1<T, N>::template initializable,
    0, 8 * sizeof(T) + 1> {};

template <aggregate T, std::size_t N>
constexpr std::size_t num_fields_on_aggregate_field1_v
    = num_fields_on_aggregate_field1<T, N>::value;

namespace detail
{
    template <aggregate T, typename Indices, typename AfterIndices>
    struct aggregate_with_indices_initializable_after;

    template <aggregate T, std::size_t... Indices, std::size_t... AfterIndices>
    struct aggregate_with_indices_initializable_after<T, 
        std::index_sequence<Indices...>, std::index_sequence<AfterIndices...>>
        : std::bool_constant<requires
        { 
            T{std::declval<indexed_any<Indices>>()..., {std::declval<any>()},
                std::declval<indexed_any<AfterIndices>>()...};
        }> {};
}

template <typename T, std::size_t N, std::size_t K>
concept aggregate_initializable_after_n_with_k_args = aggregate<T> &&
    detail::aggregate_with_indices_initializable_after<T,
        std::make_index_sequence<N>,
        std::make_index_sequence<K>>::value;

namespace detail
{
    template <aggregate T, std::size_t N>
    struct aggregate_field_inquiry2
    {
        template <std::size_t K>
        struct initializable : std::bool_constant<
            aggregate_initializable_after_n_with_k_args<T, N, K>> {};
        
        template <std::size_t K>
        struct not_initializable_m1 : std::negation<initializable<K-1>> {};
    };

    template <aggregate T, std::size_t N>
    struct le_max_init_after_n : backward_search<
        aggregate_field_inquiry2<T, N>::template initializable,
        minimum_initialization_v<T>> {};

    template <aggregate T, std::size_t N>
    constexpr std::size_t le_max_init_after_n_v
        = le_max_init_after_n<T, N>::value;

    template <aggregate T, std::size_t N>
    struct min_init_after_n : binary_search<
        aggregate_field_inquiry2<T, N>::template not_initializable_m1,
        1, 1 + le_max_init_after_n_v<T, N>> {};

    template <aggregate T, std::size_t N>
    constexpr std::size_t min_init_after_n_v = min_init_after_n<T, N>::value;

    template <typename T, std::size_t N>
    concept aggregate_field_n_init = aggregate<T> &&
        requires { le_max_init_after_n<T, N>::value; };

    template <aggregate T, std::size_t N, bool Initializable>
    struct num_fields
        : std::integral_constant<std::size_t, 1> {};

    template <aggregate T, std::size_t N>
    struct num_fields<T, N, true>
        : std::integral_constant<std::size_t, std::max(
            detail::minimum_initialization_v<T> - N
            - detail::min_init_after_n_v<T, N>,
            std::size_t(1))> {};
}

template <aggregate T, std::size_t N>
struct num_fields_on_aggregate_field2 :
    detail::num_fields<T, N,
        detail::aggregate_field_n_init<T, N>> {};

template <aggregate T, std::size_t N>
constexpr std::size_t num_fields_on_aggregate_field2_v
    = num_fields_on_aggregate_field2<T, N>::value;

template <aggregate T, std::size_t N>
struct num_fields_on_aggregate_field : std::conditional_t<
    (N >= detail::minimum_initialization_v<T>),
    num_fields_on_aggregate_field1<T, N>,
    num_fields_on_aggregate_field2<T, N>> {};

template <aggregate T, std::size_t N>
constexpr std::size_t num_fields_on_aggregate_field_v
    = num_fields_on_aggregate_field<T, N>::value;

namespace detail
{
    template <std::size_t Val, std::size_t TotalFields>
    constexpr auto detect_special_type =
        Val > TotalFields ? 1 : Val;

    template <aggregate T, std::size_t CurField,
        std::size_t TotalFields, std::size_t CountUniqueFields>
    struct unique_field_counter : unique_field_counter<T,
        CurField + detect_special_type<
            num_fields_on_aggregate_field_v<T, CurField>, TotalFields>,
        TotalFields, CountUniqueFields + 1> {};

    template <aggregate T, std::size_t Fields, std::size_t UniqueFields>
    struct unique_field_counter<T, Fields, Fields, UniqueFields>
        : std::integral_constant<std::size_t, UniqueFields> {};
}

template <aggregate T>
struct num_aggregate_unique_fields
    : detail::unique_field_counter<T, 0, num_aggregate_fields_v<T>, 0> {};

template <aggregate T>
constexpr std::size_t num_aggregate_unique_fields_v
    = num_aggregate_unique_fields<T>::value;

//////////////////////////////////////////////////////////////////////////////////////

#pragma pack(push)
#pragma pack(1)

template<typename, typename = std::void_t<>>
struct has_static_storage_type : std::false_type {};

template<typename T>
struct has_static_storage_type<T, std::void_t<typename T::static_storage_type>> : std::true_type {};

template<typename T, bool HasStaticStorage>
struct shader_pass_parameter_type {};

template<typename T>
struct shader_pass_parameter_type<T, false>
{
    u32 ParamCount = num_aggregate_fields_v<typename T::parameter_type>;
	size_t ParamSize = sizeof(typename T::parameter_type);

	bool HaveStaticStorage = false;

	typename T::parameter_type Param;
};

template<typename T>
struct shader_pass_parameter_type<T, true>
{
    u32 ParamCount = num_aggregate_fields_v<typename T::parameter_type>;
	size_t ParamSize = sizeof(typename T::parameter_type);

	bool HaveStaticStorage = true;

	typename T::parameter_type Param;
	typename T::static_storage_type StaticStorage;
};

template<typename context_type>
using shader_parameter = shader_pass_parameter_type<context_type, has_static_storage_type<context_type>::value>;

#pragma pack(pop)

namespace utils
{
	namespace render_context
	{
		// TODO: Make better abstraction
		struct input_data
		{
			cull_mode CullMode;
			bool UseColor;
			bool UseDepth;
			bool UseOutline;
			bool UseMultiview;
			bool UseConservativeRaster;
			u32  ViewMask;
		};
	};

	namespace texture
	{
		struct sampler_info
		{
			border_color BorderColor;
			sampler_address_mode AddressMode;
			sampler_reduction_mode ReductionMode;
			filter MinFilter;
			filter MagFilter;
			mipmap_mode MipmapMode;
		};

		struct input_data
		{
			image_format Format;
			image_type Type;
			u32 Usage;
			u32 MipLevels;
			u32 Layers;
			bool UseStagingBuffer;
			barrier_state InitialState;
			sampler_info SamplerInfo;
		};
	};
};

struct shader_define
{
	std::string Name;
	std::string Value;
};

struct resource_reference
{
	u64 Handle;
	u32 ResourceType;
	u32 SubresourceIndex;
};

struct buffer;
struct texture;
struct renderer_backend;
class memory_heap
{
public:
	memory_heap() = default;
	virtual ~memory_heap() = default;

	virtual void CreateResource(renderer_backend* Backend) = 0;
	virtual void DestroyResource() = 0;

	//void BeginFrame();
	//void EndFrame();
	//void CollectGarbage();

	virtual buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, u64 DataSize, u64 Count, u32 Flags) = 0;
	virtual buffer* PushBuffer(renderer_backend* Backend, std::string DebugName, void* Data, u64 DataSize, u64 Count, u32 Flags) = 0;

	virtual texture* PushTexture(renderer_backend* Backend, std::string DebugName, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) = 0;
	virtual texture* PushTexture(renderer_backend* Backend, std::string DebugName, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData) = 0;

	u64 Size = 0;
	u64 TotalSize = 0;
	u64 BeginData = 0;
	u64 Alignment = 0;

	std::unordered_map<u64, buffer*>  PersistentBuffers;
	std::unordered_map<u64, buffer*>  TransientBuffers;

	std::unordered_map<u64, texture*> PersistentTextures;
	std::unordered_map<u64, texture*> TransientTextures;

	//std::unordered_map<texture, sampler> Samplers;
};

struct renderer_backend
{
	virtual ~renderer_backend() = default;
	virtual void DestroyObject() = 0;
	virtual void RecreateSwapchain(u32 NewWidth, u32 NewHeight) = 0;

	u32 Width;
	u32 Height;
	memory_heap* GlobalHeap;
};

struct descriptor_param
{
	resource_type Type;
	u32 Count = 0;
	image_type ImageType;
	u32 ShaderToUse = 0;
	barrier_state BarrierState;
	u32 AspectMask;
};

class general_context
{
public:
	general_context() = default;
	virtual ~general_context() = default;

	general_context(const general_context&) = delete;
	general_context operator=(const general_context&) = delete;

	virtual void Clear() = 0;

	std::string Name;
	pass_type Type;
	std::map<u32, std::vector<descriptor_param>> ParameterLayout;

	u32 ParamCount = 0;
	u32 StaticStorageCount = 0;
};

class render_context;
class compute_context;
struct command_list
{
	command_list() = default;

	virtual ~command_list() = default;
	
	virtual void DestroyObject() = 0;

	virtual void CreateResource(renderer_backend* Backend) = 0;

	virtual void AcquireNextImage() = 0;

	virtual void Begin() = 0;

	virtual void End() = 0;

	virtual void DeviceWaitIdle() = 0;

	virtual void EndOneTime() = 0;

	virtual void Update(buffer* BufferToUpdate, void* Data) = 0;
	virtual void UpdateSize(buffer* BufferToUpdate, void* Data, u32 UpdateByteSize) = 0;
	virtual void ReadBackSize(buffer* BufferToRead, void* Data, u32 UpdateByteSize) = 0;

	virtual void Update(texture* TextureToUpdate, void* Data) = 0;
	virtual void ReadBack(texture* TextureToUpdate, void* Data) = 0;

	virtual void SetGraphicsPipelineState(render_context* Context) = 0;
	virtual void SetComputePipelineState(compute_context* Context) = 0;

	virtual void SetConstant(void* Data, size_t Size) = 0;

	virtual void SetViewport(u32 StartX, u32 StartY, u32 RenderWidth, u32 RenderHeight) = 0;
	virtual void SetIndexBuffer(buffer* Buffer) = 0;

	virtual void EmplaceColorTarget(texture* RenderTexture) = 0;

	virtual void Present() = 0;

	virtual void SetColorTarget(const std::vector<texture*>& Targets, vec4 Clear = {0, 0, 0, 1}) = 0;
	virtual void SetDepthTarget(texture* Target, vec2 Clear = {1, 0}) = 0;
	virtual void SetStencilTarget(texture* Target, vec2 Clear = {1, 0}) = 0;

	virtual void BindShaderParameters(void* Data) = 0;

	virtual void DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount) = 0;
	virtual void DrawIndirect(buffer* IndirectCommands, u32 ObjectDrawCount, u32 CommandStructureSize) = 0;
	virtual void Dispatch(u32 X = 1, u32 Y = 1, u32 Z = 1) = 0;

	virtual void FillBuffer(buffer* Buffer, u32 Value) = 0;
	virtual void FillTexture(texture* Texture, vec4 Value) = 0;

	virtual void CopyImage(texture* Dst, texture* Src) = 0;

	virtual void SetMemoryBarrier(u32 SrcAccess, u32 DstAccess, 
								  u32 SrcStageMask, u32 DstStageMask) = 0;

	virtual void SetBufferBarriers(const std::vector<std::tuple<buffer*, u32, u32>>& BarrierData) = 0;
	virtual void SetImageBarriers(const std::vector<std::tuple<texture*, u32, barrier_state, u32, u32>>& BarrierData) = 0;

	virtual void DebugGuiBegin(texture* RenderTarget) = 0;
	virtual void DebugGuiEnd()   = 0;

	u32 BackBufferIndex = 0;

	general_context* CurrentContext;

	std::unordered_set<buffer*>  BuffersToCommon;
	std::unordered_set<texture*> TexturesToCommon;

	std::vector<std::tuple<buffer*, u32, u32>> AttachmentBufferBarriers;
	std::vector<std::tuple<texture*, u32, barrier_state, u32, u32>> AttachmentImageBarriers;
};

struct resource_binder
{
	resource_binder() = default;

	virtual ~resource_binder() = default;

	resource_binder(const resource_binder&) = delete;
	resource_binder operator=(const resource_binder&) = delete;

	virtual void AppendStaticStorage(general_context* Context, void* Data) = 0;
	virtual void BindStaticStorage(renderer_backend* GeneralBackend) = 0;

	virtual void SetStorageBufferView(buffer* Buffer, u32 Set = 0) = 0;
	virtual void SetUniformBufferView(buffer* Buffer, u32 Set = 0) = 0;

	// TODO: Remove image layouts and move them inside texture structure
	virtual void SetSampledImage(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetStorageImage(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetImageSampler(u32 Count, const std::vector<texture*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
};

struct texture_ref
{
	u64 SubresourceIndex = SUBRESOURCES_ALL;
	std::vector<texture*> Handle;
};

// TODO: use the actual structure for the data.
// Create one VkBuffer and then update the buffer when the structure is being actually updated
// Destroy if actually stopped being used/haven't being used for some time
struct buffer_ref
{
	buffer* Handle = nullptr;
};

struct shader_view_context
{
	shader_view_context() = default;
	virtual ~shader_view_context() = default;

	std::vector<shader_define> Defines;

	virtual utils::render_context::input_data SetupPipelineState() { return {}; }
};

struct shader_graphics_view_context : public shader_view_context
{
	shader_graphics_view_context() = default;
	virtual ~shader_graphics_view_context() = default;

	load_op  LoadOp;
	store_op StoreOp;

	std::vector<std::string> Shaders;

	virtual std::vector<image_format> SetupAttachmentDescription() { return {}; }
};

struct shader_compute_view_context : public shader_view_context
{
	shader_compute_view_context() = default;
	virtual ~shader_compute_view_context() = default;

	std::string Shader;
};

struct transfer : public shader_view_context
{
};

struct full_screen : public shader_graphics_view_context
{
};

#include "general_view_functions.hpp"

class render_context : public general_context
{
public:
	render_context() = default;
	virtual ~render_context() = default;

	render_context(const render_context&) = delete;
	render_context& operator=(const render_context&) = delete;

	utils::render_context::input_data Info;
};

class compute_context : public general_context
{
public:
	compute_context() = default;
	virtual ~compute_context() = default;

	compute_context(const compute_context&) = delete;
	compute_context& operator=(const compute_context&) = delete;

	u32 BlockSizeX;
	u32 BlockSizeY;
	u32 BlockSizeZ;
};

struct buffer
{
	buffer() = default;
	virtual ~buffer() = default;

	virtual void Update(renderer_backend* Backend, void* Data) = 0;
	virtual void UpdateSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) = 0;
	virtual void ReadBackSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) = 0;

	virtual void Update(void* Data, command_list* Cmd) = 0;
	virtual void UpdateSize(void* Data, u32 UpdateByteSize, command_list* Cmd) = 0;
	virtual void ReadBackSize(void* Data, u32 UpdateByteSize, command_list* Cmd) = 0;

	virtual void CreateResource(renderer_backend* Backend, memory_heap* Heap, std::string DebugName, u64 NewSize, u64 Count, u32 Flags) = 0;
	virtual void DestroyResource() = 0;

	std::string Name;
	u64  Size          = 0;
	u64  Alignment     = 0;
	u32  CounterOffset = 0;
	bool WithCounter   = false;

	u32 PrevShader = PSF_TopOfPipe;
	u32 CurrentLayout = 0;
	u32 Usage = 0;
};

struct texture_sampler {};

struct texture
{
	texture() = default;
	virtual ~texture() = default;

	virtual void Update(renderer_backend* Backend, void* Data) = 0;
	virtual void ReadBack(renderer_backend* Backend, void* Data) = 0;

	virtual void Update(void* Data, command_list* Cmd) = 0;

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

	u32 PrevShader = PSF_TopOfPipe;
	std::vector<u32> CurrentLayout;
	std::vector<barrier_state> CurrentState;
};

namespace std
{
	template<>
    struct hash<buffer>
    {
        size_t operator()(const buffer& b) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<u64>{}(b.Size));
            hash_combine(Result, hash<u64>{}(b.Alignment));
            hash_combine(Result, hash<u32>{}(b.CounterOffset));
            hash_combine(Result, hash<bool>{}(b.WithCounter));
            return Result;
        }
    };

	template<>
    struct hash<utils::texture::sampler_info>
    {
        size_t operator()(const utils::texture::sampler_info& s) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<u64>{}(static_cast<u64>(s.BorderColor)));
            hash_combine(Result, hash<u64>{}(static_cast<u64>(s.AddressMode)));
            hash_combine(Result, hash<u64>{}(static_cast<u64>(s.ReductionMode)));
            hash_combine(Result, hash<u64>{}(static_cast<u64>(s.MinFilter)));
            hash_combine(Result, hash<u64>{}(static_cast<u64>(s.MagFilter)));
            hash_combine(Result, hash<u64>{}(static_cast<u64>(s.MipmapMode)));
            return Result;
        }
    };

	template<>
    struct hash<utils::texture::input_data>
    {
        size_t operator()(const utils::texture::input_data& id) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<u64>{}(static_cast<u64>(id.Format)));
            hash_combine(Result, hash<u64>{}(static_cast<u64>(id.Type)));
            hash_combine(Result, hash<u32>{}(id.Usage));
            hash_combine(Result, hash<u32>{}(id.MipLevels));
            hash_combine(Result, hash<u32>{}(id.Layers));
            hash_combine(Result, hash<bool>{}(id.UseStagingBuffer));
            //hash_combine(Result, hash<utils::texture::sampler_info>{}(id.SamplerInfo)); // Do I actually need this for texture hash?
            hash_combine(Result, hash<u64>{}(static_cast<u64>(id.InitialState)));
            return Result;
        }
    };

	template<>
    struct hash<texture>
    {
        size_t operator()(const texture& t) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<u64>{}(t.Width));
            hash_combine(Result, hash<u64>{}(t.Height));
            hash_combine(Result, hash<u32>{}(t.Depth));
            hash_combine(Result, hash<bool>{}(t.Size));
            hash_combine(Result, hash<utils::texture::input_data>{}(t.Info));
            return Result;
        }
    };
};

#include "reflection.gen.cpp"
