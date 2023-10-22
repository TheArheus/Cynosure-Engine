#include "intrinsics.h"
#include "utils.h"
#include "mesh.cpp"
#include "renderer_vulkan.cpp"
#include "win32_window.cpp"

#include <random>


// TODO: Implement correct materials per object instance
// TODO: Implement correct mesh object creation, update and other functions architecture
//           Use entity component system for objects
//           Posibility for oop with objects if needed
//           Maybe event system will be needed in the future?
//
// TODO: Implement sound system with openal???
//           Find API to implement this
//
// TODO: Use scene class and use vector of scene classes for multiple scenes
//           Change between them at runtime and posibility to change them while main app is running(right now it is not quite work)
//
// TODO: Use only one staging buffer instead of many (a lot of memory usage here, current solution is totally not optimal)
// TODO: Posibility to use only outer samplers(do not create a sampler for each texture)
// TODO: Fix image barriers and current layouts
// 
// TODO: Implement good while loop architecture
// TODO: Make everything to use memory allocator
// TODO: Reorganize files for the corresponding files. For ex. win32_window files to platform folders and etc.

struct indirect_draw_indexed_command
{
	VkDrawIndexedIndirectCommand DrawArg; // 5
	u32 CommandIdx;
};

// TODO: Make scale to be vec4 and add rotation vector
struct alignas(16) mesh_draw_command
{
	mesh::material Mat;
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
};

struct alignas(16) global_world_data 
{
	mat4  View;
	mat4  DebugView;
	mat4  Proj;
	mat4  LightView[DEPTH_CASCADES_COUNT];
	mat4  LightProj[DEPTH_CASCADES_COUNT];
	vec4  CameraPos;
	vec4  CameraDir;
	vec4  GlobalLightPos;
	float GlobalLightSize;
	u32   ColorSourceCount;
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	bool  DebugColors;
};

struct alignas(16) mesh_comp_culling_common_input
{
	plane CullingPlanes[6];
	u32   FrustrumCullingEnabled;
	u32   OcclusionCullingEnabled;
	float NearZ;
	u32   DrawCount;
	u32   MeshCount;
	mat4  Proj;
	mat4  View;
};

