
#include "vulkan/vulkan_command_queue.cpp"
#include "vulkan/vulkan_pipeline_context.cpp"
#include "vulkan/vulkan_backend.cpp"

#if _WIN32
	#include "dx12/directx12_backend.cpp"
	#include "dx12/directx12_pipeline_context.cpp"
#endif

// TODO: CLEAR THE RESOURCES!!!

global_graphics_context::
global_graphics_context(renderer_backend* NewBackend, backend_type NewBackendType)
	: Backend(NewBackend), BackendType(NewBackendType)
{
	GlobalHeap = CreateMemoryHeap();

	utils::texture::input_data TextureInputData = {};
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
	TextureInputData.Usage     = image_flags::TF_ColorAttachment | image_flags::TF_Storage | image_flags::TF_CopySrc;
	GfxColorTarget = PushTexture("ColorTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	TextureInputData.Format    = image_format::D32_SFLOAT;
	TextureInputData.Usage     = image_flags::TF_DepthTexture | image_flags::TF_Sampled;
	GfxDepthTarget = PushTexture("DepthTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);
	DebugCameraViewDepthTarget = PushTexture("DebugCameraViewDepthTarget", nullptr, Backend->Width, Backend->Height, 1, TextureInputData);

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
	PoissonDiskBuffer = PushBuffer("PoissonDiskBuffer", PoissonDisk, sizeof(vec2), 64, false, resource_flags::RF_StorageBuffer);

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
	TextureInputData.Usage     = image_flags::TF_Storage | image_flags::TF_Sampled | image_flags::TF_CopyDst | image_flags::TF_ColorTexture;
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
		Sample = Sample *  float(rand());
		float Scale = RotIdx / 64.0;
		Scale = Lerp(0.1, Scale, 1.0);
		RandomSamples[RotIdx] = vec4(Sample * Scale, 0);
	}
	RandomSamplesBuffer = PushBuffer("RandomSamplesBuffer", (void*)RandomSamples, sizeof(vec4), 64, false, resource_flags::RF_StorageBuffer);

	GlobalShadow.resize(DEPTH_CASCADES_COUNT);
	TextureInputData.Format    = image_format::D32_SFLOAT;
	TextureInputData.Usage     = image_flags::TF_DepthTexture | image_flags::TF_Sampled;
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; CascadeIdx++)
	{
		GlobalShadow[CascadeIdx] = PushTexture("GlobalShadow" + std::to_string(CascadeIdx), nullptr, PreviousPowerOfTwo(Backend->Width) * 2, PreviousPowerOfTwo(Backend->Width) * 2, 1, TextureInputData);
	}

	TextureInputData.Format    = image_format::R32_SFLOAT;
	TextureInputData.Usage     = image_flags::TF_Sampled | image_flags::TF_Storage | image_flags::TF_CopySrc | image_flags::TF_ColorTexture;
	TextureInputData.MipLevels = GetImageMipLevels(PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height));
	TextureInputData.ReductionMode = sampler_reduction_mode::max;
	DepthPyramid = PushTexture("DepthPyramid", PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height), 1, TextureInputData);

	GBuffer.resize(GBUFFER_COUNT);
	TextureInputData = {};
	TextureInputData.Type	   = image_type::Texture2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Usage     = image_flags::TF_ColorAttachment | image_flags::TF_Sampled | image_flags::TF_Storage | image_flags::TF_CopySrc;
	TextureInputData.Format    = image_format::R16G16B16A16_SFLOAT;
	GBuffer[0] = PushTexture("GBuffer0", nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Vertex Positions
	TextureInputData.Format    = image_format::R16G16B16A16_SNORM;
	GBuffer[1] = PushTexture("GBuffer1", nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Vertex Normals
	GBuffer[2] = PushTexture("GBuffer2", nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Fragment Normals
	TextureInputData.Format    = image_format::R8G8B8A8_UNORM;
	GBuffer[3] = PushTexture("GBuffer3", nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Diffuse Color
	TextureInputData.Format    = image_format::R32_SFLOAT;
	GBuffer[4] = PushTexture("GBuffer4", nullptr, Backend->Width, Backend->Height, 1, TextureInputData); // Specular
	AmbientOcclusionData = PushTexture("AmbientOcclusionData", Backend->Width, Backend->Height, 1, TextureInputData);
	BlurTemp			 = PushTexture("BlurTemp", Backend->Width, Backend->Height, 1, TextureInputData);

	// TODO: Better context creation API
	utils::render_context::input_data RendererInputData = {};
	RendererInputData.UseColor	  = true;
	RendererInputData.UseDepth	  = true;
	RendererInputData.UseBackFace = true;
	RendererInputData.UseOutline  = true;
	GfxContext = CreateRenderContext(load_op::clear, store_op::store, {"..\\shaders\\mesh.vert.glsl", "..\\shaders\\mesh.frag.glsl"}, GBuffer, {true, true, true, false, false, 0}, {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}});
	DebugContext = CreateRenderContext(load_op::load, store_op::store, {"..\\shaders\\mesh.dbg.vert.glsl", "..\\shaders\\mesh.dbg.frag.glsl"}, {GfxColorTarget}, RendererInputData, {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}});
	DebugComputeContext = CreateComputeContext("..\\shaders\\mesh.dbg.comp.glsl");

	RendererInputData = {};
	RendererInputData.UseDepth = true;
	RendererInputData.UseBackFace  = true;
	CascadeShadowContext = CreateRenderContext(load_op::clear, store_op::store, {"..\\shaders\\mesh.sdw.vert.glsl", "..\\shaders\\mesh.sdw.frag.glsl"}, {}, RendererInputData, {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}});
	ShadowContext = CreateRenderContext(load_op::clear, store_op::store, {"..\\shaders\\mesh.sdw.vert.glsl", "..\\shaders\\mesh.sdw.frag.glsl"}, {}, RendererInputData, {{STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}});

	RendererInputData.UseMultiview = true;
	for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
	{
		RendererInputData.ViewMask = 1 << CubeMapFaceIdx;
		CubeMapShadowContexts.push_back(CreateRenderContext(load_op::clear, store_op::store, {"..\\shaders\\mesh.pnt.sdw.vert.glsl", "..\\shaders\\mesh.pnt.sdw.frag.glsl"}, {}, RendererInputData));
	}

	RendererInputData.UseDepth	   = true;
	RendererInputData.UseMultiview = false;
	RendererInputData.ViewMask	   = 0;
	DebugCameraViewContext = CreateRenderContext(load_op::clear, store_op::store, {"..\\shaders\\mesh.sdw.vert.glsl", "..\\shaders\\mesh.sdw.frag.glsl"}, {}, RendererInputData);

	ColorPassContext = CreateComputeContext("..\\shaders\\color_pass.comp.glsl", {{"GBUFFER_COUNT", std::to_string(GBUFFER_COUNT)}, {STRINGIFY(LIGHT_SOURCES_MAX_COUNT), std::to_string(LIGHT_SOURCES_MAX_COUNT)}, {STRINGIFY(DEBUG_COLOR_BLEND), std::to_string(DEBUG_COLOR_BLEND)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}});
	AmbientOcclusionContext = CreateComputeContext("..\\shaders\\screen_space_ambient_occlusion.comp.glsl", {{STRINGIFY(GBUFFER_COUNT), std::to_string(GBUFFER_COUNT)}, {STRINGIFY(DEPTH_CASCADES_COUNT), std::to_string(DEPTH_CASCADES_COUNT)}});
	ShadowComputeContext = CreateComputeContext("..\\shaders\\mesh.dbg.comp.glsl");
	FrustCullingContext  = CreateComputeContext("..\\shaders\\indirect_cull_frust.comp.glsl");
	OcclCullingContext   = CreateComputeContext("..\\shaders\\indirect_cull_occl.comp.glsl");
	BlurContextV = CreateComputeContext("..\\shaders\\blur.comp.glsl");
	BlurContextH = CreateComputeContext("..\\shaders\\blur.comp.glsl");

	for(u32 MipIdx = 0; MipIdx < GetImageMipLevels(PreviousPowerOfTwo(Backend->Width), PreviousPowerOfTwo(Backend->Height)); ++MipIdx)
	{
		DepthReduceContext.push_back(CreateComputeContext("..\\shaders\\depth_reduce.comp.glsl"));
	}
}

