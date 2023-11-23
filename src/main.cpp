#include "intrinsics.h"
#include "utils.h"
#include "scene_manager.hpp"
#include "mesh.cpp"
#include "gfx/vulkan/renderer_vulkan.cpp"
#include "platform/win32/win32_window.cpp"

#include <random>


// TODO: Implement better engine architecture
//			Move all related vulkan code api out of main function entirely
//			Do another function as an entry to the program and made main function callable?
//
// TODO: Implement correct materials per object instance
//			Implement objects that could emmit light
//			Also implement visualization of light source
//
// TODO: Distinction between static and dynamic meshes and their instances
//			Do I need to have 2 sets of every buffer which depends on vertices?
//
// TODO: Implement new instancing api for a mesh
//
// TODO: event system in the future?
//
// TODO: Implement mesh animations
// TODO: Implement reflections using cube maps
//
// TODO: Implement sound system with openal???
//           Find API to implement this
//
// TODO: Use scene class and use vector of scene classes for multiple scenes
//           Change between them at runtime and posibility to change them while main app is running(right now it is not quite work)
//
// TODO: Use only one staging buffer instead of many (a lot of memory usage here, current solution is totally not optimal)
// TODO: Implement mesh shading somewhere in the future
// TODO: Implement ray tracing pipeline in the future
// 
// TODO: Implement better while loop architecture (currently in work)
// TODO: Make everything to use memory allocator
// TODO: Reorganize files for the corresponding files. For ex. win32_window files to platform folders and etc.
// TODO: Minecraft like world rendering?
//			Voxels?



struct indirect_draw_indexed_command
{
	VkDrawIndexedIndirectCommand DrawArg; // 5
	u32 CommandIdx;
};

struct point_shadow_input
{
	mat4  LightMat;
	vec4  LightPos;
	float FarZ;
};

struct mesh_debug_input
{
	u32 DrawCount;
	u32 MeshCount;
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
	u32   DirectionalLightSourceCount;
	u32   PointLightSourceCount;
	u32   SpotLightSourceCount;
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	u32   DebugColors;
	u32   LightSourceShadowsEnabled;
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
	scene_manager SceneManager;

	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	view_data  ViewData = {};
	ViewData.ViewDir    = vec3(1, 0, 0);
	game_input GameInput{};
	GameInput.Buttons = VulkanWindow.Buttons;

	u32 GlobalMemorySize = MiB(128);
	void* MemoryBlock = malloc(GlobalMemorySize);

	float NearZ = 0.01f;
	float FarZ = 100.0f;

