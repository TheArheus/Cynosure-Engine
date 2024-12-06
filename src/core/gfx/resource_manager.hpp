#pragma once

class gpu_memory_heap
{
	renderer_backend* Gfx = nullptr;

	std::unordered_map<u64, resource*> Resources;
	std::unordered_map<u64, resource_descriptor> Descriptors;
	std::vector<u64> Unused;

	u64 NextID = 0;

	buffer* AllocateBufferInternal(const resource_descriptor& Desc)
	{
		buffer* NewBuffer = nullptr;
		switch(Gfx->Type)
		{
			case backend_type::vulkan:
				NewBuffer = new vulkan_buffer(Gfx, Desc.Name, Desc.Data, Desc.Size, Desc.Count, Desc.Usage);
				break;
#if _WIN32
			case backend_type::directx12:
				NewBuffer = new directx12_buffer(Gfx, Desc.Name, Desc.Data, Desc.Size, Desc.Count, Desc.Usage);
				break;
#endif
		}
		Resources[Desc.ID] = NewBuffer;
		return NewBuffer;
	}

	texture* AllocateTextureInternal(const resource_descriptor& Desc)
	{
		texture* NewTexture = nullptr;
		switch(Gfx->Type)
		{
			case backend_type::vulkan:
				NewTexture = new vulkan_texture(Gfx, Desc.Name, Desc.Data, Desc.Width, Desc.Height, Desc.Depth, Desc.Info);
				break;
#if _WIN32
			case backend_type::directx12:
				NewTexture = new directx12_texture(Gfx, Desc.Name, Desc.Data, Desc.Width, Desc.Height, Desc.Depth, Desc.Info);
				break;
#endif
		}
		Resources[Desc.ID] = NewTexture;
		return NewTexture;
	}


public:
	gpu_memory_heap(renderer_backend* Backend) : Gfx(Backend) {};
	~gpu_memory_heap() 
	{
        for (auto& [ID, Resource] : Resources)
		{
			delete Resource;
        }

        for (auto& [ID, Descriptor] : Descriptors)
		{
			if(Descriptor.Data) delete Descriptor.Data;
			Descriptor.Data = nullptr;
        }

        Resources.clear();
        Descriptors.clear();
        Unused.clear();
	}

	resource_descriptor CreateBuffer(const std::string& Name, void* Data, u64 Size, u64 Count, u32 Usage)
	{
		resource_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Name  = Name;
		Descriptor.Data  = new char[Size * Count];
		Descriptor.Size  = Size;
		Descriptor.Count = Count;
		Descriptor.Usage = Usage;
		Descriptor.Type  = resource_descriptor_type::buffer;
		memcpy(Descriptor.Data, Data, Size * Count);
		Descriptors[Descriptor.ID] = Descriptor;
		return Descriptor;
	}

	resource_descriptor CreateBuffer(const std::string& Name, u64 Size, u64 Count, u32 Usage)
	{
		resource_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Name  = Name;
		Descriptor.Size  = Size;
		Descriptor.Count = Count;
		Descriptor.Usage = Usage;
		Descriptor.Type  = resource_descriptor_type::buffer;
		Descriptors[Descriptor.ID] = Descriptor;
		return Descriptor;
	}

	resource_descriptor CreateTexture(const std::string& Name, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& Info)
	{
		resource_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Name   = Name;
		Descriptor.Data   = new char[Width * Height * Depth * GetPixelSize(Info.Format)];
		Descriptor.Width  = Width;
		Descriptor.Height = Height;
		Descriptor.Depth  = Depth;
		Descriptor.Info   = Info;
		Descriptor.Type   = resource_descriptor_type::texture;
		Descriptors[Descriptor.ID] = Descriptor;
		return Descriptor;
	}

	resource_descriptor CreateTexture(const std::string& Name, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& Info)
	{
		resource_descriptor Descriptor;
		if(!Unused.empty())
		{
			Descriptor.ID = Unused.back();
			Unused.pop_back();
		}
		else
		{
			Descriptor.ID = NextID++;
		}
		Descriptor.Name   = Name;
		Descriptor.Width  = Width;
		Descriptor.Height = Height;
		Descriptor.Depth  = Depth;
		Descriptor.Info   = Info;
		Descriptor.Type   = resource_descriptor_type::texture;
		Descriptors[Descriptor.ID] = Descriptor;
		return Descriptor;
	}

	[[nodiscard]] resource_descriptor GetResourceDescriptor(u64 ID)
	{
		if(Descriptors.find(ID) != Descriptors.end())
			return Descriptors[ID];
		return {};
	}

	// TODO: if the texture is still not created, create it here so that gather would work correctly here
	[[nodiscard]] buffer* GetBuffer(const resource_descriptor& Desc)
	{
		if(Resources.find(Desc.ID) != Resources.end())
		{
			return (buffer*)Resources[Desc.ID];
		}
		return AllocateBufferInternal(Desc);
	}

	[[nodiscard]] buffer* GetBuffer(u64 ID)
	{
		if(Resources.find(ID) != Resources.end())
		{
			return (buffer*)Resources[ID];
		}
		return AllocateBufferInternal(GetResourceDescriptor(ID));
	}

	// TODO: if the texture is still not created, create it here so that gather would work correctly here
	[[nodiscard]] texture* GetTexture(const resource_descriptor& Desc)
	{
		if(Resources.find(Desc.ID) != Resources.end())
		{
			return (texture*)Resources[Desc.ID];
		}
		return AllocateTextureInternal(Desc);
	}

	[[nodiscard]] texture* GetTexture(u64 ID)
	{
		if(Resources.find(ID) != Resources.end())
		{
			return (texture*)Resources[ID];
		}
		return AllocateTextureInternal(GetResourceDescriptor(ID));
	}

	// TODO: if the texture is still not created, create it here so that gather would work correctly here
	[[nodiscard]] std::vector<texture*> GetTexture(const std::vector<resource_descriptor>& Descs)
	{
		std::vector<texture*> Res(Descs.size());
		for(int i = 0; i < Descs.size(); i++)
		{
			resource_descriptor Desc = Descs[i];
			if(Resources.find(Desc.ID) != Resources.end())
			{
				Res[i] = (texture*)Resources[Desc.ID];
			}
			else
			{
				Res[i] = AllocateTextureInternal(Desc);
			}
		}
		return Res;
	}

	void ReleaseBuffer(const resource_descriptor& Desc)
	{
		auto it = Resources.find(Desc.ID);
		if(it != Resources.end())
		{
			Unused.push_back(Desc.ID);
		}
	}

	void ReleaseTexture(const resource_descriptor& Desc)
	{
		auto it = Resources.find(Desc.ID);
		if(it != Resources.end())
		{
			Unused.push_back(Desc.ID);
		}
	}

	void FreeBuffer(const resource_descriptor& Desc)
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

	void FreeTexture(const resource_descriptor& Desc)
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
