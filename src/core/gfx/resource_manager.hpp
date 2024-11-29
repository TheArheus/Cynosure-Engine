#pragma once

class gpu_memory_heap
{
	renderer_backend* Gfx = nullptr;

	std::unordered_map<u64, resource*> Resources;
	//std::unordered_map<u64, resource_descriptor> Descriptors;
	std::vector<u64> Unused;

	u64 NextID = 1;

public:
	gpu_memory_heap(renderer_backend* Backend) : Gfx(Backend) {};
	~gpu_memory_heap() = default;

	// TODO: Need to think about how to manage the resources which are transient.
	// Also I need to think how they should be: per-frame or per-pass. And when they will go out of the scope(frame or pass)
	// I need to think how they will be managed: either they could be reused somehow or deleted in the end of the frame(but this one will be bad if I will allocate the very same buffer every frame)

	// NOTE: this is persistent(per-program) resource management. 
	// I think the API would be somethink like this.
	// But, still, need some room for the better implementation

	buffer_descriptor AllocateBuffer(u64 Size, u64 Count, u32 Usage)
	{
		buffer* NewBuffer = nullptr;
		buffer_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Size  = Size;
		Descriptor.Count = Count;
		Descriptor.Usage = Usage;
		if(Resources.find(Descriptor.ID) != Resources.end())
		{
			return Descriptor;
		}

		switch(Gfx->Type)
		{
			case backend_type::vulkan:
				NewBuffer = new vulkan_buffer(Gfx, "", Size, Count, Usage);
#if _WIN32
			case backend_type::directx12:
				NewBuffer = new directx12_buffer(Gfx, "", Size, Count, Usage);
#endif
		}
		return Descriptor;
	}

	buffer_descriptor AllocateBuffer(u64 Size, u64 Count, u32 Usage, void* Data)
	{
		buffer* NewBuffer = nullptr;
		buffer_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Size  = Size;
		Descriptor.Count = Count;
		Descriptor.Usage = Usage;
		if(Resources.find(Descriptor.ID) != Resources.end())
		{
			return Descriptor;
		}

		switch(Gfx->Type)
		{
			case backend_type::vulkan:
				NewBuffer = new vulkan_buffer(Gfx, "", Data, Size, Count, Usage);
#if _WIN32
			case backend_type::directx12:
				NewBuffer = new directx12_buffer(Gfx, "", Data, Size, Count, Usage);
#endif
		}
		return Descriptor;
	}

	texture_descriptor AllocateTexture(u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData)
	{
		texture* NewTexture = nullptr;
		texture_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Width  = Width;
		Descriptor.Height = Height;
		Descriptor.Depth  = Depth;
		Descriptor.Info   = InputData;
		if(Resources.find(Descriptor.ID) != Resources.end())
		{
			return Descriptor;
		}

		switch(Gfx->Type)
		{
			case backend_type::vulkan:
				NewTexture = new vulkan_texture(Gfx, "", nullptr, Width, Height, Depth, InputData);
#if _WIN32
			case backend_type::directx12:
				NewTexture = new directx12_texture(Gfx, "", nullptr, Width, Height, Depth, InputData);
#endif
		}
		return Descriptor;
	}

	texture_descriptor AllocateTexture(u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& InputData, void* Data)
	{
		texture* NewTexture = nullptr;
		texture_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Width  = Width;
		Descriptor.Height = Height;
		Descriptor.Depth  = Depth;
		Descriptor.Info   = InputData;
		if(Resources.find(Descriptor.ID) != Resources.end())
		{
			return Descriptor;
		}

		switch(Gfx->Type)
		{
			case backend_type::vulkan:
				NewTexture = new vulkan_texture(Gfx, "", Data, Width, Height, Depth, InputData);
#if _WIN32
			case backend_type::directx12:
				NewTexture = new directx12_texture(Gfx, "", Data, Width, Height, Depth, InputData);
#endif
		}
		return Descriptor;
	}

	[[nodiscard]] buffer* GetBuffer(const buffer_descriptor& Desc)
	{
		if(Resources.find(Desc.ID) != Resources.end())
		{
			return (buffer*)Resources[Desc.ID];
		}
		return nullptr;
	}

	[[nodiscard]] texture* GetTexture(const texture_descriptor& Desc)
	{
		if(Resources.find(Desc.ID) != Resources.end())
		{
			return (texture*)Resources[Desc.ID];
		}
		return nullptr;
	}

	void ReleaseBuffer(const buffer_descriptor& Desc)
	{
		auto it = Resources.find(Desc.ID);
		if(it != Resources.end())
		{
			Unused.push_back(Desc.ID);
		}
	}

	void ReleaseTexture(const texture_descriptor& Desc)
	{
		auto it = Resources.find(Desc.ID);
		if(it != Resources.end())
		{
			Unused.push_back(Desc.ID);
		}
	}

	void FreeBuffer(const buffer_descriptor& Desc)
	{
		auto it = Resources.find(Desc.ID);
		if(it != Resources.end())
		{
			delete it->second;
			Resources.erase(it);
			//Descriptors.erase(Desc.ID);
			Unused.push_back(Desc.ID);
		}
	}

	void FreeTexture(const texture_descriptor& Desc)
	{
		auto it = Resources.find(Desc.ID);
		if(it != Resources.end())
		{
			delete it->second;
			Resources.erase(it);
			//Descriptors.erase(Desc.ID);
			Unused.push_back(Desc.ID);
		}
	}
};