	//memory_heap GlobalHeap(VulkanWindow.Gfx, MiB(256));
	buffer GeometryOffsets(VulkanWindow.Gfx, KiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer GeometryDebugOffsets(VulkanWindow.Gfx, KiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	buffer MeshDrawCommandDataBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer MeshDrawVisibilityDataBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	buffer DebugMeshDrawCommandDataBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer DebugMeshDrawVisibilityDataBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	buffer IndirectDrawIndexedCommands(VulkanWindow.Gfx, MiB(16), true, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer DebugIndirectDrawIndexedCommands(VulkanWindow.Gfx, MiB(16), true, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer ShadowIndirectDrawIndexedCommands(VulkanWindow.Gfx, MiB(16), true, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	buffer VertexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer IndexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	buffer DebugVertexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer DebugIndexBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	global_world_data WorldUpdate = {};
	buffer WorldUpdateBuffer(VulkanWindow.Gfx, sizeof(global_world_data), false, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer LightSourcesBuffer(VulkanWindow.Gfx, sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT, false, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	texture::input_data TextureInputData = {};
	TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
	TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Alignment = 0;
	std::vector<texture> GlobalShadow(DEPTH_CASCADES_COUNT);
	for(texture& Shadow : GlobalShadow)
	{
		Shadow = texture(VulkanWindow.Gfx, PreviousPowerOfTwo(VulkanWindow.Gfx->Width) * 2, PreviousPowerOfTwo(VulkanWindow.Gfx->Width) * 2, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
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
	vec4  RandomAngles[Res][Res][Res] = {};
	for(u32 x = 0; x < Res; x++)
	{
		for(u32 y = 0; y < Res; y++)
		{
			for(u32 z = 0; z < Res; z++)
			{
				RandomAngles[x][y][z] = vec4(float(rand()) / RAND_MAX * 2.0f / Pi, 
											 float(rand()) / RAND_MAX * 2.0f / Pi,
											 float(rand()) / RAND_MAX * 2.0f / Pi, 1);
			}
		}
	}
	vec2 RandomRotations[16] = {};
	for(u32 RotIdx = 0; RotIdx < 16; ++RotIdx)
	{
		RandomRotations[RotIdx] = vec2(float(rand()) / RAND_MAX * 2.0 - 1.0, float(rand()) / RAND_MAX * 2.0 - 1.0);
	}
	TextureInputData.Format    = VK_FORMAT_R32G32B32A32_SFLOAT;
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
		Sample = Sample *  float(rand());
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
	TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SNORM;
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
	MeshCompCullingCommonData.FrustrumCullingEnabled  = true;
	MeshCompCullingCommonData.OcclusionCullingEnabled = true;
	MeshCompCullingCommonData.NearZ = NearZ;
	buffer MeshCommonCullingData(VulkanWindow.Gfx, sizeof(mesh_comp_culling_common_input), false, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	buffer MeshDrawCommandBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer MeshDrawShadowCommandBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	buffer MeshDrawDebugCommandBuffer(VulkanWindow.Gfx, MiB(16), false, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// TODO: Check and fix this new implementation. In theory, this should be correct
	shader_input GfxRootSignature;
	GfxRootSignature.PushStorageBuffer()->				// Vertex Data
					 PushUniformBuffer()->				// World Global Data
					 PushStorageBuffer()->				// MeshDrawCommands
					 PushImageSampler(1, 0, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Diffuse Texture
					 PushImageSampler(1, 0, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Normal Map Texture
					 PushImageSampler(1, 0, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Specular Map Texture
					 PushImageSampler(1, 0, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Height Map Texture
					 Build(VulkanWindow.Gfx, 0, true)->
					 BuildAll(VulkanWindow.Gfx);

	shader_input ColorPassRootSignature;
	ColorPassRootSignature.PushUniformBuffer()->					// World Global Data
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
						   PushConstant((DEPTH_CASCADES_COUNT + 1) * sizeof(float))-> // Cascade Splits
						   Build(VulkanWindow.Gfx, 0, true)->
						   Build(VulkanWindow.Gfx, 1)->
						   BuildAll(VulkanWindow.Gfx);

	shader_input AmbientOcclusionRootSignature;
	AmbientOcclusionRootSignature.PushUniformBuffer()->					// World Global Data
								  PushStorageBuffer()->					// Poisson Disk
								  PushImageSampler()->					// Random Rotations
								  PushImageSampler(GBUFFER_COUNT)->		// G-Buffer Vertex Position Data
																		// G-Buffer Vertex Normal Data
																		// G-Buffer Fragment Normal Data
																		// G-Buffer Diffuse Data
																		// G-Buffer Specular Data
								  PushStorageImage()->					// Ambient Occlusion Target Texture
								  Build(VulkanWindow.Gfx, 0, true)->
								  BuildAll(VulkanWindow.Gfx);

	shader_input DebugRootSignature;
	DebugRootSignature.PushStorageBuffer()->
					   PushUniformBuffer()->
					   PushStorageBuffer()->
					   Build(VulkanWindow.Gfx, 0, true)->
					   BuildAll(VulkanWindow.Gfx);

	shader_input DebugComputeRootSignature;
	DebugComputeRootSignature.PushStorageBuffer()->		// Mesh Offsets
							  PushStorageBuffer()->		// Draw Command Input
							  PushStorageBuffer()->		// Draw Command Visibility
							  PushStorageBuffer()->		// Indirect Draw Indexed Command
							  PushStorageBuffer()->		// Indirect Draw Indexed Command Counter
							  PushStorageBuffer()->		// Draw Commands 
							  PushConstant(sizeof(mesh_debug_input), VK_SHADER_STAGE_COMPUTE_BIT)->
							  Build(VulkanWindow.Gfx, 0, true)->
							  BuildAll(VulkanWindow.Gfx);

	shader_input ShadowSignature;
	ShadowSignature.PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
					PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
					PushConstant(sizeof(mat4))->
				    Build(VulkanWindow.Gfx, 0, true)->
					BuildAll(VulkanWindow.Gfx);

	shader_input PointShadowSignature;
	PointShadowSignature.PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
						 PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
						 PushConstant(sizeof(point_shadow_input))->
						 Build(VulkanWindow.Gfx, 0, true)->
						 BuildAll(VulkanWindow.Gfx);

	shader_input CmpIndirectFrustRootSignature;
	CmpIndirectFrustRootSignature.
					 PushStorageBuffer()->				// Mesh Offsets
					 PushStorageBuffer()->				// Draw Command Input
					 PushStorageBuffer()->				// Draw Command Visibility
					 PushStorageBuffer()->				// Indirect Draw Indexed Command
					 PushStorageBuffer()->				// Indirect Draw Indexed Command Counter
					 PushStorageBuffer()->				// Shadow Indirect Draw Indexed Command
					 PushStorageBuffer()->				// Shadow Indirect Draw Indexed Command Counter
					 PushStorageBuffer()->				// Mesh Draw Command Data
					 PushStorageBuffer()->				// Mesh Draw Shadow Command Data
					 PushStorageBuffer()->				// Common Input
				     Build(VulkanWindow.Gfx, 0, true)->
					 BuildAll(VulkanWindow.Gfx);

	shader_input CmpIndirectOcclRootSignature;
	CmpIndirectOcclRootSignature.
					 PushStorageBuffer()->				// Mesh Offsets
					 PushStorageBuffer()->				// Draw Command Input
					 PushStorageBuffer()->				// Draw Command Visibility
					 PushStorageBuffer()->				// Mesh Draw Command Data
					 PushStorageBuffer()->				// Common Input
					 PushImageSampler()->				// Hierarchy-Z depth texture
				     Build(VulkanWindow.Gfx, 0, true)->
					 BuildAll(VulkanWindow.Gfx);

	shader_input CmpReduceRootSignature;
	CmpReduceRootSignature.PushImageSampler(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->		// Input  Texture
						   PushStorageImage(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->		// Output Texture
						   PushConstant(sizeof(vec2), VK_SHADER_STAGE_COMPUTE_BIT)->	// Output Texture Size
						   Build(VulkanWindow.Gfx, 0, true)->
						   BuildAll(VulkanWindow.Gfx);

	shader_input BlurRootSignature;
	BlurRootSignature.PushImageSampler(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->			// Input  Texture
					  PushStorageImage(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->			// Output Texture
					  PushConstant(sizeof(vec3), VK_SHADER_STAGE_COMPUTE_BIT)->		// Texture Size, Conv Size 
					  Build(VulkanWindow.Gfx, 0, true)->
					  BuildAll(VulkanWindow.Gfx);

	// TODO: Maybe create hot load shaders and compile them and runtime inside
	std::vector<VkFormat> GfxFormats;
	for(u32 FormatIdx = 0; FormatIdx < GBuffer.size(); ++FormatIdx) GfxFormats.push_back(GBuffer[FormatIdx].Info.Format);
	global_pipeline_context PipelineContext(VulkanWindow.Gfx);
	render_context GfxContext(VulkanWindow.Gfx, GfxRootSignature, {"..\\build\\shaders\\mesh.vert.spv", "..\\build\\shaders\\mesh.frag.spv"}, GfxFormats);

	render_context::input_data RendererInputData = {};
	RendererInputData.UseColor	  = true;
	RendererInputData.UseDepth	  = true;
	RendererInputData.UseBackFace = true;
	RendererInputData.UseOutline  = true;
	render_context  DebugContext(VulkanWindow.Gfx, DebugRootSignature, {"..\\build\\shaders\\mesh.dbg.vert.spv", "..\\build\\shaders\\mesh.dbg.frag.spv"}, {GfxColorTarget.Info.Format}, RendererInputData);

	RendererInputData = {};
	RendererInputData.UseDepth = true;
	RendererInputData.UseBackFace  = true;
	render_context  CascadeShadowContext(VulkanWindow.Gfx, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);
	render_context  ShadowContext(VulkanWindow.Gfx, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);

	std::vector<render_context> CubeMapShadowContexts;
	RendererInputData.UseMultiview = true;
	for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
	{
		RendererInputData.ViewMask = 1 << CubeMapFaceIdx;
		CubeMapShadowContexts.push_back(render_context(VulkanWindow.Gfx, PointShadowSignature, {"..\\build\\shaders\\mesh.pnt.sdw.vert.spv", "..\\build\\shaders\\mesh.pnt.sdw.frag.spv"}, {}, RendererInputData));
	}

	RendererInputData.UseDepth	   = true;
	RendererInputData.UseMultiview = false;
	RendererInputData.ViewMask	   = 0;
	render_context  DebugCameraViewContext(VulkanWindow.Gfx, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);

	compute_context ColorPassContext(VulkanWindow.Gfx, ColorPassRootSignature, "..\\build\\shaders\\color_pass.comp.spv");
	compute_context DebugComputeContext(VulkanWindow.Gfx, DebugComputeRootSignature, "..\\build\\shaders\\mesh.dbg.comp.spv"); // 108
	compute_context AmbientOcclusionContext(VulkanWindow.Gfx, AmbientOcclusionRootSignature, "..\\build\\shaders\\screen_space_ambient_occlusion.comp.spv"); // 10a
	compute_context FrustCullingContext(VulkanWindow.Gfx, CmpIndirectFrustRootSignature, "..\\build\\shaders\\indirect_cull_frust.comp.spv");
	compute_context OcclCullingContext(VulkanWindow.Gfx, CmpIndirectOcclRootSignature, "..\\build\\shaders\\indirect_cull_occl.comp.spv");
	compute_context DepthReduceContext(VulkanWindow.Gfx, CmpReduceRootSignature, "..\\build\\shaders\\depth_reduce.comp.spv");
	compute_context BlurContext(VulkanWindow.Gfx, BlurRootSignature, "..\\build\\shaders\\blur.comp.spv");

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

	std::vector<vec3> CubeDirections = 
	{
		vec3( 1.0,  0.0,  0.0),
		vec3(-1.0,  0.0,  0.0),
		vec3( 0.0, -1.0,  0.0),
		vec3( 0.0,  1.0,  0.0),
		vec3( 0.0,  0.0,  1.0),
		vec3( 0.0,  0.0, -1.0),
	};
	std::vector<vec3> CubeMapUpVectors = 
	{
		vec3(0.0, -1.0,  0.0), // +X
		vec3(0.0, -1.0,  0.0), // -X
		vec3(0.0,  0.0, -1.0), // +Y
		vec3(0.0,  0.0,  1.0), // -Y
		vec3(0.0, -1.0,  0.0), // +Z
		vec3(0.0, -1.0,  0.0), // -Z
	};
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

	std::vector<texture> LightShadows;
	std::vector<texture> PointLightShadows;

	// TODO: Transient material storage
	// TODO: create necessary textures on scene load. Maybe think about something better on the architecture here
	// Every type of the textures(diffuse, specular, normal etc) should be here?
	std::vector<texture> Textures;
	texture_data Diffuse1("..\\assets\\bricks4\\brick-wall.diff.tga");
	texture_data Normal1("..\\assets\\bricks4\\brick-wall.norm.tga");
	texture_data Specular1("..\\assets\\bricks4\\brick-wall.spec.tga");
	texture_data Height1("..\\assets\\bricks4\\brick-wall.disp.png");

	TextureInputData.Format    = VK_FORMAT_R8G8B8A8_SRGB;
	TextureInputData.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
	TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
	TextureInputData.MipLevels = 1;
	TextureInputData.Layers    = 1;
	TextureInputData.Alignment = 0;
	Textures.push_back(texture(VulkanWindow.Gfx, Diffuse1.Data, Diffuse1.Width, Diffuse1.Height, 1, TextureInputData));
	Textures.push_back(texture(VulkanWindow.Gfx, Normal1.Data, Normal1.Width, Normal1.Height, 1, TextureInputData));
	Textures.push_back(texture(VulkanWindow.Gfx, Specular1.Data, Normal1.Width, Normal1.Height, 1, TextureInputData));
	Textures.push_back(texture(VulkanWindow.Gfx, Height1.Data, Height1.Width, Height1.Height, 1, TextureInputData));
	Diffuse1.Delete();
	Normal1.Delete();
	Specular1.Delete();
	Height1.Delete();

	u32 DirectionalLightSourceCount = 0;
	u32 PointLightSourceCount = 0;
	u32 SpotLightSourceCount = 0;
	float  FOV = 45.0f;
	double TimeLast = window::GetTimestamp();
	double TimeElapsed = 0.0;
	double TimeEnd = 0.0;
	double AvgCpuTime = 0.0;
	WorldUpdate.LightSourceShadowsEnabled = false;
	while(VulkanWindow.IsRunning())
	{
		mesh DebugGeometries;

		linear_allocator SystemsAllocator(GlobalMemorySize, MemoryBlock);
		linear_allocator LightSourcesAlloc(sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT, SystemsAllocator.Allocate(sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT));
		linear_allocator GlobalMeshInstancesAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator GlobalMeshVisibleAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator DebugMeshInstancesAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));
		linear_allocator DebugMeshVisibleAlloc(MiB(16), SystemsAllocator.Allocate(MiB(16)));

		// TODO: Use memory allocator with pre-allocated memory
		std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>> GlobalMeshInstances(GlobalMeshInstancesAlloc);
		std::vector<u32, allocator_adapter<u32, linear_allocator>> GlobalMeshVisibility(GlobalMeshVisibleAlloc);
		std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>> DebugMeshInstances(DebugMeshInstancesAlloc);
		std::vector<u32, allocator_adapter<u32, linear_allocator>> DebugMeshVisibility(DebugMeshVisibleAlloc);
		std::vector<light_source, allocator_adapter<light_source, linear_allocator>> GlobalLightSources(LightSourcesAlloc);
		
		auto Result = VulkanWindow.ProcessMessages();
		if(Result) return *Result;

		SceneManager.UpdateScenes();
		if(!SceneManager.IsCurrentSceneInitialized()) continue;

		GameInput.DeltaTime = TimeElapsed;

		SceneManager.StartScene();
		DebugGeometries.Load(SceneManager.GlobalDebugGeometries);
		SceneManager.UpdateScene(GlobalMeshInstances, GlobalMeshVisibility, DebugMeshInstances, DebugMeshVisibility, GlobalLightSources);

		assert(GlobalLightSources.size() <= LIGHT_SOURCES_MAX_COUNT);
		if((LightShadows.size() + PointLightShadows.size()) != GlobalLightSources.size())
		{
			DirectionalLightSourceCount = 0;
			PointLightSourceCount = 0;
			SpotLightSourceCount = 0;

			LightShadows.clear();
			PointLightShadows.clear();

			TextureInputData.Format = VK_FORMAT_D32_SFLOAT;
			TextureInputData.Usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			for(const light_source& LightSource : GlobalLightSources)
			{
				if(LightSource.LightType == light_type_point)
				{
					TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_CUBE;
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 6;
					if(WorldUpdate.LightSourceShadowsEnabled)
						PointLightShadows.push_back(texture(VulkanWindow.Gfx, VulkanWindow.Width, VulkanWindow.Height, 1, TextureInputData));
					PointLightSourceCount++;
				}
				else if(LightSource.LightType == light_type_spot)
				{
					TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 1;
					if(WorldUpdate.LightSourceShadowsEnabled)
						LightShadows.push_back(texture(VulkanWindow.Gfx, VulkanWindow.Width, VulkanWindow.Height, 1, TextureInputData));
					SpotLightSourceCount++;
				}
				else if(LightSource.LightType == light_type_directional)
				{
					TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 1;
					if(WorldUpdate.LightSourceShadowsEnabled)
						LightShadows.push_back(texture(VulkanWindow.Gfx, VulkanWindow.Width, VulkanWindow.Height, 1, TextureInputData));
					DirectionalLightSourceCount++;
				}
			}

			ColorPassContext.SetImageSampler(LightShadows, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
			ColorPassContext.SetImageSampler(PointLightShadows, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
			ColorPassContext.StaticUpdate();
		}

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

		float Aspect  = (float)VulkanWindow.Gfx->Width / (float)VulkanWindow.Gfx->Height;
		vec3 LightPos =  vec3(-4, 4, 2);
		vec3 LightDir = -Normalize(LightPos);

		mat4 CameraProj = PerspRH(FOV, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, NearZ, FarZ);
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

		WorldUpdate.LightSourceShadowsEnabled   = false;
		WorldUpdate.DirectionalLightSourceCount = DirectionalLightSourceCount;
		WorldUpdate.PointLightSourceCount		= PointLightSourceCount;
		WorldUpdate.SpotLightSourceCount		= SpotLightSourceCount;

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

			mat4 CameraCascadeProj = PerspRH(FOV, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, LightNearZ, LightFarZ);
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
			DebugMeshInstances.push_back({{vec4(CascadeColors[CascadeIdx - 1], 1), 0, 0, 0, 0}, vec4(0), vec4(1), DebugGeometries.Load(DebugFrustum, CubeIndices)});
			DebugMeshVisibility.push_back(true);

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
		MeshCompCullingCommonData.MeshCount = SceneManager.GlobalGeometries.MeshCount;
		MeshCompCullingCommonData.Proj = CameraProj;
		MeshCompCullingCommonData.View = WorldUpdate.DebugView;

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
			ViewData.CameraPos += (ViewData.ViewDir * 4.0f*CameraSpeed);
		}
		if(GameInput.Buttons[EC_S].IsDown)
		{
			ViewData.CameraPos -= (ViewData.ViewDir * 4.0f*CameraSpeed);
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
			quat RotQuat( CameraSpeed, U);
			ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		if(GameInput.Buttons[EC_DOWN].IsDown)
		{
			quat ViewDirQuat(ViewData.ViewDir, 0);
			quat RotQuat(-CameraSpeed, U);
			ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}

		if(!VulkanWindow.IsGfxPaused)
		{
			PipelineContext.Begin(VulkanWindow.Gfx);

			{
				std::vector<std::tuple<buffer&, VkAccessFlags, VkAccessFlags>> SetupBufferBarrier = 
				{
					{GeometryOffsets, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{VertexBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{GeometryDebugOffsets, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{DebugVertexBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{MeshDrawCommandDataBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{DebugMeshDrawCommandDataBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{WorldUpdateBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{MeshCommonCullingData, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
					{LightSourcesBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
				};
				if(IsFirstFrame)
				{
					SetupBufferBarrier.push_back({MeshDrawVisibilityDataBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT});
					SetupBufferBarrier.push_back({DebugMeshDrawVisibilityDataBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT});
				}
				PipelineContext.SetBufferBarriers(SetupBufferBarrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

				// TODO: Optimize the updates(update only when the actual data have been changed etc.)
				if(IsFirstFrame)
				{
					// NOTE: Update visibility buffer if the amount of instances was changed
					MeshDrawVisibilityDataBuffer.UpdateSize(VulkanWindow.Gfx, GlobalMeshVisibility.data(), GlobalMeshVisibility.size() * sizeof(u32), PipelineContext);
					DebugMeshDrawVisibilityDataBuffer.UpdateSize(VulkanWindow.Gfx, DebugMeshVisibility.data(), DebugMeshVisibility.size() * sizeof(u32), PipelineContext);
													  
				}

				GeometryOffsets.UpdateSize(VulkanWindow.Gfx, SceneManager.GlobalGeometries.Offsets.data(), SceneManager.GlobalGeometries.Offsets.size() * sizeof(mesh::offset), PipelineContext);
				VertexBuffer.UpdateSize(VulkanWindow.Gfx, SceneManager.GlobalGeometries.Vertices.data(), SceneManager.GlobalGeometries.Vertices.size() * sizeof(vertex), PipelineContext);
				IndexBuffer.UpdateSize(VulkanWindow.Gfx, SceneManager.GlobalGeometries.VertexIndices.data(), SceneManager.GlobalGeometries.VertexIndices.size() * sizeof(u32), PipelineContext);

				GeometryDebugOffsets.UpdateSize(VulkanWindow.Gfx, DebugGeometries.Offsets.data(), DebugGeometries.Offsets.size() * sizeof(mesh::offset), PipelineContext);
				DebugVertexBuffer.UpdateSize(VulkanWindow.Gfx, DebugGeometries.Vertices.data(), DebugGeometries.Vertices.size() * sizeof(vertex), PipelineContext);
				DebugIndexBuffer.UpdateSize(VulkanWindow.Gfx, DebugGeometries.VertexIndices.data(), DebugGeometries.VertexIndices.size() * sizeof(u32), PipelineContext);

				MeshDrawCommandDataBuffer.UpdateSize(VulkanWindow.Gfx, GlobalMeshInstances.data(), GlobalMeshInstances.size() * sizeof(mesh_draw_command_input), PipelineContext);
				DebugMeshDrawCommandDataBuffer.UpdateSize(VulkanWindow.Gfx, DebugMeshInstances.data(), DebugMeshInstances.size() * sizeof(mesh_draw_command_input), PipelineContext);

				WorldUpdateBuffer.Update(VulkanWindow.Gfx, (void*)&WorldUpdate, PipelineContext);
				MeshCommonCullingData.Update(VulkanWindow.Gfx, (void*)&MeshCompCullingCommonData, PipelineContext);
				LightSourcesBuffer.UpdateSize(VulkanWindow.Gfx, GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), PipelineContext);
			}

			{
				FrustCullingContext.Begin(PipelineContext);
				PipelineContext.SetBufferBarriers({
													  {GeometryOffsets, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
													  {MeshDrawCommandDataBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
													  {MeshDrawVisibilityDataBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
													  {IndirectDrawIndexedCommands, 0, VK_ACCESS_SHADER_WRITE_BIT},
													  {ShadowIndirectDrawIndexedCommands, 0, VK_ACCESS_SHADER_WRITE_BIT},
													  {MeshCommonCullingData, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
												  }, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				FrustCullingContext.SetStorageBufferView(GeometryOffsets);
				FrustCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
				FrustCullingContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
				FrustCullingContext.SetStorageBufferView(IndirectDrawIndexedCommands);
				FrustCullingContext.SetStorageBufferView(ShadowIndirectDrawIndexedCommands);
				FrustCullingContext.SetStorageBufferView(MeshDrawCommandBuffer);
				FrustCullingContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
				FrustCullingContext.SetStorageBufferView(MeshCommonCullingData);

				FrustCullingContext.Execute(GlobalMeshInstances.size());

				FrustCullingContext.End();
			}

			PipelineContext.SetBufferBarriers({
												  {GeometryDebugOffsets, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
												  {DebugMeshDrawCommandDataBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
												  {DebugMeshDrawVisibilityDataBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
												  {DebugIndirectDrawIndexedCommands, 0, VK_ACCESS_SHADER_WRITE_BIT},
												  {MeshDrawDebugCommandBuffer, 0, VK_ACCESS_SHADER_WRITE_BIT},
											  }, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			{
				mesh_debug_input Input = {};
				Input.MeshCount = DebugGeometries.MeshCount;
				Input.DrawCount = DebugMeshInstances.size();

				DebugComputeContext.Begin(PipelineContext);

				DebugComputeContext.SetStorageBufferView(GeometryDebugOffsets);
				DebugComputeContext.SetStorageBufferView(DebugMeshDrawCommandDataBuffer);
				DebugComputeContext.SetStorageBufferView(DebugMeshDrawVisibilityDataBuffer);
				DebugComputeContext.SetStorageBufferView(DebugIndirectDrawIndexedCommands);
				DebugComputeContext.SetStorageBufferView(MeshDrawDebugCommandBuffer);
				DebugComputeContext.SetConstant(&Input, sizeof(mesh_debug_input));

				DebugComputeContext.Execute(DebugMeshInstances.size());

				DebugComputeContext.End();
			}

			// TODO: this should be moved to directional light calculations I guess
			//		 or make it possible to turn off
			for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
			{
				mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];
				std::vector<VkImageMemoryBarrier> ShadowBarrier = 
				{
					CreateImageBarrier(GlobalShadow[CascadeIdx].Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);
				PipelineContext.SetBufferBarriers({{IndexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
				PipelineContext.SetBufferBarriers({{VertexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
				PipelineContext.SetBufferBarriers({{MeshDrawShadowCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
				PipelineContext.SetBufferBarriers({{ShadowIndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

				CascadeShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, GlobalShadow[CascadeIdx].Width, GlobalShadow[CascadeIdx].Height, GlobalShadow[CascadeIdx], {1, 0});
				CascadeShadowContext.Begin(VulkanWindow.Gfx, PipelineContext, GlobalShadow[CascadeIdx].Width, GlobalShadow[CascadeIdx].Height);

				CascadeShadowContext.SetStorageBufferView(VertexBuffer);
				CascadeShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
				CascadeShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
				CascadeShadowContext.DrawIndirect<indirect_draw_indexed_command>(SceneManager.GlobalGeometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

				CascadeShadowContext.End();
			}

			// TODO: something better or/and efficient here
			// TODO: render only when the actual light source have been changed
			u32 DirectionalSourceIdx = 0;
			u32 PointLightSourceIdx  = 0;
			u32 SpotLightSourceIdx   = 0;
			if(WorldUpdate.LightSourceShadowsEnabled)
			{
				for(const light_source& LightSource : GlobalLightSources)
				{
					if(LightSource.LightType == light_type_directional)
					{
						const texture& ShadowMapTexture = LightShadows[DirectionalSourceIdx + SpotLightSourceIdx];
						std::vector<VkImageMemoryBarrier> ShadowBarrier = 
						{
							CreateImageBarrier(ShadowMapTexture.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
						};
						ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

						mat4 ShadowMapProj = PerspRH(FOV, ShadowMapTexture.Width, ShadowMapTexture.Height, NearZ, FarZ);
						mat4 ShadowMapView = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + vec3(LightSource.Dir), UpVector);
						mat4 Shadow = ShadowMapView * ShadowMapProj;

						ShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0});
						ShadowContext.Begin(VulkanWindow.Gfx, PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

						ShadowContext.SetStorageBufferView(VertexBuffer);
						ShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
						ShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
						ShadowContext.DrawIndirect<indirect_draw_indexed_command>(SceneManager.GlobalGeometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

						ShadowContext.End();
						DirectionalSourceIdx++;
					}
					else if(LightSource.LightType == light_type_point)
					{
						const texture& ShadowMapTexture = PointLightShadows[PointLightSourceIdx];
						std::vector<VkImageMemoryBarrier> ShadowBarrier = 
						{
							CreateImageBarrier(ShadowMapTexture.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
						};
						ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

						for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
						{
							vec3 CubeMapFaceDir = CubeDirections[CubeMapFaceIdx];
							vec3 CubeMapUpVect  = CubeMapUpVectors[CubeMapFaceIdx];
							mat4 ShadowMapProj  = PerspRH(90.0f, ShadowMapTexture.Width, ShadowMapTexture.Height, NearZ, FarZ);
							mat4 ShadowMapView  = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + CubeMapFaceDir, CubeMapUpVect);
							mat4 Shadow = ShadowMapView * ShadowMapProj;
							point_shadow_input PointShadowInput = {};
							PointShadowInput.LightPos = GlobalLightSources[DirectionalSourceIdx + PointLightSourceIdx + SpotLightSourceIdx].Pos;
							PointShadowInput.LightMat = Shadow;
							PointShadowInput.FarZ = FarZ;

							CubeMapShadowContexts[CubeMapFaceIdx].SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0}, CubeMapFaceIdx, true);
							CubeMapShadowContexts[CubeMapFaceIdx].Begin(VulkanWindow.Gfx, PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

							CubeMapShadowContexts[CubeMapFaceIdx].SetStorageBufferView(VertexBuffer);
							CubeMapShadowContexts[CubeMapFaceIdx].SetStorageBufferView(MeshDrawShadowCommandBuffer);
							CubeMapShadowContexts[CubeMapFaceIdx].SetConstant((void*)&PointShadowInput, sizeof(point_shadow_input));
							CubeMapShadowContexts[CubeMapFaceIdx].DrawIndirect<indirect_draw_indexed_command>(SceneManager.GlobalGeometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

							CubeMapShadowContexts[CubeMapFaceIdx].End();
						}
						PointLightSourceIdx++;
					}
					else if(LightSource.LightType == light_type_spot)
					{
						const texture& ShadowMapTexture = LightShadows[DirectionalSourceIdx + SpotLightSourceIdx];
						std::vector<VkImageMemoryBarrier> ShadowBarrier = 
						{
							CreateImageBarrier(ShadowMapTexture.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
						};
						ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);
						mat4 ShadowMapProj = PerspRH(LightSource.Pos.w, ShadowMapTexture.Width, ShadowMapTexture.Height, NearZ, FarZ);
						mat4 ShadowMapView = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + vec3(LightSource.Dir), UpVector);
						mat4 Shadow = ShadowMapView * ShadowMapProj;

						ShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0});
						ShadowContext.Begin(VulkanWindow.Gfx, PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

						ShadowContext.SetStorageBufferView(VertexBuffer);
						ShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
						ShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
						ShadowContext.DrawIndirect<indirect_draw_indexed_command>(SceneManager.GlobalGeometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

						ShadowContext.End();
						SpotLightSourceIdx++;
					}
				}
			}

			// NOTE: This is only for debug. Maybe not compile on release mode???
			{
				mat4 Shadow = DebugCameraView * CameraProj;
				std::vector<VkImageMemoryBarrier> ShadowBarrier = 
				{
					CreateImageBarrier(DebugCameraViewDepthTarget.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);
				PipelineContext.SetBufferBarriers({{MeshDrawCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
				PipelineContext.SetBufferBarriers({{IndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

				DebugCameraViewContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, DebugCameraViewDepthTarget.Width, DebugCameraViewDepthTarget.Height, DebugCameraViewDepthTarget, {1, 0});
				DebugCameraViewContext.Begin(VulkanWindow.Gfx, PipelineContext, DebugCameraViewDepthTarget.Width, DebugCameraViewDepthTarget.Height);

				DebugCameraViewContext.SetStorageBufferView(VertexBuffer);
				DebugCameraViewContext.SetStorageBufferView(MeshDrawCommandBuffer);
				DebugCameraViewContext.SetConstant((void*)&Shadow, sizeof(mat4));
				DebugCameraViewContext.DrawIndirect<indirect_draw_indexed_command>(SceneManager.GlobalGeometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

				DebugCameraViewContext.End();
			}

			{
				std::vector<VkImageMemoryBarrier> ImageBeginRenderBarriers;
				for(u32 Idx = 0;
					Idx < Textures.size();
					++Idx)
				{
					ImageBeginRenderBarriers.push_back(CreateImageBarrier(Textures[Idx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
				}
				for(texture& ColorTarget : GBuffer)
					ImageBeginRenderBarriers.push_back(CreateImageBarrier(ColorTarget.Handle, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(GfxDepthTarget.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ImageBeginRenderBarriers);
				PipelineContext.SetBufferBarrier({WorldUpdateBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				GfxContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GBuffer, {0, 0, 0, 0});
				GfxContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GfxDepthTarget, {1, 0});
				GfxContext.Begin(VulkanWindow.Gfx, PipelineContext, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height);

				GfxContext.SetStorageBufferView(VertexBuffer);
				GfxContext.SetUniformBufferView(WorldUpdateBuffer);
				GfxContext.SetStorageBufferView(MeshDrawCommandBuffer);
				GfxContext.SetImageSampler({Textures[0]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.SetImageSampler({Textures[1]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.SetImageSampler({Textures[2]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.SetImageSampler({Textures[3]}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				GfxContext.DrawIndirect<indirect_draw_indexed_command>(SceneManager.GlobalGeometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

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

			// NOTE: Horizontal blur
			{
				BlurContext.Begin(PipelineContext);

				std::vector<VkImageMemoryBarrier> BlurPassBarrier = 
				{
					CreateImageBarrier(AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, BlurPassBarrier);

				vec3 BlurInput(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 1.0);
				BlurContext.SetImageSampler({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
				BlurContext.SetStorageImage({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
				BlurContext.SetConstant((void*)BlurInput.E, sizeof(vec3));
				BlurContext.Execute(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height);

				BlurContext.End();
			}

			// NOTE: Vertical blur
			{
				BlurContext.Begin(PipelineContext);

				std::vector<VkImageMemoryBarrier> BlurPassBarrier = 
				{
					CreateImageBarrier(AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, BlurPassBarrier);

				vec3 BlurInput(VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, 0.0);
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
					CreateImageBarrier(AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
					CreateImageBarrier(RandomAnglesTexture.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				};
				for(u32 CascadeIdx = 0;
					CascadeIdx < DEPTH_CASCADES_COUNT;
					++CascadeIdx)
				{
					ColorPassBarrier.push_back(CreateImageBarrier(GlobalShadow[CascadeIdx].Handle, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
				}
				for(u32 ShadowMapIdx = 0; ShadowMapIdx < PointLightShadows.size(); ShadowMapIdx++)
				{
					ColorPassBarrier.push_back(CreateImageBarrier(PointLightShadows[ShadowMapIdx].Handle, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
				}
				for(u32 ShadowMapIdx = 0; ShadowMapIdx < LightShadows.size(); ShadowMapIdx++)
				{
					ColorPassBarrier.push_back(CreateImageBarrier(LightShadows[ShadowMapIdx].Handle, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
				}
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, ColorPassBarrier);
				PipelineContext.SetBufferBarriers({{LightSourcesBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				PipelineContext.SetBufferBarriers({{PoissonDiskBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

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
					CreateImageBarrier(DepthPyramid.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
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
						CreateImageBarrier(DepthPyramid.Handle, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
					};
					ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DepthPyramidBarrier);
				}
				DepthReduceContext.End();
			}

			{
				VkAccessFlags ColorSrcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				VkAccessFlags DepthSrcAccessMask = VK_ACCESS_SHADER_READ_BIT;

				VkImageLayout ColorOldLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkImageLayout DepthOldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkPipelineStageFlags SrcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				VkPipelineStageFlags DstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

				std::vector<VkImageMemoryBarrier> ImageBeginRenderBarriers = 
				{
					CreateImageBarrier(GfxColorTarget.Handle, ColorSrcAccessMask, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, ColorOldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
					CreateImageBarrier(GfxDepthTarget.Handle, DepthSrcAccessMask, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, DepthOldLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT)
				};
				ImageBarrier(*PipelineContext.CommandList, SrcStageMask, DstStageMask, ImageBeginRenderBarriers);
				PipelineContext.SetBufferBarriers({{DebugIndexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
				PipelineContext.SetBufferBarriers({{DebugVertexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
				PipelineContext.SetBufferBarriers({{MeshDrawDebugCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
				PipelineContext.SetBufferBarriers({{DebugIndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

				DebugContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, {GfxColorTarget}, {0, 0, 0, 1});
				DebugContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height, GfxDepthTarget, {1, 0});
				DebugContext.Begin(VulkanWindow.Gfx, PipelineContext, VulkanWindow.Gfx->Width, VulkanWindow.Gfx->Height);

				DebugContext.SetStorageBufferView(DebugVertexBuffer);
				DebugContext.SetUniformBufferView(WorldUpdateBuffer);
				DebugContext.SetStorageBufferView(MeshDrawDebugCommandBuffer);
				DebugContext.DrawIndirect<indirect_draw_indexed_command>(DebugGeometries.MeshCount, DebugIndexBuffer, DebugIndirectDrawIndexedCommands);

				DebugContext.End();
			}

			{
				OcclCullingContext.Begin(PipelineContext);

				OcclCullingContext.SetStorageBufferView(GeometryOffsets);
				OcclCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
				OcclCullingContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
				OcclCullingContext.SetStorageBufferView(MeshDrawCommandBuffer);
				OcclCullingContext.SetStorageBufferView(MeshCommonCullingData);
				OcclCullingContext.SetImageSampler({DepthPyramid}, VK_IMAGE_LAYOUT_GENERAL);

				OcclCullingContext.Execute(GlobalMeshInstances.size());

				OcclCullingContext.End();
			}

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
