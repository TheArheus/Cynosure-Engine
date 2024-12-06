#pragma once

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

namespace std
{
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
            hash_combine(Result, hash<u64>{}(static_cast<u64>(id.InitialState)));
            return Result;
        }
    };
};

struct descriptor_param
{
	resource_type Type;
	u32 Count = 0;
	bool IsWritable = false;
	image_type ImageType;
	u32 ShaderToUse = 0;
	barrier_state BarrierState;
	u32 AspectMask;
};

struct shader_define
{
	std::string Name;
	std::string Value;
};

// NOTE: this will be something that will be used to requirest the gpu resource
#pragma pack(push, 1)
struct resource_descriptor
{
    std::string Name;

    u64 ID;
    u64 SubresourceIdx = SUBRESOURCES_ALL;
    void* Data = nullptr;
    resource_descriptor_type Type;

    u64 Size;
    u64 Count;
    u32 Usage;

    u64 Width;
    u64 Height;
    u64 Depth;
    utils::texture::input_data Info;

    resource_descriptor()
        : Name(), ID(0), SubresourceIdx(SUBRESOURCES_ALL),
          Data(nullptr), Type(), 
		  Size(0), Count(0), Usage(0),
          Width(0), Height(0), Depth(0), Info()
    {}

    resource_descriptor(const resource_descriptor& other)
        : Name(other.Name), ID(other.ID), SubresourceIdx(other.SubresourceIdx),
          Data(other.Data), Type(other.Type),
          Size(other.Size), Count(other.Count), Usage(other.Usage),
          Width(other.Width), Height(other.Height), Depth(other.Depth), Info(other.Info)
    {}

    resource_descriptor(resource_descriptor&& other) noexcept
        : Name(std::move(other.Name)), ID(std::move(other.ID)), SubresourceIdx(std::move(other.SubresourceIdx)),
          Data(other.Data), Type(std::move(other.Type)),
          Size(std::move(other.Size)), Count(std::move(other.Count)), Usage(std::move(other.Usage)),
          Width(std::move(other.Width)), Height(std::move(other.Height)), Depth(std::move(other.Depth)), Info(std::move(other.Info))
    {
        other.Data = nullptr;
    }

    resource_descriptor& operator=(const resource_descriptor& other)
    {
        if (this != &other)
        {
            Name = other.Name;
            ID = other.ID;
            SubresourceIdx = other.SubresourceIdx;
            Data = other.Data;
            Type = other.Type;
            Size = other.Size;
            Count = other.Count;
            Usage = other.Usage;
            Width = other.Width;
            Height = other.Height;
            Depth = other.Depth;
            Info = other.Info;
        }
        return *this;
    }

    resource_descriptor& operator=(resource_descriptor&& other) noexcept
    {
        if (this != &other)
        {
            Name = std::move(other.Name);
            ID = std::move(other.ID);
            SubresourceIdx = std::move(other.SubresourceIdx);
            Data = other.Data;
            Type = std::move(other.Type);
            Size = std::move(other.Size);
            Count = std::move(other.Count);
            Usage = std::move(other.Usage);
            Width = std::move(other.Width);
            Height = std::move(other.Height);
            Depth = std::move(other.Depth);
            Info = std::move(other.Info);

            other.Data = nullptr;
        }
        return *this;
    }

    resource_descriptor UseSubresource(u32 SubresourceIdxToUse) const
    {
        resource_descriptor NewDescriptor = *this;
        NewDescriptor.SubresourceIdx = SubresourceIdxToUse;
        return NewDescriptor;
    }
};

struct gpu_buffer
{
	u64 ID;
	bool WithCounter = false;

	gpu_buffer() = default;
	gpu_buffer(const resource_descriptor& Desc)
	{
		assert(Desc.Type == resource_descriptor_type::buffer && "Wrong resource: should be buffer");
		ID = Desc.ID;
		WithCounter = RF_WithCounter & Desc.Usage;
	}

	gpu_buffer& operator=(const resource_descriptor& Desc)
	{
		assert(Desc.Type == resource_descriptor_type::buffer && "Wrong resource: should be buffer");
		ID = Desc.ID;
		WithCounter = RF_WithCounter & Desc.Usage;
		return *this;
	}
};

struct gpu_texture
{
	u64 ID;
    u64 SubresourceIdx = SUBRESOURCES_ALL;

