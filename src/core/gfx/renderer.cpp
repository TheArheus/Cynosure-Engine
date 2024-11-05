// TODO: CLEAR THE RESOURCES!!!

global_graphics_context::
global_graphics_context(renderer_backend* NewBackend, backend_type NewBackendType)
	: Backend(NewBackend), BackendType(NewBackendType)
{
	ExecutionContext = CreateGlobalPipelineContext();

	utils::texture::input_data TextureInputData = {};
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled | TF_CopySrc;
	GfxColorTarget[0] = PushTexture("ColorTarget0", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	GfxColorTarget[1] = PushTexture("ColorTarget1", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	TextureInputData.Format    = image_format::D32_SFLOAT;
	TextureInputData.Usage     = TF_DepthTexture | TF_Sampled;
	GfxDepthTarget = PushTexture("DepthTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	DebugCameraViewDepthTarget = PushTexture("DebugCameraViewDepthTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);

	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled | TF_CopySrc;
	VolumetricLightOut = PushTexture("Volumetric light Result", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	IndirectLightOut   = PushTexture("Indirect light result"  , nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	LightColor         = PushTexture("Light color"  , nullptr, Backend->Width, Backend->Height, 1, TextureInputData);

	TextureInputData.Type	   = image_type::Texture3D;
	TextureInputData.Format    = image_format::R8G8B8A8_UNORM;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled | TF_CopySrc;
	TextureInputData.MipLevels = GetImageMipLevels(VOXEL_SIZE, VOXEL_SIZE);
	TextureInputData.SamplerInfo.AddressMode = sampler_address_mode::clamp_to_border;
	TextureInputData.UseStagingBuffer = true;
	VoxelGridTarget   = PushTexture("VoxelGridTarget", nullptr, VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE, TextureInputData);

	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.Format    = image_format::R11G11B10_SFLOAT;
	TextureInputData.MipLevels = 1;
	TextureInputData.UseStagingBuffer = false;
	HdrColorTarget = PushTexture("HdrColorTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	TextureInputData.MipLevels = 6;
	TextureInputData.SamplerInfo.AddressMode = sampler_address_mode::clamp_to_border;
	BrightTarget   = PushTexture("BrightTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	TempBrTarget   = PushTexture("TempBrTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);

	vec2 PoissonDisk[64] = {};
	PoissonDisk[0]  = vec2(-0.613392, 0.617481);
	PoissonDisk[1]  = vec2(0.170019, -0.040254);
	PoissonDisk[2]  = vec2(-0.299417, 0.791925);
	PoissonDisk[3]  = vec2(0.645680, 0.493210);
	PoissonDisk[4]  = vec2(-0.651784, 0.717887);
	PoissonDisk[5]  = vec2(0.421003, 0.027070);
	PoissonDisk[6]  = vec2(-0.817194, -0.271096);
	PoissonDisk[7]  = vec2(-0.705374, -0.668203);
	PoissonDisk[8]  = vec2(0.977050, -0.108615);
	PoissonDisk[9]  = vec2(0.063326, 0.142369);
	PoissonDisk[10] = vec2(0.203528, 0.214331);
	PoissonDisk[11] = vec2(-0.667531, 0.326090);
	PoissonDisk[12] = vec2(-0.098422, -0.295755);
	PoissonDisk[13] = vec2(-0.885922, 0.215369);
	PoissonDisk[14] = vec2(0.566637, 0.605213);
	PoissonDisk[15] = vec2(0.039766, -0.396100);
	PoissonDisk[16] = vec2(0.751946, 0.453352);
	PoissonDisk[17] = vec2(0.078707, -0.715323);
	PoissonDisk[18] = vec2(-0.075838, -0.529344);
	PoissonDisk[19] = vec2(0.724479, -0.580798);
	PoissonDisk[20] = vec2(0.222999, -0.215125);
	PoissonDisk[21] = vec2(-0.467574, -0.405438);
	PoissonDisk[22] = vec2(-0.248268, -0.814753);
	PoissonDisk[23] = vec2(0.354411, -0.887570);
	PoissonDisk[24] = vec2(0.175817, 0.382366);
	PoissonDisk[25] = vec2(0.487472, -0.063082);
	PoissonDisk[26] = vec2(-0.084078, 0.898312);
	PoissonDisk[27] = vec2(0.488876, -0.783441);
	PoissonDisk[28] = vec2(0.470016, 0.217933);
	PoissonDisk[29] = vec2(-0.696890, -0.549791);
	PoissonDisk[30] = vec2(-0.149693, 0.605762);
	PoissonDisk[31] = vec2(0.034211, 0.979980);
	PoissonDisk[32] = vec2(0.503098, -0.308878);
	PoissonDisk[33] = vec2(-0.016205, -0.872921);
	PoissonDisk[34] = vec2(0.385784, -0.393902);
	PoissonDisk[35] = vec2(-0.146886, -0.859249);
	PoissonDisk[36] = vec2(0.643361, 0.164098);
	PoissonDisk[37] = vec2(0.634388, -0.049471);
	PoissonDisk[38] = vec2(-0.688894, 0.007843);
	PoissonDisk[39] = vec2(0.464034, -0.188818);
	PoissonDisk[40] = vec2(-0.440840, 0.137486);
	PoissonDisk[41] = vec2(0.364483, 0.511704);
	PoissonDisk[42] = vec2(0.034028, 0.325968);
	PoissonDisk[43] = vec2(0.099094, -0.308023);
	PoissonDisk[44] = vec2(0.693960, -0.366253);
	PoissonDisk[45] = vec2(0.678884, -0.204688);
	PoissonDisk[46] = vec2(0.001801, 0.780328);
	PoissonDisk[47] = vec2(0.145177, -0.898984);
	PoissonDisk[48] = vec2(0.062655, -0.611866);
	PoissonDisk[49] = vec2(0.315226, -0.604297);
	PoissonDisk[50] = vec2(-0.780145, 0.486251);
	PoissonDisk[51] = vec2(-0.371868, 0.882138);
	PoissonDisk[52] = vec2(0.200476, 0.494430);
	PoissonDisk[53] = vec2(-0.494552, -0.711051);
	PoissonDisk[54] = vec2(0.612476, 0.705252);
	PoissonDisk[55] = vec2(-0.578845, -0.768792);
	PoissonDisk[56] = vec2(-0.772454, -0.090976);
	PoissonDisk[57] = vec2(0.504440, 0.372295);
	PoissonDisk[58] = vec2(0.155736, 0.065157);
	PoissonDisk[59] = vec2(0.391522, 0.849605);
	PoissonDisk[60] = vec2(-0.620106, -0.328104);
	PoissonDisk[61] = vec2(0.789239, -0.419965);
	PoissonDisk[62] = vec2(-0.545396, 0.538133);
	PoissonDisk[63] = vec2(-0.178564, -0.596057);
	PoissonDiskBuffer = PushBuffer("PoissonDiskBuffer", PoissonDisk, sizeof(vec2), 64, RF_StorageBuffer);

	const u32 Res = 32;
	vec4  RandomAngles[Res][Res][Res] = {};
	for(u32 x = 0; x < Res; x++)
	{
		for(u32 y = 0; y < Res; y++)
		{
			for(u32 z = 0; z < Res; z++)
			{
				RandomAngles[x][y][z] = vec4(float(rand()) / RAND_MAX * 2.0f / Pi<float>,
											 float(rand()) / RAND_MAX * 2.0f / Pi<float>,
											 float(rand()) / RAND_MAX * 2.0f / Pi<float>, 1);
			}
		}
	}
	vec2 RandomRotations[16] = {};
	for(u32 RotIdx = 0; RotIdx < 16; ++RotIdx)
	{
		RandomRotations[RotIdx] = vec2(float(rand()) / RAND_MAX * 2.0 - 1.0, float(rand()) / RAND_MAX * 2.0 - 1.0);
	}
	TextureInputData = {};
	TextureInputData.Format    = image_format::R32G32B32A32_SFLOAT;
	TextureInputData.Usage     = TF_Storage | TF_Sampled | TF_CopyDst | TF_ColorTexture;
	TextureInputData.Type	   = image_type::Texture3D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	RandomAnglesTexture = PushTexture("RandomAnglesTexture", (void*)RandomAngles, 32, 32, 32, TextureInputData);
	TextureInputData.Format    = image_format::R32G32_SFLOAT;
	TextureInputData.Type	   = image_type::Texture2D;
	NoiseTexture = PushTexture("NoiseTexture", (void*)RandomRotations, 4, 4, 1, TextureInputData);

	vec4 RandomSamples[64] = {};
	for(u32 RotIdx = 0; RotIdx < 64; ++RotIdx)
	{
		vec3 Sample = vec3(float(rand()) / RAND_MAX * 2.0f - 1.0,
						   float(rand()) / RAND_MAX * 2.0f - 1.0,
						   float(rand()) / RAND_MAX).Normalize();
		Sample = Sample *  float(rand()) / RAND_MAX;
		float Scale = RotIdx / 64.0;
		Scale = Lerp(0.1, Scale, 1.0);
		RandomSamples[RotIdx] = vec4(Sample * Scale, 0);
	}
	RandomSamplesBuffer = PushBuffer("RandomSamplesBuffer", (void*)RandomSamples, sizeof(vec4), 64, RF_StorageBuffer);

	GlobalShadow.resize(DEPTH_CASCADES_COUNT);
	TextureInputData.Format    = image_format::D32_SFLOAT;
	TextureInputData.Usage     = TF_DepthTexture | TF_Sampled;
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; CascadeIdx++)
	{
		GlobalShadow[CascadeIdx] = PushTexture("GlobalShadow" + std::to_string(CascadeIdx), nullptr, PreviousPowerOfTwo(Backend->Width) * 2, PreviousPowerOfTwo(Backend->Width) * 2, 1, TextureInputData);
	}

	TextureInputData.Format    = image_format::R32_SFLOAT;
	TextureInputData.Usage     = TF_Sampled | TF_Storage | TF_CopySrc | TF_ColorTexture;
	TextureInputData.MipLevels = GetImageMipLevels(PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height));
	TextureInputData.SamplerInfo.ReductionMode = sampler_reduction_mode::max;
	TextureInputData.SamplerInfo.MinFilter = filter::nearest;
	TextureInputData.SamplerInfo.MagFilter = filter::nearest;
	TextureInputData.SamplerInfo.MipmapMode = mipmap_mode::nearest;
	DepthPyramid = PushTexture("DepthPyramid", PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height), 1, TextureInputData);

	GBuffer.resize(GBUFFER_COUNT);
	TextureInputData = {};
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Usage     = TF_ColorAttachment | TF_Sampled | TF_Storage | TF_CopySrc;
	TextureInputData.Format    = image_format::R16G16B16A16_SNORM;
	GBuffer[0] = PushTexture("GBuffer_VertexNormals",  nullptr, Backend->Width, Backend->Height, 1, TextureInputData);   // Vertex   Normals
	GBuffer[1] = PushTexture("GBuffer_FragmentNormals",  nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Fragment Normals
	TextureInputData.Format    = image_format::R8G8B8A8_UNORM;
	GBuffer[2] = PushTexture("GBuffer_Diffuse",  nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Diffuse Color
	GBuffer[3] = PushTexture("GBuffer_Emmit",    nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Emmit Color
	TextureInputData.Format    = image_format::R32G32_SFLOAT;
	GBuffer[4] = PushTexture("GBuffer_Specular", nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Specular + Light Emmission Ammount
	AmbientOcclusionData = PushTexture("AmbientOcclusionData", Backend->Width, Backend->Height, 1, TextureInputData);
	BlurTemp			 = PushTexture("BlurTemp", Backend->Width, Backend->Height, 1, TextureInputData);
}

global_graphics_context::
global_graphics_context(global_graphics_context&& Oth) noexcept : 
	Backend(std::move(Oth.Backend)),
	BackendType(std::move(Oth.BackendType)),
	PoissonDiskBuffer(std::move(Oth.PoissonDiskBuffer)),
	RandomSamplesBuffer(std::move(Oth.RandomSamplesBuffer)),
	GfxDepthTarget(std::move(Oth.GfxDepthTarget)),
	DebugCameraViewDepthTarget(std::move(Oth.DebugCameraViewDepthTarget)),
	GlobalShadow(std::move(Oth.GlobalShadow)),
	GBuffer(std::move(Oth.GBuffer)),
	AmbientOcclusionData(std::move(Oth.AmbientOcclusionData)),
	BlurTemp(std::move(Oth.BlurTemp)),
	DepthPyramid(std::move(Oth.DepthPyramid)),
	RandomAnglesTexture(std::move(Oth.RandomAnglesTexture)),
	NoiseTexture(std::move(Oth.NoiseTexture))
{
	GfxColorTarget[0] = std::move(Oth.GfxColorTarget[0]);
	GfxColorTarget[1] = std::move(Oth.GfxColorTarget[1]);
	VoxelGridTarget = std::move(Oth.VoxelGridTarget);
	HdrColorTarget = std::move(Oth.HdrColorTarget);
	BrightTarget = std::move(Oth.BrightTarget);
	TempBrTarget = std::move(Oth.TempBrTarget);

	VolumetricLightOut = std::move(Oth.VolumetricLightOut);
	IndirectLightOut = std::move(Oth.IndirectLightOut);
	LightColor = std::move(Oth.LightColor);

	ExecutionContext = std::move(Oth.ExecutionContext);
	ContextMap = std::move(Oth.ContextMap);
	GeneralShaderViewMap = std::move(Oth.GeneralShaderViewMap);
	Dispatches = std::move(Oth.Dispatches);
}

global_graphics_context& global_graphics_context::
operator=(global_graphics_context&& Oth) noexcept
{
	if(this != &Oth)
	{
		Backend = std::move(Oth.Backend);
		BackendType = std::move(Oth.BackendType);
		PoissonDiskBuffer = std::move(Oth.PoissonDiskBuffer);
		RandomSamplesBuffer = std::move(Oth.RandomSamplesBuffer);
		GfxColorTarget[0] = std::move(Oth.GfxColorTarget[0]);
		GfxColorTarget[1] = std::move(Oth.GfxColorTarget[1]);
		GfxDepthTarget = std::move(Oth.GfxDepthTarget);
		VoxelGridTarget = std::move(Oth.VoxelGridTarget);
		HdrColorTarget = std::move(Oth.HdrColorTarget);
		BrightTarget = std::move(Oth.BrightTarget);
		TempBrTarget = std::move(Oth.TempBrTarget);
		DebugCameraViewDepthTarget = std::move(Oth.DebugCameraViewDepthTarget);
		GlobalShadow = std::move(Oth.GlobalShadow);
		GBuffer = std::move(Oth.GBuffer);
		AmbientOcclusionData = std::move(Oth.AmbientOcclusionData);
		BlurTemp = std::move(Oth.BlurTemp);
		DepthPyramid = std::move(Oth.DepthPyramid);
		RandomAnglesTexture = std::move(Oth.RandomAnglesTexture);
		NoiseTexture = std::move(Oth.NoiseTexture);

		VolumetricLightOut = std::move(Oth.VolumetricLightOut);
		IndirectLightOut = std::move(Oth.IndirectLightOut);
		LightColor = std::move(Oth.LightColor);

		ExecutionContext = std::move(Oth.ExecutionContext);
		ContextMap = std::move(Oth.ContextMap);
		GeneralShaderViewMap = std::move(Oth.GeneralShaderViewMap);
		Dispatches = std::move(Oth.Dispatches);
	}

	return *this;
}

general_context* global_graphics_context::
GetOrCreateContext(shader_pass* Pass)
{
    std::type_index ContextType = PassToContext.at(Pass);
    
    if (auto it = ContextMap.find(ContextType); it != ContextMap.end())
    {
        return it->second;
    }

    shader_view_context* ContextView = GeneralShaderViewMap[ContextType];
    general_context* NewContext = nullptr;

    if (Pass->Type == pass_type::graphics)
    {
        auto* GraphicsContextView = static_cast<shader_graphics_view_context*>(ContextView);
        NewContext = CreateRenderContext(
            GraphicsContextView->LoadOp,
            GraphicsContextView->StoreOp,
            GraphicsContextView->Shaders,
            GraphicsContextView->SetupAttachmentDescription(),
            GraphicsContextView->SetupPipelineState(),
            GraphicsContextView->Defines
        );
		ContextMap[ContextType] = NewContext;
    }
    else if (Pass->Type == pass_type::compute)
    {
        auto* ComputeContextView = static_cast<shader_compute_view_context*>(ContextView);
        NewContext = CreateComputeContext(ComputeContextView->Shader, ComputeContextView->Defines);
		ContextMap[ContextType] = NewContext;
    }

    return NewContext;
}

void global_graphics_context::
SetContext(shader_pass* Pass, command_list* Context)
{
    CurrentContext = GetOrCreateContext(Pass);
    if (Pass->Type == pass_type::graphics)
    {
        Context->SetGraphicsPipelineState(static_cast<render_context*>(CurrentContext));
    }
    else if (Pass->Type == pass_type::compute)
    {
        Context->SetComputePipelineState(static_cast<compute_context*>(CurrentContext));
    }
}

template<typename context_type, typename param_type>
shader_pass* global_graphics_context::
AddPass(std::string Name, param_type Parameters, pass_type Type, execute_func Exec)
{
	shader_pass* NewPass = PushStructConstruct(shader_pass);
	NewPass->Name = Name;
	NewPass->Type = Type;
	NewPass->HaveStaticStorage = has_static_storage_type<context_type>::value;
	
	meta_descriptor* ReflectionData = reflect<context_type>::Get();
	ParseShaderParam(ReflectionData, (void*)&Parameters);

	NewPass->Parameters = PushStructConstruct(param_type);
	*((param_type*)NewPass->Parameters) = Parameters;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	PassToContext.emplace(NewPass, std::type_index(typeid(context_type)));

	auto FindIt = ContextMap.find(std::type_index(typeid(context_type)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(context_type))] = PushStructConstruct(context_type);
	}

	return NewPass;
}

shader_pass* global_graphics_context::
AddTransferPass(std::string Name, execute_func Exec)
{
	shader_pass* NewPass = PushStructConstruct(shader_pass);
	NewPass->Name = Name;
	NewPass->Type = pass_type::transfer;
	NewPass->Parameters = nullptr;

	Passes.push_back(NewPass);
	Dispatches[NewPass] = Exec;
	PassToContext.emplace(NewPass, std::type_index(typeid(transfer)));

	auto FindIt = ContextMap.find(std::type_index(typeid(transfer)));

	if(FindIt == ContextMap.end())
	{
		GeneralShaderViewMap[std::type_index(typeid(transfer))] = PushStructConstruct(transfer);
	}

	return NewPass;
}

void global_graphics_context::
ParseShaderParam(meta_descriptor* Descriptor, void* Parameters)
{
	void* ParametersToParse = Parameters;
	for(int ParamIdx = 0; ParamIdx < Descriptor->Size; ParamIdx++)
	{
		member_definition* Member = Descriptor->Data + ParamIdx;
		void* Data = (void*)((uptr)Parameters + Member->Offset);
		if(Member->Type == meta_type::buffer_ref)
		{
		}
		else if(Member->Type == meta_type::texture_ref)
		{
		}
		else
		{
		}
	}
}

// TODO: Prepass and postpass barriers (postpass barrier is for when the resource is not fully in the same state, for example, after mip generation etc)
// TODO: Command parallelization maybe
void global_graphics_context::
Compile()
{
	resource_binder* Binder = CreateResourceBinder();
	for(shader_pass* Pass : Passes)
	{
		if(Pass->Type == pass_type::transfer) continue;

		general_context* UseContext = GetOrCreateContext(Pass);

		if(Pass->HaveStaticStorage)
		{
			Binder->AppendStaticStorage(UseContext, Pass->Parameters);
		}
	}
	Binder->BindStaticStorage(Backend);
}

// TODO: Consider of pushing pass work in different thread if some of them are not dependent and can use different queue type
void global_graphics_context::
Execute(scene_manager& SceneManager)
{
	ExecutionContext->AcquireNextImage();
	ExecutionContext->Begin();
	for(shader_pass* Pass : Passes)
	{
		SetContext(Pass, ExecutionContext);
		Dispatches[Pass](ExecutionContext, Pass->Parameters);
	}

	ExecutionContext->DebugGuiBegin(GfxColorTarget[BackBufferIndex]);
	ImGui::NewFrame();

	SceneManager.RenderUI();

	if(SceneManager.Count() > 1)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 300));
		ImGui::SetNextWindowSize(ImVec2(150, 100));
		ImGui::Begin("Active Scenes", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		for(u32 SceneIdx = 0; SceneIdx < SceneManager.Count(); ++SceneIdx)
		{
			if(ImGui::Button(SceneManager.Infos[SceneIdx].Name.c_str()))
			{
				SceneManager.CurrentScene = SceneIdx;
			}
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ExecutionContext->DebugGuiEnd();

	ExecutionContext->EmplaceColorTarget(GfxColorTarget[BackBufferIndex]);
	ExecutionContext->Present();

	for(shader_pass* Pass : Passes)
	{
		if(Pass->Type == pass_type::transfer) continue;
		ContextMap[PassToContext.at(Pass)]->Clear();
	}

	Passes.clear();
	Dispatches.clear();
	PassToContext.clear();

	BackBufferIndex = (BackBufferIndex + 1) % 2;
}