global_graphics_context::
global_graphics_context(global_graphics_context&& Oth) noexcept : 
	Backend(std::move(Oth.Backend)),
	BackendType(std::move(Oth.BackendType)),
	GlobalHeap(std::move(Oth.GlobalHeap)),
	PoissonDiskBuffer(std::move(Oth.PoissonDiskBuffer)),
	RandomSamplesBuffer(std::move(Oth.RandomSamplesBuffer)),
	GfxColorTarget(std::move(Oth.GfxColorTarget)),
	GfxDepthTarget(std::move(Oth.GfxDepthTarget)),
	DebugCameraViewDepthTarget(std::move(Oth.DebugCameraViewDepthTarget)),
	GlobalShadow(std::move(Oth.GlobalShadow)),
	GBuffer(std::move(Oth.GBuffer)),
	AmbientOcclusionData(std::move(Oth.AmbientOcclusionData)),
	BlurTemp(std::move(Oth.BlurTemp)),
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
	BlurContextV(std::move(Oth.BlurContextV)),
	BlurContextH(std::move(Oth.BlurContextH)),
	DebugComputeContext(std::move(Oth.DebugComputeContext))
{}

global_graphics_context& global_graphics_context::
operator=(global_graphics_context&& Oth) noexcept
{
	if(this != &Oth)
	{
		Backend = std::move(Oth.Backend);
		BackendType = std::move(Oth.BackendType);
		GlobalHeap = std::move(Oth.GlobalHeap);
		PoissonDiskBuffer = std::move(Oth.PoissonDiskBuffer);
		RandomSamplesBuffer = std::move(Oth.RandomSamplesBuffer);
		GfxColorTarget = std::move(Oth.GfxColorTarget);
		GfxDepthTarget = std::move(Oth.GfxDepthTarget);
		DebugCameraViewDepthTarget = std::move(Oth.DebugCameraViewDepthTarget);
		GlobalShadow = std::move(Oth.GlobalShadow);
		GBuffer = std::move(Oth.GBuffer);
		AmbientOcclusionData = std::move(Oth.AmbientOcclusionData);
		BlurTemp = std::move(Oth.BlurTemp);
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
		BlurContextV = std::move(Oth.BlurContextV);
		BlurContextH = std::move(Oth.BlurContextH);
		DebugComputeContext = std::move(Oth.DebugComputeContext);
	}

	return *this;
}
