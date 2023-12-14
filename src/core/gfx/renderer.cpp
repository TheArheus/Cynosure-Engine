
global_graphics_context::
global_graphics_context(renderer_backend* NewBackend)
	: Backend(NewBackend)
{
	GlobalHeap.CreateResource(Backend);
	CommandQueue.Init(Backend);

	vulkan_backend* VulkanBackend = static_cast<vulkan_backend*>(Backend);

	ImGui_ImplVulkan_InitInfo InitInfo = {};
	InitInfo.Instance = VulkanBackend->Instance;
	InitInfo.PhysicalDevice = VulkanBackend->PhysicalDevice;
	InitInfo.Device = VulkanBackend->Device;
	InitInfo.DescriptorPool = VulkanBackend->ImGuiPool;
	InitInfo.QueueFamily = VulkanBackend->FamilyIndex;
	InitInfo.Queue = CommandQueue.Handle;
	InitInfo.MinImageCount = 2;
	InitInfo.ImageCount = VulkanBackend->SwapchainImages.size();
	InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT; //MsaaQuality;
	InitInfo.UseDynamicRendering = true;
	InitInfo.ColorAttachmentFormat = VulkanBackend->SurfaceFormat.format;
	ImGui_ImplVulkan_Init(&InitInfo, VK_NULL_HANDLE);

	ImGui_ImplVulkan_CreateFontsTexture();

	texture::input_data TextureInputData = {};
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = VulkanBackend->SurfaceFormat.format;
	TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	GfxColorTarget = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	GfxDepthTarget = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	DebugCameraViewDepthTarget = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData);

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
	PoissonDiskBuffer = PushBuffer(PoissonDisk, sizeof(vec2) * 64, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

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
	TextureInputData.Format    = VK_FORMAT_R32G32B32A32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_3D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_3D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	RandomAnglesTexture = PushTexture((void*)RandomAngles, 32, 32, 32, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	NoiseTexture = PushTexture((void*)RandomRotations, 4, 4, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	vec4 RandomSamples[64] = {};
	for(u32 RotIdx = 0; RotIdx < 64; ++RotIdx)
	{
		vec3 Sample = vec3(float(rand()) / RAND_MAX * 2.0f - 1.0,
						   float(rand()) / RAND_MAX * 2.0f - 1.0,
						   float(rand()) / RAND_MAX).Normalize();
		Sample = Sample *  float(rand());
		float Scale = RotIdx / 64.0;
		Scale = Lerp(0.1, Scale, 1.0);
		RandomSamples[RotIdx] = vec4(Sample * Scale, 0);
	}
	RandomSamplesBuffer = PushBuffer((void*)RandomSamples, sizeof(vec4) * 64, false, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	GlobalShadow.resize(DEPTH_CASCADES_COUNT);
	TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	for(texture& Shadow : GlobalShadow)
	{
		Shadow = PushTexture(nullptr, PreviousPowerOfTwo(Backend->Width) * 2, PreviousPowerOfTwo(Backend->Width) * 2, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	}

	TextureInputData.Format    = VK_FORMAT_R32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	TextureInputData.MipLevels = GetImageMipLevels(PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height));
	DepthPyramid = PushTexture(PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height), 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_MAX);

	GBuffer.resize(GBUFFER_COUNT);
	TextureInputData = {};
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = VulkanBackend->SurfaceFormat.format;
	TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SFLOAT;
	GBuffer[0] = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Vertex Positions
	TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SNORM;
	GBuffer[1] = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Vertex Normals
	GBuffer[2] = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Fragment Normals
	TextureInputData.Format    = VK_FORMAT_R8G8B8A8_UNORM;
	GBuffer[3] = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Diffuse Color
	TextureInputData.Format    = VK_FORMAT_R32_SFLOAT;
	GBuffer[4] = PushTexture(nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Specular
	AmbientOcclusionData = PushTexture(Backend->Width, Backend->Height, 1, TextureInputData);

	shader_input* GfxRootSignature = new shader_input;
	GfxRootSignature->PushUniformBuffer()->				// World Update Buffer
					 PushStorageBuffer()->				// Vertex Data
					 PushStorageBuffer()->				// Mesh Draw Commands
					 PushStorageBuffer()->				// Mesh Materials
					 // TODO: Update ammount of image samplers in the frame maybe???
					 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Diffuse Texture
					 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Normal Map Texture
					 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Specular Map Texture
					 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Height Map Texture
					 Build(Backend, 0, true)->
					 Build(Backend, 1)->
					 BuildAll(Backend);

	shader_input* ColorPassRootSignature = new shader_input;
	ColorPassRootSignature->PushUniformBuffer()->					// World Update Buffer
						   PushUniformBuffer()->					// Array of Light Sources
						   PushStorageBuffer()->					// Poisson Disk
						   PushImageSampler()->						// Random Rotations
						   PushImageSampler(GBUFFER_COUNT)->		// G-Buffer Vertex Position Data
																	// G-Buffer Vertex Normal Data
																	// G-Buffer Fragment Normal Data
																	// G-Buffer Diffuse Data
																	// G-Buffer Specular Data
						   PushImageSampler()->						// Ambient Occlusion Data
						   PushImageSampler(LIGHT_SOURCES_MAX_COUNT, 1, true)->	// Spot Directional Light Shadow Maps
						   PushImageSampler(LIGHT_SOURCES_MAX_COUNT, 1, true)->	// Point Light Shadow Maps
						   PushStorageImage()->						// Color Target Texture
						   PushImageSampler(DEPTH_CASCADES_COUNT)->	// Shadow Cascades
						   Build(Backend, 0, true)->
						   Build(Backend, 1)->
						   BuildAll(Backend);

	shader_input* AmbientOcclusionRootSignature = new shader_input;
	AmbientOcclusionRootSignature->PushUniformBuffer()->					// World Update Buffer
								  PushStorageBuffer()->					// Poisson Disk
								  PushImageSampler()->					// Random Rotations
								  PushImageSampler(GBUFFER_COUNT)->		// G-Buffer Vertex Position Data
																		// G-Buffer Vertex Normal Data
																		// G-Buffer Fragment Normal Data
																		// G-Buffer Diffuse Data
																		// G-Buffer Specular Data
								  PushStorageImage()->					// Ambient Occlusion Target Texture
								  Build(Backend, 0, true)->
								  BuildAll(Backend);

	shader_input* ShadowSignature = new shader_input;
	ShadowSignature->PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
					PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
					PushConstant(sizeof(mat4))->
					Build(Backend, 0, true)->
					BuildAll(Backend);

	shader_input* PointShadowSignature = new shader_input;
	PointShadowSignature->PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
						 PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
						 PushConstant(sizeof(point_shadow_input))->
						 Build(Backend, 0, true)->
						 BuildAll(Backend);


	shader_input* CmpIndirectFrustRootSignature = new shader_input;
	CmpIndirectFrustRootSignature->
					 PushUniformBuffer()->				// Mesh Common Culling Input Buffer
					 PushStorageBuffer()->				// Mesh Offsets
					 PushStorageBuffer()->				// Draw Command Input
					 PushStorageBuffer()->				// Draw Command Visibility
					 PushStorageBuffer()->				// Indirect Draw Indexed Command
					 PushStorageBuffer()->				// Indirect Draw Indexed Command Counter
					 PushStorageBuffer()->				// Mesh Draw Command Data
					 Build(Backend, 0, true)->
					 BuildAll(Backend);

	shader_input* ShadowComputeRootSignature = new shader_input;
	ShadowComputeRootSignature->PushUniformBuffer()->	// Mesh Common Culling Input
							  PushStorageBuffer()->		// Mesh Offsets
							  PushStorageBuffer()->		// Draw Command Input
							  PushStorageBuffer()->		// Draw Command Visibility
							  PushStorageBuffer()->		// Indirect Draw Indexed Command
							  PushStorageBuffer()->		// Indirect Draw Indexed Command Counter
							  PushStorageBuffer()->		// Draw Commands
							  Build(Backend, 0, true)->
							  BuildAll(Backend);

	shader_input* CmpIndirectOcclRootSignature = new shader_input;
	CmpIndirectOcclRootSignature->
					 PushUniformBuffer()->				// Mesh Common Culling Input Buffer
					 PushStorageBuffer()->				// Mesh Offsets
					 PushStorageBuffer()->				// Draw Command Input
					 PushStorageBuffer()->				// Draw Command Visibility
					 PushImageSampler()->				// Hierarchy-Z depth texture
					 Build(Backend, 0, true)->
					 BuildAll(Backend);

	shader_input* CmpReduceRootSignature = new shader_input;
	CmpReduceRootSignature->PushImageSampler(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->		// Input  Texture
						   PushStorageImage(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->		// Output Texture
						   PushConstant(sizeof(vec2), VK_SHADER_STAGE_COMPUTE_BIT)->	// Output Texture Size
						   Build(Backend, 0, true)->
						   BuildAll(Backend);

	shader_input* BlurRootSignature = new shader_input;
	BlurRootSignature->PushImageSampler(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->			// Input  Texture
					  PushStorageImage(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->			// Output Texture
					  PushConstant(sizeof(vec3), VK_SHADER_STAGE_COMPUTE_BIT)->		// Texture Size, Conv Size 
					  Build(Backend, 0, true)->
					  BuildAll(Backend);

	shader_input* DebugRootSignature = new shader_input;
	DebugRootSignature->PushUniformBuffer()-> // Global World Update 
					   PushStorageBuffer()-> // Vertex Data
					   PushStorageBuffer()-> // Mesh Draw Commands
					   PushStorageBuffer()-> // Mesh Materials
					   Build(Backend, 0, true)->
					   BuildAll(Backend);

	shader_input* DebugComputeRootSignature = new shader_input;
	DebugComputeRootSignature->PushUniformBuffer()->		// Mesh Common Culling Input
							  PushStorageBuffer()->		// Mesh Offsets
							  PushStorageBuffer()->		// Draw Command Input
							  PushStorageBuffer()->		// Draw Command Visibility
							  PushStorageBuffer()->		// Indirect Draw Indexed Command
							  PushStorageBuffer()->		// Indirect Draw Indexed Command Counter
							  PushStorageBuffer()->		// Draw Commands
							  Build(Backend, 0, true)->
							  BuildAll(Backend);

	render_context::input_data RendererInputData = {};
	RendererInputData.UseColor	  = true;
	RendererInputData.UseDepth	  = true;
	RendererInputData.UseBackFace = true;
	RendererInputData.UseOutline  = true;
	DebugContext = render_context(Backend, DebugRootSignature, {"..\\build\\shaders\\mesh.dbg.vert.spv", "..\\build\\shaders\\mesh.dbg.frag.spv"}, {VulkanBackend->SurfaceFormat.format}, RendererInputData);
	DebugComputeContext = compute_context(Backend, DebugComputeRootSignature, "..\\build\\shaders\\mesh.dbg.comp.spv");

	std::vector<VkFormat> GfxFormats;
	for(u32 FormatIdx = 0; FormatIdx < GBuffer.size(); ++FormatIdx) GfxFormats.push_back(GBuffer[FormatIdx].Info.Format);
	GfxContext = render_context(Backend, GfxRootSignature, {"..\\build\\shaders\\mesh.vert.spv", "..\\build\\shaders\\mesh.frag.spv"}, GfxFormats);

	RendererInputData = {};
	RendererInputData.UseDepth = true;
	RendererInputData.UseBackFace  = true;
	CascadeShadowContext = render_context(Backend, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);
	ShadowContext = render_context(Backend, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);

	RendererInputData.UseMultiview = true;
	for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
	{
		RendererInputData.ViewMask = 1 << CubeMapFaceIdx;
		CubeMapShadowContexts.push_back(render_context(Backend, PointShadowSignature, {"..\\build\\shaders\\mesh.pnt.sdw.vert.spv", "..\\build\\shaders\\mesh.pnt.sdw.frag.spv"}, {}, RendererInputData));
	}

	RendererInputData.UseDepth	   = true;
	RendererInputData.UseMultiview = false;
	RendererInputData.ViewMask	   = 0;
	DebugCameraViewContext = render_context(Backend, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);

	ColorPassContext = compute_context (Backend, ColorPassRootSignature, "..\\build\\shaders\\color_pass.comp.spv");
	AmbientOcclusionContext = compute_context (Backend, AmbientOcclusionRootSignature, "..\\build\\shaders\\screen_space_ambient_occlusion.comp.spv");
	ShadowComputeContext = compute_context(Backend, ShadowComputeRootSignature, "..\\build\\shaders\\mesh.dbg.comp.spv");
	FrustCullingContext  = compute_context(Backend, CmpIndirectFrustRootSignature, "..\\build\\shaders\\indirect_cull_frust.comp.spv");
	OcclCullingContext   = compute_context(Backend, CmpIndirectOcclRootSignature, "..\\build\\shaders\\indirect_cull_occl.comp.spv");
	DepthReduceContext   = compute_context(Backend, CmpReduceRootSignature, "..\\build\\shaders\\depth_reduce.comp.spv");
	BlurContext = compute_context (Backend, BlurRootSignature, "..\\build\\shaders\\blur.comp.spv");
}

global_graphics_context::
global_graphics_context(global_graphics_context&& Oth) noexcept : 
	Backend(std::move(Oth.Backend)),
	GlobalHeap(std::move(Oth.GlobalHeap)),
	CommandQueue(std::move(Oth.CommandQueue)),
	PoissonDiskBuffer(std::move(Oth.PoissonDiskBuffer)),
	RandomSamplesBuffer(std::move(Oth.RandomSamplesBuffer)),
	GfxColorTarget(std::move(Oth.GfxColorTarget)),
	GfxDepthTarget(std::move(Oth.GfxDepthTarget)),
	DebugCameraViewDepthTarget(std::move(Oth.DebugCameraViewDepthTarget)),
	GlobalShadow(std::move(Oth.GlobalShadow)),
	GBuffer(std::move(Oth.GBuffer)),
	AmbientOcclusionData(std::move(Oth.AmbientOcclusionData)),
	DepthPyramid(std::move(Oth.DepthPyramid)),
	RandomAnglesTexture(std::move(Oth.RandomAnglesTexture)),
	NoiseTexture(std::move(Oth.NoiseTexture)),
	CubeMapShadowContexts(std::move(Oth.CubeMapShadowContexts)),
	GfxContext(std::move(Oth.GfxContext)),
	CascadeShadowContext(std::move(Oth.CascadeShadowContext)),
	ShadowContext(std::move(Oth.ShadowContext)),
	DebugCameraViewContext(std::move(Oth.DebugCameraViewContext)),
	DebugContext(std::move(Oth.DebugContext)),
	ColorPassContext(std::move(Oth.ColorPassContext)),
	AmbientOcclusionContext(std::move(Oth.AmbientOcclusionContext)),
	ShadowComputeContext(std::move(Oth.ShadowComputeContext)),
	FrustCullingContext(std::move(Oth.FrustCullingContext)),
	OcclCullingContext(std::move(Oth.OcclCullingContext)),
	DepthReduceContext(std::move(Oth.DepthReduceContext)),
	BlurContext(std::move(Oth.BlurContext)),
	DebugComputeContext(std::move(Oth.DebugComputeContext))
{}

global_graphics_context& global_graphics_context::
operator=(global_graphics_context&& Oth) noexcept
{
	if(this != &Oth)
	{
		Backend = std::move(Oth.Backend);
		GlobalHeap = std::move(Oth.GlobalHeap);
		CommandQueue = std::move(Oth.CommandQueue);
		PoissonDiskBuffer = std::move(Oth.PoissonDiskBuffer);
		RandomSamplesBuffer = std::move(Oth.RandomSamplesBuffer);
		GfxColorTarget = std::move(Oth.GfxColorTarget);
		GfxDepthTarget = std::move(Oth.GfxDepthTarget);
		DebugCameraViewDepthTarget = std::move(Oth.DebugCameraViewDepthTarget);
		GlobalShadow = std::move(Oth.GlobalShadow);
		GBuffer = std::move(Oth.GBuffer);
		AmbientOcclusionData = std::move(Oth.AmbientOcclusionData);
		DepthPyramid = std::move(Oth.DepthPyramid);
		RandomAnglesTexture = std::move(Oth.RandomAnglesTexture);
		NoiseTexture = std::move(Oth.NoiseTexture);
		CubeMapShadowContexts = std::move(Oth.CubeMapShadowContexts);
		GfxContext = std::move(Oth.GfxContext);
		CascadeShadowContext = std::move(Oth.CascadeShadowContext);
		ShadowContext = std::move(Oth.ShadowContext);
		DebugCameraViewContext = std::move(Oth.DebugCameraViewContext);
		DebugContext = std::move(Oth.DebugContext);
		ColorPassContext = std::move(Oth.ColorPassContext);
		AmbientOcclusionContext = std::move(Oth.AmbientOcclusionContext);
		ShadowComputeContext = std::move(Oth.ShadowComputeContext);
		FrustCullingContext = std::move(Oth.FrustCullingContext);
		OcclCullingContext = std::move(Oth.OcclCullingContext);
		DepthReduceContext = std::move(Oth.DepthReduceContext);
		BlurContext = std::move(Oth.BlurContext);
		DebugComputeContext = std::move(Oth.DebugComputeContext);
	}

	return *this;
}