	gpu_texture() = default;
	gpu_texture(const resource_descriptor& Desc)
	{
		assert(Desc.Type == resource_descriptor_type::texture && "Wrong resource: should be texture");
		ID = Desc.ID;
		SubresourceIdx = Desc.SubresourceIdx;
	}
	gpu_texture& operator=(const resource_descriptor& Desc)
	{
		assert(Desc.Type == resource_descriptor_type::texture && "Wrong resource: should be texture");
		ID = Desc.ID;
		SubresourceIdx = Desc.SubresourceIdx;
		return *this;
	}
};

struct gpu_texture_array
{
	array<u64> IDs;
    u64 SubresourceIdx = SUBRESOURCES_ALL;

	gpu_texture_array() = default;
	gpu_texture_array(const std::vector<resource_descriptor>& Vec)
	{
		IDs = array<u64>(Vec.size());
		for(size_t Idx = 0; Idx < Vec.size(); Idx++)
		{
			assert(Vec[Idx].Type == resource_descriptor_type::texture && "Wrong resource: should be texture");
			IDs[Idx] = Vec[Idx].ID;
		}
	}
	gpu_texture_array(const resource_descriptor& Desc)
	{
		assert(Desc.Type == resource_descriptor_type::texture && "Wrong resource: should be texture");
		IDs = array<u64>(1);
		IDs[0] = Desc.ID;
		SubresourceIdx = Desc.SubresourceIdx;
	}

	gpu_texture_array& operator=(const std::vector<resource_descriptor>& Vec)
	{
		IDs = array<u64>(Vec.size());
		for(size_t Idx = 0; Idx < Vec.size(); Idx++)
		{
			assert(Vec[Idx].Type == resource_descriptor_type::texture && "Wrong resource: should be texture");
			IDs[Idx] = Vec[Idx].ID;
		}
		return *this;
	}
	gpu_texture_array& operator=(const resource_descriptor& Desc)
	{
		assert(Desc.Type == resource_descriptor_type::texture && "Wrong resource: should be texture");
		IDs = array<u64>(1);
		IDs[0] = Desc.ID;
		SubresourceIdx = Desc.SubresourceIdx;
		return *this;
	}

	size_t size() { return IDs.size(); }
    u64& operator[](size_t i) { return IDs[i]; }
};
#pragma pack(pop)

struct renderer_backend
{
	virtual ~renderer_backend() = default;
	virtual void DestroyObject() = 0;
	virtual void RecreateSwapchain(u32 NewWidth, u32 NewHeight) = 0;

	u32 Width;
	u32 Height;
	backend_type Type;
};

class general_context
{
public:
	general_context() = default;
	virtual ~general_context() = default;

	general_context(const general_context&) = delete;
	general_context operator=(const general_context&) = delete;

	virtual void Clear() = 0;
	virtual void DestroyObject() = 0;

	std::string Name;
	pass_type Type;
	std::map<u32, std::vector<descriptor_param>> ParameterLayout;
};

struct buffer;
struct texture;
struct resource;
struct binding_packet
{
	resource* Resource;
	array<resource*> Resources;
	u64 SubresourceIndex = SUBRESOURCES_ALL;
	u64 Mips = SUBRESOURCES_ALL;
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
	virtual void ReadBack(buffer* BufferToRead, void* Data) = 0;
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

	virtual void BindShaderParameters(const array<binding_packet>& Data) = 0;

	virtual void DrawIndexed(u32 FirstIndex, u32 IndexCount, s32 VertexOffset, u32 FirstInstance, u32 InstanceCount) = 0;
	virtual void DrawIndirect(buffer* IndirectCommands, u32 ObjectDrawCount, u32 CommandStructureSize) = 0;
	virtual void Dispatch(u32 X = 1, u32 Y = 1, u32 Z = 1) = 0;

	virtual void FillBuffer(buffer* Buffer, u32 Value) = 0;
	virtual void FillTexture(texture* Texture, vec4 Value) = 0;
	virtual void FillTexture(texture* Texture, float Depth, u32 Stencil) = 0;

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
	virtual void DestroyObject() = 0;

	resource_binder(const resource_binder&) = delete;
	resource_binder operator=(const resource_binder&) = delete;

	virtual void SetContext(general_context* ContextToUse) = 0;

	virtual void AppendStaticStorage(general_context* Context, const array<binding_packet>& Data, u32 Offset) = 0;
	virtual void BindStaticStorage(renderer_backend* GeneralBackend) = 0;

	virtual void SetStorageBufferView(resource* Buffer, u32 Set = 0) = 0;
	virtual void SetUniformBufferView(resource* Buffer, u32 Set = 0) = 0;