int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{
	window VulkanWindow("3D Renderer");
	VulkanWindow.InitGraphics();

	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	view_data  ViewData = {};
	ViewData.CameraPos  = vec3(-4, 4, 2);
	ViewData.ViewDir    = -ViewData.CameraPos;
	game_input GameInput{};
	GameInput.Buttons = VulkanWindow.Buttons;

	u32 GlobalMemorySize = MiB(128);
	void* MemoryBlock = malloc(GlobalMemorySize);

	float NearZ = 0.01f;
	float FarZ = 100.0f;
	buffer GeometryOffsets(VulkanWindow.Gfx, KiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer MeshDrawCommandDataBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	buffer DebugMeshDrawCommandDataBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer IndirectDrawIndexedCommands(VulkanWindow.Gfx, MiB(16), true, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer ShadowIndirectDrawIndexedCommands(VulkanWindow.Gfx, MiB(16), true, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

#if 0
	memory_heap VertexHeap(VulkanWindow.Gfx, MiB(16), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	memory_heap IndexHeap(VulkanWindow.Gfx, MiB(16), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	buffer VertexBuffer = VertexHeap.PushBuffer(VulkanWindow.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(VulkanWindow.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32));
#else
	buffer VertexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer IndexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	buffer DebugVertexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer DebugIndexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
#endif

	global_world_data WorldUpdate = {};
	buffer WorldUpdateBuffer(VulkanWindow.Gfx, sizeof(global_world_data), false, 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer LightSourcesBuffer(VulkanWindow.Gfx, sizeof(light_source) * 1024, false, 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	texture::input_data TextureInputData = {};
	TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Alignment = 0;
	std::vector<texture> GlobalShadow(DEPTH_CASCADES_COUNT); // TODO: Make it to work in a loop
	for(texture& Shadow : GlobalShadow)
	{
		Shadow = texture(VulkanWindow.Gfx, PreviousPowerOfTwo(VulkanWindow.Gfx->Width) * 4, PreviousPowerOfTwo(VulkanWindow.Gfx->Width) * 4, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	}

	TextureInputData.Format    = VK_FORMAT_R32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	TextureInputData.MipLevels = GetImageMipLevels(PreviousPowerOfTwo(VulkanWindow.Gfx->Width), PreviousPowerOfTwo(VulkanWindow.Gfx->Height));
	texture DepthPyramid(VulkanWindow.Gfx, PreviousPowerOfTwo(VulkanWindow.Gfx->Width), PreviousPowerOfTwo(VulkanWindow.Gfx->Height), 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_MAX);

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
	buffer PoissonDiskBuffer(VulkanWindow.Gfx, PoissonDisk, sizeof(vec2) * 64, false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	const u32 Res = 32;
	vec2  RandomAngles[Res][Res][Res] = {};
	for(u32 x = 0; x < Res; x++)
	{
		for(u32 y = 0; y < Res; y++)
		{
			for(u32 z = 0; z < Res; z++)
			{
				RandomAngles[x][y][z] = vec2(cosf(float(rand()) / RAND_MAX * 2.0f / Pi), 
											 sinf(float(rand()) / RAND_MAX * 2.0f / Pi));
			}
		}
	}
	vec2 RandomRotations[16] = {};
	for(u32 RotIdx = 0; RotIdx < 16; ++RotIdx)
	{
		RandomRotations[RotIdx] = vec2(float(rand()) / RAND_MAX * 2.0 - 1.0, float(rand()) / RAND_MAX * 2.0 - 1.0);
	}
	TextureInputData.Format    = VK_FORMAT_R32G32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_3D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_3D;
	TextureInputData.MipLevels = 1;
	texture RandomAnglesTexture(VulkanWindow.Gfx, (void*)RandomAngles, 32, 32, 32, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	texture NoiseTexture(VulkanWindow.Gfx, (void*)RandomRotations, 4, 4, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	vec3 RandomSamples[64] = {};
	for(u32 RotIdx = 0; RotIdx < 64; ++RotIdx)
	{
		vec3 Sample = vec3(float(rand()) / RAND_MAX * 2.0f - 1.0,
						   float(rand()) / RAND_MAX * 2.0f - 1.0,
						   float(rand()) / RAND_MAX).Normalize();
		Sample = Sample * float(rand()) / RAND_MAX * 2.0f - 1.0;
		float Scale = RotIdx / 64.0;
		Scale = Lerp(0.1, Scale, 1.0);
		RandomSamples[RotIdx] = Sample * Scale;
	}
	buffer RandomSamplesBuffer(VulkanWindow.Gfx, (void*)RandomSamples, sizeof(vec3) * 64, false, 0, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	// NOTE: Rendering targets:
	std::vector<texture> GBuffer(GBUFFER_COUNT);
	TextureInputData = {};
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Alignment = 0;
	TextureInputData.Format    = VulkanWindow.Gfx->SurfaceFormat.format;
	TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	texture GfxColorTarget(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData);
	TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SFLOAT;
	GBuffer[0] = texture(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData); // Vertex Normals
	TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SFLOAT;
	GBuffer[1] = texture(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData); // Vertex Normals
	GBuffer[2] = texture(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData); // Fragment Normals
	TextureInputData.Format    = VK_FORMAT_R8G8B8A8_UNORM;
	GBuffer[3] = texture(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData); // Diffuse Color
	TextureInputData.Format    = VK_FORMAT_R32_SFLOAT;
	GBuffer[4] = texture(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData); // Specular
	texture AmbientOcclusionData(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData);
	TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	texture GfxDepthTarget(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData);
	texture DebugCameraViewDepthTarget(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1, TextureInputData);

	mesh_comp_culling_common_input MeshCompCullingCommonData = {};
	MeshCompCullingCommonData.FrustrumCullingEnabled  = false;
	MeshCompCullingCommonData.OcclusionCullingEnabled = false;
	MeshCompCullingCommonData.NearZ = NearZ;
	buffer MeshCommonCullingData(VulkanWindow.Gfx, sizeof(mesh_comp_culling_common_input), false, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	buffer MeshDrawCommandBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer MeshDrawShadowCommandBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// TODO: Check and fix this new implementation. In theory, this should be correct
	shader_input GfxRootSignature;
	GfxRootSignature.PushStorageBuffer()->				// Vertex Data
					 PushUniformBuffer()->				// World Global Data
					 PushStorageBuffer()->				// MeshDrawCommands
					 PushImageSampler(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)->				// Diffuse Texture
					 PushImageSampler(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)->				// Normal Map Texture
					 PushImageSampler(1, 0, VK_SHADER_STAGE_FRAGMENT_BIT)->				// Height Map Texture
					 Build(VulkanWindow.Gfx);

	shader_input ColorPassRootSignature;
	ColorPassRootSignature.PushUniformBuffer()->					// World Global Data
						   PushUniformBuffer()->					// Array of Light Sources
						   PushStorageBuffer()->					// Poisson Disk
						   PushImageSampler()->						// Random Rotations
						   PushImageSampler()->						// G-Buffer DepthStencil Data
						   PushImageSampler()->						// G-Buffer Vertex Normal Data
						   PushImageSampler()->						// G-Buffer Fragment Normal Data
						   PushImageSampler()->						// G-Buffer Diffuse Data
						   PushImageSampler()->						// G-Buffer Specular Data
						   PushImageSampler()->						// Ambient Occlusion Data
						   PushStorageImage()->						// Color Target Texture
						   PushImageSampler(DEPTH_CASCADES_COUNT)->	// Shadow Cascades
						   PushConstant((DEPTH_CASCADES_COUNT + 1) * sizeof(float))-> // Cascade Splits
						   Build(VulkanWindow.Gfx);

	shader_input AmbientOcclusionRootSignature;
	AmbientOcclusionRootSignature.PushUniformBuffer()->					// World Global Data
								  PushStorageBuffer()->					// Poisson Disk
								  PushImageSampler()->					// Random Rotations
								  PushImageSampler()->					// G-Buffer DepthStencil Data
								  PushImageSampler()->					// G-Buffer Vertex Normal Data
								  PushImageSampler()->					// G-Buffer Fragment Normal Data
								  PushImageSampler()->					// G-Buffer Diffuse Data
								  PushImageSampler()->					// G-Buffer Specular Data
								  PushStorageImage()->					// Ambient Occlusion Target Texture
								  Build(VulkanWindow.Gfx);

	shader_input DebugRootSignature;
	DebugRootSignature.PushStorageBuffer()->
					   PushUniformBuffer()->
					   PushStorageBuffer()->
					   Build(VulkanWindow.Gfx);

	shader_input ShadowSignature;
	ShadowSignature.PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
					PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
					PushConstant(sizeof(mat4))->
					Build(VulkanWindow.Gfx);

	shader_input CmpIndirectFrustRootSignature;
	CmpIndirectFrustRootSignature.
					 PushStorageBuffer()->				// Mesh Offsets
					 PushStorageBuffer()->				// Common Data
					 PushStorageBuffer()->				// Indirect Draw Indexed Command
					 PushStorageBuffer()->				// Indirect Draw Indexed Command Counter
					 PushStorageBuffer()->				// Shadow Indirect Draw Indexed Command
					 PushStorageBuffer()->				// Shadow Indirect Draw Indexed Command Counter
					 PushStorageBuffer()->				// Mesh Draw Command Data
					 PushStorageBuffer()->				// Mesh Draw Shadow Command Data
					 PushStorageBuffer()->				// Common Input
					 Build(VulkanWindow.Gfx);

	shader_input CmpIndirectOcclRootSignature;
	CmpIndirectOcclRootSignature.
					 PushStorageBuffer()->				// Mesh Offsets
					 PushStorageBuffer()->				// Common Data
					 PushStorageBuffer()->				// Indirect Draw Indexed Command
					 PushStorageBuffer()->				// Indirect Draw Indexed Command Counter
					 PushStorageBuffer()->				// Mesh Draw Command Data
					 PushStorageBuffer()->				// Common Input
					 PushImageSampler()->				// Hierarchy-Z depth texture
					 Build(VulkanWindow.Gfx);

	shader_input CmpReduceRootSignature;
	CmpReduceRootSignature.PushImageSampler(1, 0, VK_SHADER_STAGE_COMPUTE_BIT)->		// Input  Texture
						   PushStorageImage(1, 0, VK_SHADER_STAGE_COMPUTE_BIT)->		// Output Texture
						   PushConstant(sizeof(vec2), VK_SHADER_STAGE_COMPUTE_BIT)->	// Output Texture Size
						   Build(VulkanWindow.Gfx);

	shader_input BlurRootSignature;
	BlurRootSignature.PushImageSampler(1, 0, VK_SHADER_STAGE_COMPUTE_BIT)->		// Input  Texture
					  PushStorageImage(1, 0, VK_SHADER_STAGE_COMPUTE_BIT)->		// Output Texture
					  PushConstant(sizeof(vec3), VK_SHADER_STAGE_COMPUTE_BIT)->		// Texture Size, Conv Size 
					  Build(VulkanWindow.Gfx);

	// TODO: Maybe create hot load shaders and compile them and runtime inside
	std::vector<VkFormat> GfxFormats;
	for(u32 FormatIdx = 0; FormatIdx < GBuffer.size(); ++FormatIdx) GfxFormats.push_back(GBuffer[FormatIdx].Info.Format);
	global_pipeline_context PipelineContext(VulkanWindow.Gfx);
	render_context GfxContext(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GfxRootSignature, {"..\\build\\mesh.vert.spv", "..\\build\\mesh.frag.spv"}, GfxFormats);

	render_context::input_data RendererInputData = {};
	RendererInputData.UseColor		= true;
	RendererInputData.UseDepth		= true;
	RendererInputData.UseBackFace	= true;
	RendererInputData.UseOutline	= true;
	render_context  DebugContext(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, DebugRootSignature, {"..\\build\\mesh.dbg.vert.spv", "..\\build\\mesh.dbg.frag.spv"}, {GfxColorTarget.Info.Format}, RendererInputData);

	RendererInputData = {};
	RendererInputData.UseDepth = true;
	render_context  CascadeShadowContext(VulkanWindow.Gfx, GlobalShadow[0].Width, GlobalShadow[0].Height, ShadowSignature, {"..\\build\\mesh.sdw.vert.spv", "..\\build\\mesh.sdw.frag.spv"}, {}, RendererInputData);
	render_context  ShadowContext(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, ShadowSignature, {"..\\build\\mesh.sdw.vert.spv", "..\\build\\mesh.sdw.frag.spv"}, {}, RendererInputData);

	RendererInputData.UseDepth		= true;
	RendererInputData.UseBackFace	= true;
	render_context  DebugCameraViewContext(VulkanWindow.Gfx, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, ShadowSignature, {"..\\build\\mesh.sdw.vert.spv", "..\\build\\mesh.sdw.frag.spv"}, {}, RendererInputData);

	compute_context ColorPassContext(VulkanWindow.Gfx, ColorPassRootSignature, "..\\build\\color_pass.comp.spv");
	compute_context AmbientOcclusionContext(VulkanWindow.Gfx, AmbientOcclusionRootSignature, "..\\build\\screen_space_ambient_occlusion.comp.spv");
	compute_context FrustCullingContext(VulkanWindow.Gfx, CmpIndirectFrustRootSignature, "..\\build\\indirect_cull_frust.comp.spv");
	compute_context OcclCullingContext(VulkanWindow.Gfx, CmpIndirectOcclRootSignature, "..\\build\\indirect_cull_occl.comp.spv");
	compute_context DepthReduceContext(VulkanWindow.Gfx, CmpReduceRootSignature, "..\\build\\depth_reduce.comp.spv");
	compute_context BlurContext(VulkanWindow.Gfx, BlurRootSignature, "..\\build\\blur.comp.spv");

	float CascadeSplits[DEPTH_CASCADES_COUNT + 1] = {};
	CascadeSplits[0] = NearZ;
	for(u32 CascadeIdx = 1;
		CascadeIdx < DEPTH_CASCADES_COUNT;
		++CascadeIdx)
	{
		CascadeSplits[CascadeIdx] = Lerp(NearZ * powf(FarZ / NearZ, float(CascadeIdx) / DEPTH_CASCADES_COUNT),
										 0.75,
										 NearZ + (float(CascadeIdx) / DEPTH_CASCADES_COUNT) * (FarZ - NearZ));
	}
	CascadeSplits[DEPTH_CASCADES_COUNT] = FarZ;

	std::vector<u32> CubeIndices = 
	{
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
	};
	vec3 CascadeColors[] = 
	{
		vec3(1, 0, 0),
		vec3(0, 1, 0),
		vec3(0, 0, 1),
		vec3(1, 1, 0),
		vec3(1, 0, 1),
		vec3(0, 1, 1),
		vec3(1, 1, 1),
		vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX),
	};

	vec3 ViewDir = ViewData.ViewDir;
	vec3 ViewPos = ViewData.CameraPos;
	vec3 UpVector(0, 1, 0);

	bool SceneIsLoaded  = false;
	bool DebugIsLoaded  = false;
	bool IsFirstFrame   = true;
	bool IsCameraLocked = false;
	bool IsDebugColors  = false;

	mesh GlobalGeometries;

	// TODO: create necessary textures on scene load. Maybe think about something better on the architecture here
	// Every type of the textures(diffuse, specular, normal etc) should be here?
	std::vector<texture> Textures;
	texture_data Diffuse1("..\\assets\\brick-wall2.diff.tga");
	texture_data Normal1("..\\assets\\brick-wall2.norm.tga");
	texture_data Height1("..\\assets\\brick-wall2.disp.png");

	TextureInputData.Format    = VK_FORMAT_R8G8B8A8_SRGB;
	TextureInputData.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Alignment = 0;
	Textures.push_back(texture(VulkanWindow.Gfx, Diffuse1.Data, Diffuse1.Width, Diffuse1.Height, 1, TextureInputData));
	Textures.push_back(texture(VulkanWindow.Gfx, Normal1.Data, Normal1.Width, Normal1.Height, 1, TextureInputData));
	Textures.push_back(texture(VulkanWindow.Gfx, Height1.Data, Height1.Width, Height1.Height, 1, TextureInputData));
	stbi_image_free(Diffuse1.Data);
	stbi_image_free(Normal1.Data);
	stbi_image_free(Height1.Data);

	game_scene_create* CreateCubeGameScene = (game_scene_create*)window::GetProcAddr("..\\build\\cube_scene.dll", "CubeSceneCreate");
	std::unique_ptr<scene> GameScene(CreateCubeGameScene());

	double TimeLast = window::GetTimestamp();
	double TimeElapsed = 0.0;
	double TimeEnd = 0.0;
	double AvgCpuTime = 0.0;
	while(VulkanWindow.IsRunning())
	{
		mesh DebugGeometries;

		linear_allocator SystemsAllocator(GlobalMemorySize, MemoryBlock);
		linear_allocator LightSourcesAlloc(sizeof(light_source) * 1024, SystemsAllocator.Allocate(sizeof(light_source) * 2048));
		linear_allocator GlobalMeshInstancesAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator DebugMeshInstancesAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));

		// TODO: Use memory allocator with pre-allocated memory
		std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>> GlobalMeshInstances(GlobalMeshInstancesAlloc);
		std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>> DebugMeshInstances(DebugMeshInstancesAlloc);
		std::vector<light_source, allocator_adapter<light_source, linear_allocator>> GlobalLightSources(LightSourcesAlloc);
		
		auto Result = VulkanWindow.ProcessMessages();
		if(Result) return *Result;

		if(VulkanWindow.Buttons[EC_LCONTROL].IsDown && VulkanWindow.Buttons[EC_R].IsDown)
		{
			// TODO: Load and unload game scenes (will be inside of the scene manager)
		}

		GameInput.DeltaTime = TimeElapsed;

		if(!GameScene->IsInitialized)
		{
			GameScene->Start(GlobalGeometries);
		}

		GameScene->Reset();
		GameScene->Update(GlobalMeshInstances);
		GlobalLightSources.insert(GlobalLightSources.end(), GameScene->LightSources.begin(), GameScene->LightSources.end());

		if(VulkanWindow.Buttons[EC_I].IsDown)
		{
			IsDebugColors = !IsDebugColors;
		}
		if(VulkanWindow.Buttons[EC_L].IsDown)
		{
			IsCameraLocked = !IsCameraLocked;
		}
		if(!IsCameraLocked)
		{
			ViewDir = ViewData.ViewDir;
			ViewPos	= ViewData.CameraPos;
		}
		//GlobalLightSources.push_back({vec4(ViewPos, 1), ViewDir, vec4(1, 0, 0, 1), light_type_spot});

		float Aspect  = (float)VulkanWindow.Gfx->Width / (float)VulkanWindow.Gfx->Height;
		vec3 LightPos =  vec3(-4, 4, 2);
		vec3 LightDir = -Normalize(LightPos);

		mat4 CameraProj = PerspRH(45.0f, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, NearZ, FarZ);
		mat4 CameraView = LookAtRH(ViewData.CameraPos, ViewData.CameraPos + ViewData.ViewDir, UpVector);
		mat4 DebugCameraView = LookAtRH(ViewPos, ViewPos + ViewDir, UpVector);
		GeneratePlanes(MeshCompCullingCommonData.CullingPlanes, CameraProj, 1);

		WorldUpdate = {};
		WorldUpdate.View			= CameraView;
		WorldUpdate.DebugView		= DebugCameraView;
		WorldUpdate.Proj			= CameraProj;
		WorldUpdate.CameraPos		= vec4(ViewPos, 0);
		WorldUpdate.CameraDir		= vec4(ViewDir, 0);
		WorldUpdate.GlobalLightPos	= vec4(LightPos, 0);
		WorldUpdate.GlobalLightSize = 1;
		WorldUpdate.ScreenWidth		= VulkanWindow.Gfx->Width;
		WorldUpdate.ScreenHeight	= VulkanWindow.Gfx->Height;
		WorldUpdate.NearZ			= NearZ;
		WorldUpdate.FarZ			= FarZ;
		WorldUpdate.DebugColors		= IsDebugColors;

		// TODO: try to move ortho projection calculation to compute shader???
		// TODO: move into game_main everything related to cascade splits calculation
		for(u32 CascadeIdx = 1;
			CascadeIdx <= DEPTH_CASCADES_COUNT;
			++CascadeIdx)
		{
			std::vector<vertex> DebugFrustum;

			float LightNearZ = CascadeSplits[CascadeIdx - 1];
			float LightFarZ  = CascadeSplits[CascadeIdx];
			float LightDistZ = LightFarZ - LightNearZ;

			mat4 CameraCascadeProj = PerspRH(45.0f, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, LightNearZ, LightFarZ);
			mat4 InverseProjectViewMatrix = Inverse(DebugCameraView * CameraCascadeProj);

			std::vector<vec4> CameraViewCorners = 
			{
				// Near
				vec4{-1.0f, -1.0f, 0.0f, 1.0f},
				vec4{ 1.0f, -1.0f, 0.0f, 1.0f},
				vec4{ 1.0f,  1.0f, 0.0f, 1.0f},
				vec4{-1.0f,  1.0f, 0.0f, 1.0f},
				// Far
				vec4{-1.0f, -1.0f, 1.0f, 1.0f},
				vec4{ 1.0f, -1.0f, 1.0f, 1.0f},
				vec4{ 1.0f,  1.0f, 1.0f, 1.0f},
				vec4{-1.0f,  1.0f, 1.0f, 1.0f},
			};
			std::vector<vec4> LightAABBCorners = CameraViewCorners; // NOTE: Light View Corners
			std::vector<vec4> LightViewCorners = CameraViewCorners; // NOTE: Light View Corners

			for(size_t i = 0; i < CameraViewCorners.size(); i++)
			{
				CameraViewCorners[i] = InverseProjectViewMatrix * CameraViewCorners[i];
				CameraViewCorners[i] = CameraViewCorners[i] / CameraViewCorners[i].w;

				DebugFrustum.push_back({CameraViewCorners[i], {}, 0});
			}
			DebugMeshInstances.push_back({{vec4(CascadeColors[CascadeIdx - 1], 1), 0, 0, 0, 0}, vec4(0), vec4(1), DebugGeometries.Load(DebugFrustum, CubeIndices), true});

			vec3 FrustumCenter(0);
			for(vec3 V : CameraViewCorners)
			{
				FrustumCenter += V;
			}
			FrustumCenter /= 8.0;

			float Radius = -INFINITY;
			for(vec3 V : CameraViewCorners)
			{
				float Dist = Length(V - FrustumCenter);
				Radius = Max(Radius, Dist);
			}
			WorldUpdate.LightView[CascadeIdx - 1] = LookAtRH(FrustumCenter - LightDir * Radius, FrustumCenter, UpVector);
			WorldUpdate.LightProj[CascadeIdx - 1] = OrthoRH(-Radius, Radius, Radius, -Radius, 0.0f, 2.0f * Radius);

			vec4 ShadowOrigin = vec4(0, 0, 0, 1);
			mat4 ShadowMatrix = WorldUpdate.LightView[CascadeIdx - 1] * WorldUpdate.LightProj[CascadeIdx - 1];
			ShadowOrigin = ShadowMatrix * ShadowOrigin;
			ShadowOrigin = ShadowOrigin * GlobalShadow[CascadeIdx - 1].Width / 2.0f;

			vec4 RoundedOrigin = vec4(round(ShadowOrigin.x), round(ShadowOrigin.y), round(ShadowOrigin.z), round(ShadowOrigin.w));
			vec4 RoundOffset = RoundedOrigin - ShadowOrigin;
			RoundOffset = RoundOffset * 2.0f / GlobalShadow[CascadeIdx - 1].Width;
			RoundOffset.z = 0.0f;
			RoundOffset.w = 0.0f;
			WorldUpdate.LightProj[CascadeIdx - 1].Line3 += RoundOffset;
		}

		MeshCompCullingCommonData.DrawCount = GlobalMeshInstances.size();
		MeshCompCullingCommonData.MeshCount = GlobalGeometries.MeshCount;
		MeshCompCullingCommonData.Proj = CameraProj;
		MeshCompCullingCommonData.View = WorldUpdate.DebugView;
		WorldUpdate.ColorSourceCount = GlobalLightSources.size();

		float CameraSpeed = 0.01f;
		if(GameInput.Buttons[EC_R].IsDown)
		{
			ViewData.CameraPos += vec3(0, 4.0f*CameraSpeed, 0);
		}
		if(GameInput.Buttons[EC_F].IsDown)
		{
			ViewData.CameraPos -= vec3(0, 4.0f*CameraSpeed, 0);
		}
		if(GameInput.Buttons[EC_W].IsDown)
		{
			ViewData.CameraPos += (ViewData.ViewDir * CameraSpeed);
		}
		if(GameInput.Buttons[EC_S].IsDown)
		{
			ViewData.CameraPos -= (ViewData.ViewDir * CameraSpeed);
		}
#if 0
		vec3 z = (ViewData.ViewDir - ViewData.CameraPos).Normalize();
		vec3 x = Cross(vec3(0, 1, 0), z).Normalize();
		vec3 y = Cross(z, x);
		vec3 Horizontal = x;
		if(GameInput.Buttons[EC_D].IsDown)
		{
			ViewData.CameraPos -= Horizontal * CameraSpeed;
		}
		if(GameInput.Buttons[EC_A].IsDown)
		{
			ViewData.CameraPos += Horizontal * CameraSpeed;
		}
#endif
		if(GameInput.Buttons[EC_LEFT].IsDown)
		{
			quat ViewDirQuat(ViewData.ViewDir, 0);
			quat RotQuat( CameraSpeed * 2, vec3(0, 1, 0));
			ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		if(GameInput.Buttons[EC_RIGHT].IsDown)
		{
			quat ViewDirQuat(ViewData.ViewDir, 0);
			quat RotQuat(-CameraSpeed * 2, vec3(0, 1, 0));
			ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		vec3 U = Cross(vec3(0, 1, 0), ViewData.ViewDir);
		if(GameInput.Buttons[EC_UP].IsDown)
		{
			quat ViewDirQuat(ViewData.ViewDir, 0);
			quat RotQuat( CameraSpeed / 2, U);
			ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		if(GameInput.Buttons[EC_DOWN].IsDown)
		{
			quat ViewDirQuat(ViewData.ViewDir, 0);
			quat RotQuat(-CameraSpeed / 2, U);
			ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}

		if(!VulkanWindow.IsGfxPaused)
		{
			PipelineContext.Begin(VulkanWindow.Gfx);

			// NOTE: Those buffer updates makes everything slow when there are a lot of meshes with high ammount of vertices
			// TODO: Optimize the updates(update only when the actual data have been changed etc.)
			if(!IsFirstFrame)
			{
				MeshDrawCommandDataBuffer.ReadBackSize(VulkanWindow.Gfx, GlobalMeshInstances.data(), GlobalMeshInstances.size() * sizeof(mesh_draw_command_input), PipelineContext);
			}

			GeometryOffsets.UpdateSize(VulkanWindow.Gfx, GlobalGeometries.Offsets.data(), GlobalGeometries.Offsets.size() * sizeof(mesh::offset), PipelineContext);

			VertexBuffer.UpdateSize(VulkanWindow.Gfx, GlobalGeometries.Vertices.data(), GlobalGeometries.Vertices.size() * sizeof(vertex), PipelineContext);
			IndexBuffer.UpdateSize(VulkanWindow.Gfx, GlobalGeometries.VertexIndices.data(), GlobalGeometries.VertexIndices.size() * sizeof(u32), PipelineContext);
			DebugVertexBuffer.UpdateSize(VulkanWindow.Gfx, DebugGeometries.Vertices.data(), DebugGeometries.Vertices.size() * sizeof(vertex), PipelineContext);
			DebugIndexBuffer.UpdateSize(VulkanWindow.Gfx, DebugGeometries.VertexIndices.data(), DebugGeometries.VertexIndices.size() * sizeof(u32), PipelineContext);
			MeshDrawCommandDataBuffer.UpdateSize(VulkanWindow.Gfx, GlobalMeshInstances.data(), GlobalMeshInstances.size() * sizeof(mesh_draw_command_input), PipelineContext);
			DebugMeshDrawCommandDataBuffer.UpdateSize(VulkanWindow.Gfx, DebugMeshInstances.data(), DebugMeshInstances.size() * sizeof(mesh_draw_command_input), PipelineContext);
			WorldUpdateBuffer.Update(VulkanWindow.Gfx, (void*)&WorldUpdate, PipelineContext);
			MeshCommonCullingData.Update(VulkanWindow.Gfx, (void*)&MeshCompCullingCommonData, PipelineContext);
			LightSourcesBuffer.UpdateSize(VulkanWindow.Gfx, GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), PipelineContext);

			PipelineContext.SetBufferBarriers({
												  {GeometryOffsets, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {VertexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {IndexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {DebugVertexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {DebugIndexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {MeshDrawCommandDataBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {DebugMeshDrawCommandDataBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {WorldUpdateBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {MeshCommonCullingData, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
												  {LightSourcesBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
											  }, 
											  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

			{
				FrustCullingContext.Begin(PipelineContext);

				FrustCullingContext.SetStorageBufferView(GeometryOffsets);
				FrustCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
				FrustCullingContext.SetStorageBufferView(IndirectDrawIndexedCommands);
				FrustCullingContext.SetStorageBufferView(ShadowIndirectDrawIndexedCommands);
				FrustCullingContext.SetStorageBufferView(MeshDrawCommandBuffer);
				FrustCullingContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
				FrustCullingContext.SetStorageBufferView(MeshCommonCullingData);

				FrustCullingContext.Execute(GlobalMeshInstances.size());

				FrustCullingContext.End();
			}

			for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
			{
				mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];
				std::vector<VkImageMemoryBarrier> ShadowBarrier = 
				{
					CreateImageBarrier(GlobalShadow[CascadeIdx].Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, ShadowBarrier);

				CascadeShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, GlobalShadow[CascadeIdx].Width, GlobalShadow[CascadeIdx].Height, GlobalShadow[CascadeIdx], {1, 0});
				CascadeShadowContext.Begin(VulkanWindow.Gfx, PipelineContext);

				CascadeShadowContext.SetStorageBufferView(VertexBuffer);
				CascadeShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
				CascadeShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
				CascadeShadowContext.DrawIndirect<indirect_draw_indexed_command>(GlobalGeometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

				CascadeShadowContext.End();
			}

			// TODO: Don't forget to remove this one
			// NOTE: This is only for debug. Maybe not compile on release mode???
			{
				mat4 Shadow = DebugCameraView * CameraProj;

				DebugCameraViewContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, DebugCameraViewContext.Width, DebugCameraViewContext.Height, DebugCameraViewDepthTarget, {1, 0});
				DebugCameraViewContext.Begin(VulkanWindow.Gfx, PipelineContext);

				DebugCameraViewContext.SetStorageBufferView(VertexBuffer);
				DebugCameraViewContext.SetStorageBufferView(MeshDrawCommandBuffer);
				DebugCameraViewContext.SetConstant((void*)&Shadow, sizeof(mat4));
				DebugCameraViewContext.DrawIndirect<indirect_draw_indexed_command>(GlobalGeometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

				DebugCameraViewContext.End();
			}

			{
				std::vector<VkImageMemoryBarrier> ShadowBarrier;
				for(u32 CascadeIdx = 0;
					CascadeIdx < DEPTH_CASCADES_COUNT;
					++CascadeIdx)
				{
					ShadowBarrier.push_back(CreateImageBarrier(GlobalShadow[CascadeIdx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
				}
				for(u32 Idx = 0;
					Idx < Textures.size();
					++Idx)
				{
					ShadowBarrier.push_back(CreateImageBarrier(Textures[Idx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
				}
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, ShadowBarrier);

				GfxContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GBuffer, {0, 0, 0, 0});
				GfxContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GfxDepthTarget, {1, 0});
				GfxContext.Begin(VulkanWindow.Gfx, PipelineContext);

				GfxContext.SetStorageBufferView(VertexBuffer);
				GfxContext.SetUniformBufferView(WorldUpdateBuffer);
				GfxContext.SetStorageBufferView(MeshDrawCommandBuffer);
				GfxContext.SetImageSampler({Textures[0]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.SetImageSampler({Textures[1]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.SetImageSampler({Textures[2]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.DrawIndirect<indirect_draw_indexed_command>(GlobalGeometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

				GfxContext.End();
			}

			{
				AmbientOcclusionContext.Begin(PipelineContext);

				std::vector<VkImageMemoryBarrier> AmbientOcclusionPassBarrier = 
				{
					CreateImageBarrier(AmbientOcclusionData.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
					CreateImageBarrier(GfxDepthTarget.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
				};
				for(u32 Idx = 0; Idx < GBUFFER_COUNT; Idx++)
				{
					AmbientOcclusionPassBarrier.push_back(CreateImageBarrier(GBuffer[Idx].Handle, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
				}
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, AmbientOcclusionPassBarrier);

				AmbientOcclusionContext.SetUniformBufferView(WorldUpdateBuffer);
				AmbientOcclusionContext.SetStorageBufferView(RandomSamplesBuffer);
				AmbientOcclusionContext.SetImageSampler({NoiseTexture}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				//AmbientOcclusionContext.SetImageSampler({GfxDepthTarget}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				AmbientOcclusionContext.SetImageSampler(GBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				AmbientOcclusionContext.SetStorageImage({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
				AmbientOcclusionContext.Execute(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height);

				AmbientOcclusionContext.End();
			}

			{
				BlurContext.Begin(PipelineContext);

				std::vector<VkImageMemoryBarrier> BlurPassBarrier = 
				{
					CreateImageBarrier(AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, BlurPassBarrier);

				vec3 BlurInput(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 9);
				BlurContext.SetImageSampler({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
				BlurContext.SetStorageImage({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
				BlurContext.SetConstant((void*)BlurInput.E, sizeof(vec3));
				BlurContext.Execute(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height);

				BlurContext.End();
			}

			{
				ColorPassContext.Begin(PipelineContext);

				std::vector<VkImageMemoryBarrier> ColorPassBarrier = 
				{
					CreateImageBarrier(GfxColorTarget.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
					CreateImageBarrier(AmbientOcclusionData.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, ColorPassBarrier);

				ColorPassContext.SetUniformBufferView(WorldUpdateBuffer);
				ColorPassContext.SetUniformBufferView(LightSourcesBuffer);
				ColorPassContext.SetStorageBufferView(PoissonDiskBuffer);
				ColorPassContext.SetImageSampler({RandomAnglesTexture}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				//ColorPassContext.SetImageSampler({GfxDepthTarget}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				ColorPassContext.SetImageSampler(GBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				ColorPassContext.SetImageSampler({AmbientOcclusionData}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				ColorPassContext.SetStorageImage({GfxColorTarget}, VK_IMAGE_LAYOUT_GENERAL);
				ColorPassContext.SetImageSampler(GlobalShadow, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				ColorPassContext.SetConstant(CascadeSplits, (DEPTH_CASCADES_COUNT + 1) * sizeof(float));
				ColorPassContext.Execute(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height);

				ColorPassContext.End();
			}

			{
				DepthReduceContext.Begin(PipelineContext);

				std::vector<VkImageMemoryBarrier> DepthReadBarriers = 
				{
					CreateImageBarrier(DebugCameraViewDepthTarget.Handle, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
					CreateImageBarrier(DepthPyramid.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DepthReadBarriers);

				for(u32 MipIdx = 0;
					MipIdx < DepthPyramid.Info.MipLevels;
					++MipIdx)
				{
					vec2 VecDims(Max(1, DepthPyramid.Width  >> MipIdx),
								 Max(1, DepthPyramid.Height >> MipIdx));

					DepthReduceContext.SetImageSampler({MipIdx == 0 ? DebugCameraViewDepthTarget : DepthPyramid}, MipIdx == 0 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, MipIdx == 0 ? MipIdx : (MipIdx - 1));
					DepthReduceContext.SetStorageImage({DepthPyramid}, VK_IMAGE_LAYOUT_GENERAL, MipIdx);
					DepthReduceContext.SetConstant((void*)VecDims.E, sizeof(vec2));

					DepthReduceContext.Execute(VecDims.x, VecDims.y);

					std::vector<VkImageMemoryBarrier> DepthPyramidBarrier = 
					{
						CreateImageBarrier(DepthPyramid.Handle, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
					};
					ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DepthPyramidBarrier);
				}
				DepthReduceContext.End();
			}

			for(u32 DrawIdx = 0; DrawIdx < DebugGeometries.Offsets.size(); ++DrawIdx)
			{
				DebugContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, {GfxColorTarget}, {0, 0, 0, 1});
				DebugContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GfxDepthTarget, {1, 0});
				DebugContext.Begin(VulkanWindow.Gfx, PipelineContext);

				DebugContext.SetStorageBufferView(DebugVertexBuffer);
				DebugContext.SetUniformBufferView(WorldUpdateBuffer);
				DebugContext.SetStorageBufferView(DebugMeshDrawCommandDataBuffer);
				DebugContext.DrawIndexed(DebugIndexBuffer, 0, DebugGeometries.Offsets[DrawIdx].IndexCount, DebugGeometries.Offsets[DrawIdx].VertexOffset, DrawIdx);

				DebugContext.End();
			}

			{
				OcclCullingContext.Begin(PipelineContext);

				OcclCullingContext.SetStorageBufferView(GeometryOffsets);
				OcclCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
				OcclCullingContext.SetStorageBufferView(IndirectDrawIndexedCommands);
				OcclCullingContext.SetStorageBufferView(MeshDrawCommandBuffer);
				OcclCullingContext.SetStorageBufferView(MeshCommonCullingData);
				OcclCullingContext.SetImageSampler({DepthPyramid}, VK_IMAGE_LAYOUT_GENERAL);

				OcclCullingContext.Execute(GlobalMeshInstances.size());

				OcclCullingContext.End();
			}

#if 0
			std::vector<VkImageMemoryBarrier> ColorPassBarrier = 
			{
				CreateImageBarrier(GfxColorTarget.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, ColorPassBarrier);
#endif

			PipelineContext.EmplaceColorTarget(VulkanWindow.Gfx, GfxColorTarget);
			PipelineContext.Present(VulkanWindow.Gfx);
		}

		TimeEnd = window::GetTimestamp();
		TimeElapsed = (TimeEnd - TimeLast);

#if 0
		if(TimeElapsed < TargetFrameRate)
		{
			while(TimeElapsed < TargetFrameRate)
			{
				double TimeToSleep = TargetFrameRate - TimeElapsed;
				if (TimeToSleep > 0) 
				{
					Sleep(TimeToSleep);
				}
				TimeEnd = window::GetTimestamp();
				TimeElapsed = TimeEnd - TimeLast;
			}
		}
#endif

		TimeLast = TimeEnd;
		AvgCpuTime = 0.75 * AvgCpuTime + TimeElapsed * 0.25;

		std::string Title = "Frame " + std::to_string(AvgCpuTime) + "ms, " + std::to_string(1.0 / AvgCpuTime * 1000.0) + "fps";
		VulkanWindow.SetTitle(Title);
		IsFirstFrame = false;
	}

	free(MemoryBlock);

	return 0;
}

int main(int argc, char* argv[])
{
	std::string CommandLine;
	for(int i = 1; i < argc; ++i)
	{
		CommandLine += std::string(argv[i]);
		if(i != (argc - 1))
		{
			CommandLine += " ";
		}
	}

	return WinMain(0, 0, const_cast<char*>(CommandLine.c_str()), SW_SHOWNORMAL);
}
