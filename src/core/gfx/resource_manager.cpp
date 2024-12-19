
gpu_memory_heap::
gpu_memory_heap(renderer_backend* Backend) : Gfx(Backend) {};

gpu_memory_heap::
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

buffer* gpu_memory_heap::
AllocateBufferInternal(const resource_descriptor& Desc, command_list* CommandList)
{
	buffer* NewBuffer = nullptr;
	switch(Gfx->Type)
	{
		case backend_type::vulkan:
			if(CommandList)
			{
				NewBuffer = new vulkan_buffer(Gfx, Desc.Name, nullptr, Desc.Size, Desc.Count, Desc.Usage);
				NewBuffer->Update(Desc.Data, CommandList);
			}
			else
			{
				NewBuffer = new vulkan_buffer(Gfx, Desc.Name, Desc.Data, Desc.Size, Desc.Count, Desc.Usage);
			}
			break;
#if _WIN32
		case backend_type::directx12:
			if(CommandList)
			{
				NewBuffer = new directx12_buffer(Gfx, Desc.Name, nullptr, Desc.Size, Desc.Count, Desc.Usage);
				NewBuffer->Update(Desc.Data, CommandList);
			}
			else
			{
				NewBuffer = new directx12_buffer(Gfx, Desc.Name, Desc.Data, Desc.Size, Desc.Count, Desc.Usage);
			}
			break;
#endif
	}
	Resources[Desc.ID] = NewBuffer;
	return NewBuffer;
}

texture* gpu_memory_heap::
AllocateTextureInternal(const resource_descriptor& Desc, command_list* CommandList)
{
	texture* NewTexture = nullptr;
	switch(Gfx->Type)
	{
		case backend_type::vulkan:
			if(CommandList)
			{
				NewTexture = new vulkan_texture(Gfx, Desc.Name, nullptr, Desc.Width, Desc.Height, Desc.Depth, Desc.Info);
				NewTexture->Update(Desc.Data, CommandList);
			}
			else
			{
				NewTexture = new vulkan_texture(Gfx, Desc.Name, Desc.Data, Desc.Width, Desc.Height, Desc.Depth, Desc.Info);
			}
			break;
#if _WIN32
		case backend_type::directx12:
			if(CommandList)
			{
				NewTexture = new directx12_texture(Gfx, Desc.Name, nullptr, Desc.Width, Desc.Height, Desc.Depth, Desc.Info);
				NewTexture->Update(Desc.Data, CommandList);
			}
			else
			{
				NewTexture = new directx12_texture(Gfx, Desc.Name, Desc.Data, Desc.Width, Desc.Height, Desc.Depth, Desc.Info);
			}
			break;
#endif
	}
	Resources[Desc.ID] = NewTexture;
	return NewTexture;
}

resource_descriptor gpu_memory_heap::
CreateBuffer(const std::string& Name, void* Data, u64 Size, u64 Count, u32 Usage)
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

resource_descriptor gpu_memory_heap::
CreateBuffer(const std::string& Name, u64 Size, u64 Count, u32 Usage)
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

resource_descriptor gpu_memory_heap::
CreateTexture(const std::string& Name, void* Data, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& Info)
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
	memcpy(Descriptor.Data, Data, Width * Height * Depth * GetPixelSize(Info.Format));
	Descriptors[Descriptor.ID] = Descriptor;
	return Descriptor;
}

resource_descriptor gpu_memory_heap::
CreateTexture(const std::string& Name, u32 Width, u32 Height, u32 Depth, const utils::texture::input_data& Info)
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

[[nodiscard]] resource_descriptor gpu_memory_heap::
GetResourceDescriptor(u64 ID)
{
	if(Descriptors.find(ID) != Descriptors.end())
		return Descriptors[ID];
	return {};
}

// TODO: if the texture is still not created, create it here so that gather would work correctly here
[[nodiscard]] buffer* gpu_memory_heap::
GetBuffer(const resource_descriptor& Desc)
{
	if(Resources.find(Desc.ID) != Resources.end())
	{
		return (buffer*)Resources[Desc.ID];
	}
	return AllocateBufferInternal(Desc);
}

[[nodiscard]] buffer* gpu_memory_heap::
GetBuffer(u64 ID)
{
	if(Resources.find(ID) != Resources.end())
	{
		return (buffer*)Resources[ID];
	}
	return AllocateBufferInternal(GetResourceDescriptor(ID));
}

[[nodiscard]] buffer* gpu_memory_heap::
GetBuffer(command_list* CommandList, const resource_descriptor& Desc)
{
	if(Resources.find(Desc.ID) != Resources.end())
	{
		return (buffer*)Resources[Desc.ID];
	}
	return AllocateBufferInternal(Desc, CommandList);
}

[[nodiscard]] buffer* gpu_memory_heap::
GetBuffer(command_list* CommandList, u64 ID)
{
	if(Resources.find(ID) != Resources.end())
	{
		return (buffer*)Resources[ID];
	}
	return AllocateBufferInternal(GetResourceDescriptor(ID), CommandList);
}

// TODO: if the texture is still not created, create it here so that gather would work correctly here
[[nodiscard]] texture* gpu_memory_heap::
GetTexture(const resource_descriptor& Desc)
{
	if(Resources.find(Desc.ID) != Resources.end())
	{
		return (texture*)Resources[Desc.ID];
	}
	return AllocateTextureInternal(Desc);
}

[[nodiscard]] texture* gpu_memory_heap::
GetTexture(u64 ID)
{
	if(Resources.find(ID) != Resources.end())
	{
		return (texture*)Resources[ID];
	}
	return AllocateTextureInternal(GetResourceDescriptor(ID));
}

[[nodiscard]] texture* gpu_memory_heap::
GetTexture(command_list* CommandList, const resource_descriptor& Desc)
{
	if(Resources.find(Desc.ID) != Resources.end())
	{
		return (texture*)Resources[Desc.ID];
	}
	return AllocateTextureInternal(Desc, CommandList);
}

[[nodiscard]] texture* gpu_memory_heap::
GetTexture(command_list* CommandList, u64 ID)
{
	if(Resources.find(ID) != Resources.end())
	{
		return (texture*)Resources[ID];
	}
	return AllocateTextureInternal(GetResourceDescriptor(ID), CommandList);
}

void gpu_memory_heap::
ReleaseBuffer(const resource_descriptor& Desc)
{
	auto it = Resources.find(Desc.ID);
	if(it != Resources.end())
	{
		Unused.push_back(Desc.ID);
	}
}

void gpu_memory_heap::
ReleaseTexture(const resource_descriptor& Desc)
{
	auto it = Resources.find(Desc.ID);
	if(it != Resources.end())
	{
		Unused.push_back(Desc.ID);
	}
}

void gpu_memory_heap::
FreeBuffer(const resource_descriptor& Desc)
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

void gpu_memory_heap::
FreeTexture(const resource_descriptor& Desc)
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