	// TODO: Remove image layouts and move them inside texture structure
	virtual void SetSampledImage(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetStorageImage(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
	virtual void SetImageSampler(u32 Count, const array<resource*>& Textures, image_type Type, barrier_state State, u32 ViewIdx = 0, u32 Set = 0) = 0;
};


// TODO: Use custom containers instead of std::vector and std::string
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
	virtual ~shader_graphics_view_context() override = default;

	load_op  LoadOp;
	store_op StoreOp;

	std::vector<std::string> Shaders;

	virtual std::vector<image_format> SetupAttachmentDescription() { return {}; }
};

struct shader_compute_view_context : public shader_view_context
{
	shader_compute_view_context() = default;
	virtual ~shader_compute_view_context() override = default;

	std::string Shader;
};

struct transfer : public shader_view_context
{
};

struct full_screen : public shader_graphics_view_context
{
};

#pragma pack(push, 1)
#include "general_view_functions.hpp"
#pragma pack(pop)

class render_context : public general_context
{
public:
	render_context() = default;
	virtual ~render_context() override = default;

	render_context(const render_context&) = delete;
	render_context& operator=(const render_context&) = delete;

	utils::render_context::input_data Info;
};

class compute_context : public general_context
{
public:
	compute_context() = default;
	virtual ~compute_context() override = default;

	compute_context(const compute_context&) = delete;
	compute_context& operator=(const compute_context&) = delete;

	u32 BlockSizeX;
	u32 BlockSizeY;
	u32 BlockSizeZ;
};

struct texture_sampler {};

struct resource
{
    resource() = default;
    virtual ~resource() = default;

    resource(const resource&) = delete;
    resource& operator=(const resource&) = delete;

    virtual void Update(renderer_backend* Backend, void* Data) = 0;
    virtual void ReadBack(renderer_backend* Backend, void* Data) = 0;
	virtual void Update(void* Data, command_list* Cmd) = 0;
	virtual void ReadBack(void* Data, command_list* Cmd) = 0;

    virtual void DestroyResource() = 0;

    std::string Name;
};

struct buffer : resource
{
	buffer() = default;
	virtual ~buffer() override = default;

    buffer(const buffer&) = delete;
    buffer& operator=(const buffer&) = delete;

	void Update(void* Data, command_list* Cmd) override
	{
		Cmd->Update(this, Data);
	}

	void UpdateSize(void* Data, u32 UpdateByteSize, command_list* Cmd)
	{
		Cmd->UpdateSize(this, Data, UpdateByteSize);
	}

	void ReadBack(void* Data, command_list* Cmd) override
	{
		Cmd->ReadBack(this, Data);
	}

	void ReadBackSize(void* Data, u32 UpdateByteSize, command_list* Cmd)
	{
		Cmd->ReadBackSize(this, Data, UpdateByteSize);
	}

	virtual void DestroyResource() override = 0;

	virtual void Update(renderer_backend* Backend, void* Data) override = 0;
	virtual void UpdateSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) = 0;
	virtual void ReadBack(renderer_backend* Backend, void* Data) override = 0;
	virtual void ReadBackSize(renderer_backend* Backend, void* Data, u32 UpdateByteSize) = 0;

	virtual void CreateResource(renderer_backend* Backend, std::string DebugName, u64 NewSize, u64 Count, u32 Flags) = 0;

	u64 Size          = 0;
	u64 Alignment     = 0;
	u32 CounterOffset = 0;

	u32 PrevShader    = PSF_TopOfPipe;
	u32 CurrentLayout = 0;
	u32 Usage         = 0;

	bool WithCounter  = false;
};

struct texture : resource
{
	texture() = default;
	virtual ~texture() override = default;

    texture(const texture&) = delete;
    texture& operator=(const texture&) = delete;

	void Update(void* Data, command_list* Cmd) override
	{
		Cmd->Update(this, Data);
	}

	void ReadBack(void* Data, command_list* Cmd) override
	{
		Cmd->ReadBack(this, Data);
	}

	virtual void Update(renderer_backend* Backend, void* Data) override = 0;
	virtual void ReadBack(renderer_backend* Backend, void* Data) override = 0;

	virtual void DestroyResource() override = 0;

	virtual void CreateResource(renderer_backend* Backend, std::string DebugName, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize, const utils::texture::input_data& InputData) = 0;
	virtual void CreateStagingResource() = 0;
	virtual void DestroyStagingResource() = 0;

	u64 Width  = 0;
	u64 Height = 0;
	u64 Depth  = 0;
	u64 Size   = 0;

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
            hash_combine(Result, hash<u32>{}(b.Usage));
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
            hash_combine(Result, hash<u64>{}(t.Depth));
            hash_combine(Result, hash<utils::texture::input_data>{}(t.Info));
            return Result;
        }
    };
};

#include "reflection.gen.cpp"
