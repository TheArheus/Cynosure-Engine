#pragma once

// TODO: Recreate buffers on updates
// TODO: Don't forget about object destruction
struct render_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	vec3 CubeMapDirections[6];
	vec3 CubeMapUpVectors[6];

	std::vector<texture> GlobalShadow;
	std::vector<texture> GBuffer;

	std::vector<mesh::material> Materials;
	std::vector<texture> DiffuseTextures;
	std::vector<texture> NormalTextures;
	std::vector<texture> SpecularTextures;
	std::vector<texture> HeightTextures;

	std::vector<texture> LightShadows;
	std::vector<texture> PointLightShadows;

	texture AmbientOcclusionData;
	texture DepthPyramid;
	texture RandomAnglesTexture;
	texture NoiseTexture;

	buffer GeometryOffsets;
	buffer WorldUpdateBuffer;
	buffer MeshCommonCullingInputBuffer;

	buffer MeshDrawCommandDataBuffer;
	buffer MeshDrawVisibilityDataBuffer;

	buffer IndirectDrawIndexedCommands;
	buffer ShadowIndirectDrawIndexedCommands;

	buffer MeshMaterialsBuffer;
	buffer LightSourcesBuffer;

	buffer VertexBuffer;
	buffer IndexBuffer;

	buffer PoissonDiskBuffer;
	buffer RandomSamplesBuffer;

	buffer MeshDrawCommandBuffer;
	buffer MeshDrawShadowCommandBuffer;

	shader_input ShadowSignature;
	shader_input GfxRootSignature;
	shader_input PointShadowSignature;
	shader_input ColorPassRootSignature;
	shader_input AmbientOcclusionRootSignature;
	shader_input ShadowComputeRootSignature;
	shader_input CmpIndirectFrustRootSignature;
	shader_input CmpIndirectOcclRootSignature;
	shader_input CmpReduceRootSignature;
	shader_input BlurRootSignature;

	std::vector<render_context> CubeMapShadowContexts;
	render_context GfxContext;
	render_context CascadeShadowContext;
	render_context ShadowContext;
	render_context DebugCameraViewContext;

	compute_context ColorPassContext;
	compute_context AmbientOcclusionContext;
	compute_context ShadowComputeContext;
	compute_context FrustCullingContext;
	compute_context OcclCullingContext;
	compute_context DepthReduceContext;
	compute_context BlurContext;

	system_constructor(render_system)
	{
		RequireComponent<mesh_component>();
		RequireComponent<static_instances_component>();

		GlobalShadow.resize(DEPTH_CASCADES_COUNT);
		GBuffer.resize(GBUFFER_COUNT);

		CubeMapDirections[0] = vec3( 1.0,  0.0,  0.0);
		CubeMapDirections[1] = vec3(-1.0,  0.0,  0.0);
		CubeMapDirections[2] = vec3( 0.0, -1.0,  0.0);
		CubeMapDirections[3] = vec3( 0.0,  1.0,  0.0);
		CubeMapDirections[4] = vec3( 0.0,  0.0,  1.0);
		CubeMapDirections[5] = vec3( 0.0,  0.0, -1.0);
		CubeMapUpVectors [0] = vec3( 0.0, -1.0,  0.0); // +X
		CubeMapUpVectors [1] = vec3( 0.0, -1.0,  0.0); // -X
		CubeMapUpVectors [2] = vec3( 0.0,  0.0, -1.0); // +Y
		CubeMapUpVectors [3] = vec3( 0.0,  0.0,  1.0); // -Y
		CubeMapUpVectors [4] = vec3( 0.0, -1.0,  0.0); // +Z
		CubeMapUpVectors [5] = vec3( 0.0, -1.0,  0.0); // -Z
	}

	void SubscribeToEvents(event_bus& Events)
	{
	}

	void Setup(window& Window, memory_heap& GlobalHeap, mesh_comp_culling_common_input& MeshCommonCullingInput)
	{
		texture_data Texture = {};
		mesh::material NewMaterial = {};
		texture::input_data TextureInputData = {};
		TextureInputData.Format    = VK_FORMAT_R8G8B8A8_SRGB;
		TextureInputData.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
		TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Layers    = 1;

		u32 TextureIdx   = 0;
		u32 NormalMapIdx = 0;
		u32 SpecularIdx  = 0;
		u32 HeightMapIdx = 0;
		for(entity& Entity : Entities)
		{
			NewMaterial = {};
			color_component* Color = Entity.GetComponent<color_component>();
			mesh_component* Mesh = Entity.GetComponent<mesh_component>();
			debug_component* Debug = Entity.GetComponent<debug_component>();
			diffuse_component* Diffuse = Entity.GetComponent<diffuse_component>();
			normal_map_component* NormalMap = Entity.GetComponent<normal_map_component>();
			specular_map_component* SpecularMap = Entity.GetComponent<specular_map_component>();
			height_map_component* HeightMap = Entity.GetComponent<height_map_component>();
			static_instances_component* InstancesComponent = Entity.GetComponent<static_instances_component>();

			NewMaterial.LightEmmit = vec4(1);
			if(Color)
			{
				NewMaterial.LightEmmit = vec4(Color->Data, 1);
			}
			if(Diffuse)
			{
				Texture.Load(Diffuse->Data);
				DiffuseTextures.push_back(GlobalHeap.PushTexture(Window.Gfx, Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasTexture = true;
				NewMaterial.TextureIdx = TextureIdx++;
				Texture.Delete();
			}
			if(NormalMap)
			{
				Texture.Load(NormalMap->Data);
				NormalTextures.push_back(GlobalHeap.PushTexture(Window.Gfx, Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasNormalMap = true;
				NewMaterial.NormalMapIdx = NormalMapIdx++;
				Texture.Delete();
			}
			if(SpecularMap)
			{
				Texture.Load(SpecularMap->Data);
				SpecularTextures.push_back(GlobalHeap.PushTexture(Window.Gfx, Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasSpecularMap = true;
				NewMaterial.SpecularMapIdx = SpecularIdx++;
				Texture.Delete();
			}
			if(HeightMap)
			{
				Texture.Load(HeightMap->Data);
				HeightTextures.push_back(GlobalHeap.PushTexture(Window.Gfx, Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasHeightMap = true;
				NewMaterial.HeightMapIdx = HeightMapIdx++;
				Texture.Delete();
			}

			if(Mesh)
			{
				Geometries.Load(Mesh->Data);
			}

			if(InstancesComponent)
			{
				for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
				{
					StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), *(u32*)&Entity.Handle });
					StaticMeshVisibility.push_back(true);
				}
			}

			Materials.push_back(NewMaterial);
		}

		MeshCommonCullingInput.DrawCount = StaticMeshInstances.size();
		MeshCommonCullingInput.MeshCount = Geometries.MeshCount;

		// TODO: Initial buffer setup. Recreate them if needed
		GeometryOffsets = GlobalHeap.PushBuffer(Window.Gfx, Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshDrawCommandDataBuffer = GlobalHeap.PushBuffer(Window.Gfx, StaticMeshInstances.data(), sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshDrawVisibilityDataBuffer = GlobalHeap.PushBuffer(Window.Gfx, StaticMeshVisibility.data(), sizeof(u32) * StaticMeshVisibility.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		IndirectDrawIndexedCommands = GlobalHeap.PushBuffer(Window.Gfx, sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		ShadowIndirectDrawIndexedCommands = GlobalHeap.PushBuffer(Window.Gfx, sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshMaterialsBuffer = GlobalHeap.PushBuffer(Window.Gfx, Materials.data(), Materials.size() * sizeof(mesh::material), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		VertexBuffer = GlobalHeap.PushBuffer(Window.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		IndexBuffer = GlobalHeap.PushBuffer(Window.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32), false, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshDrawCommandBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshDrawShadowCommandBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		LightSourcesBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		WorldUpdateBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(global_world_data), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshCommonCullingInputBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(mesh_comp_culling_common_input), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

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
		PoissonDiskBuffer = GlobalHeap.PushBuffer(Window.Gfx, PoissonDisk, sizeof(vec2) * 64, false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

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
		RandomAnglesTexture = GlobalHeap.PushTexture(Window.Gfx, (void*)RandomAngles, 32, 32, 32, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
		TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
		NoiseTexture = GlobalHeap.PushTexture(Window.Gfx, (void*)RandomRotations, 4, 4, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_REPEAT);

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
		RandomSamplesBuffer = GlobalHeap.PushBuffer(Window.Gfx, (void*)RandomSamples, sizeof(vec4) * 64, false, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		TextureInputData.Format    = VK_FORMAT_D32_SFLOAT;
		TextureInputData.Usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
		TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Layers    = 1;
		for(texture& Shadow : GlobalShadow)
		{
			Shadow = texture(Window.Gfx, nullptr, PreviousPowerOfTwo(Window.Gfx->Width) * 2, PreviousPowerOfTwo(Window.Gfx->Width) * 2, 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
		}

		TextureInputData.Format    = VK_FORMAT_R32_SFLOAT;
		TextureInputData.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		TextureInputData.MipLevels = GetImageMipLevels(PreviousPowerOfTwo(Window.Gfx->Width), PreviousPowerOfTwo(Window.Gfx->Height));
		DepthPyramid = GlobalHeap.PushTexture(Window.Gfx, PreviousPowerOfTwo(Window.Gfx->Width), PreviousPowerOfTwo(Window.Gfx->Height), 1, TextureInputData, VK_SAMPLER_REDUCTION_MODE_MAX);

		TextureInputData = {};
		TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
		TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Layers    = 1;
		TextureInputData.Format    = Window.Gfx->SurfaceFormat.format;
		TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		TextureInputData.Usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SFLOAT;
		GBuffer[0] = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData); // Vertex Positions
		TextureInputData.Format    = VK_FORMAT_R16G16B16A16_SNORM;
		GBuffer[1] = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData); // Vertex Normals
		GBuffer[2] = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData); // Fragment Normals
		TextureInputData.Format    = VK_FORMAT_R8G8B8A8_UNORM;
		GBuffer[3] = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData); // Diffuse Color
		TextureInputData.Format    = VK_FORMAT_R32_SFLOAT;
		GBuffer[4] = GlobalHeap.PushTexture(Window.Gfx, nullptr, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData); // Specular
		AmbientOcclusionData = GlobalHeap.PushTexture(Window.Gfx, Window.Gfx->Width, Window.Gfx->Height, 1, TextureInputData);

		GfxRootSignature.PushUniformBuffer()->				// World Update Buffer
						 PushStorageBuffer()->				// Vertex Data
						 PushStorageBuffer()->				// Mesh Draw Commands
						 PushStorageBuffer()->				// Mesh Materials
						 // TODO: Update ammount of image samplers in the frame maybe???
						 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Diffuse Texture
						 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Normal Map Texture
						 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Specular Map Texture
						 PushImageSampler(1024, 1, false, VK_SHADER_STAGE_FRAGMENT_BIT)->		// Height Map Texture
						 Build(Window.Gfx, 0, true)->
						 Build(Window.Gfx, 1)->
						 BuildAll(Window.Gfx);

		ColorPassRootSignature.PushUniformBuffer()->					// World Update Buffer
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
							   Build(Window.Gfx, 0, true)->
							   Build(Window.Gfx, 1)->
							   BuildAll(Window.Gfx);

		AmbientOcclusionRootSignature.PushUniformBuffer()->					// World Update Buffer
									  PushStorageBuffer()->					// Poisson Disk
									  PushImageSampler()->					// Random Rotations
									  PushImageSampler(GBUFFER_COUNT)->		// G-Buffer Vertex Position Data
																			// G-Buffer Vertex Normal Data
																			// G-Buffer Fragment Normal Data
																			// G-Buffer Diffuse Data
																			// G-Buffer Specular Data
									  PushStorageImage()->					// Ambient Occlusion Target Texture
									  Build(Window.Gfx, 0, true)->
									  BuildAll(Window.Gfx);

		ShadowSignature.PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
						PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
						PushConstant(sizeof(mat4))->
						Build(Window.Gfx, 0, true)->
						BuildAll(Window.Gfx);

		PointShadowSignature.PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
							 PushStorageBuffer(1, 0, VK_SHADER_STAGE_VERTEX_BIT)->
							 PushConstant(sizeof(point_shadow_input))->
							 Build(Window.Gfx, 0, true)->
							 BuildAll(Window.Gfx);

		CmpIndirectFrustRootSignature.
						 PushUniformBuffer()->				// Mesh Common Culling Input Buffer
						 PushStorageBuffer()->				// Mesh Offsets
						 PushStorageBuffer()->				// Draw Command Input
						 PushStorageBuffer()->				// Draw Command Visibility
						 PushStorageBuffer()->				// Indirect Draw Indexed Command
						 PushStorageBuffer()->				// Indirect Draw Indexed Command Counter
						 PushStorageBuffer()->				// Mesh Draw Command Data
						 Build(Window.Gfx, 0, true)->
						 BuildAll(Window.Gfx);

		ShadowComputeRootSignature.PushUniformBuffer()->	// Mesh Common Culling Input
								  PushStorageBuffer()->		// Mesh Offsets
								  PushStorageBuffer()->		// Draw Command Input
								  PushStorageBuffer()->		// Draw Command Visibility
								  PushStorageBuffer()->		// Indirect Draw Indexed Command
								  PushStorageBuffer()->		// Indirect Draw Indexed Command Counter
								  PushStorageBuffer()->		// Draw Commands
								  Build(Window.Gfx, 0, true)->
								  BuildAll(Window.Gfx);

		CmpIndirectOcclRootSignature.
						 PushUniformBuffer()->				// Mesh Common Culling Input Buffer
						 PushStorageBuffer()->				// Mesh Offsets
						 PushStorageBuffer()->				// Draw Command Input
						 PushStorageBuffer()->				// Draw Command Visibility
						 PushImageSampler()->				// Hierarchy-Z depth texture
						 Build(Window.Gfx, 0, true)->
						 BuildAll(Window.Gfx);

		CmpReduceRootSignature.PushImageSampler(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->		// Input  Texture
							   PushStorageImage(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->		// Output Texture
							   PushConstant(sizeof(vec2), VK_SHADER_STAGE_COMPUTE_BIT)->	// Output Texture Size
							   Build(Window.Gfx, 0, true)->
							   BuildAll(Window.Gfx);

		BlurRootSignature.PushImageSampler(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->			// Input  Texture
						  PushStorageImage(1, 0, false, VK_SHADER_STAGE_COMPUTE_BIT)->			// Output Texture
						  PushConstant(sizeof(vec3), VK_SHADER_STAGE_COMPUTE_BIT)->		// Texture Size, Conv Size 
						  Build(Window.Gfx, 0, true)->
						  BuildAll(Window.Gfx);

		std::vector<VkFormat> GfxFormats;
		for(u32 FormatIdx = 0; FormatIdx < GBuffer.size(); ++FormatIdx) GfxFormats.push_back(GBuffer[FormatIdx].Info.Format);
		GfxContext = render_context(Window.Gfx, GfxRootSignature, {"..\\build\\shaders\\mesh.vert.spv", "..\\build\\shaders\\mesh.frag.spv"}, GfxFormats);

		render_context::input_data RendererInputData = {};
		RendererInputData.UseDepth = true;
		RendererInputData.UseBackFace  = true;
		CascadeShadowContext = render_context(Window.Gfx, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);
		ShadowContext = render_context(Window.Gfx, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);

		RendererInputData.UseMultiview = true;
		for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
		{
			RendererInputData.ViewMask = 1 << CubeMapFaceIdx;
			CubeMapShadowContexts.push_back(render_context(Window.Gfx, PointShadowSignature, {"..\\build\\shaders\\mesh.pnt.sdw.vert.spv", "..\\build\\shaders\\mesh.pnt.sdw.frag.spv"}, {}, RendererInputData));
		}

		RendererInputData.UseDepth	   = true;
		RendererInputData.UseMultiview = false;
		RendererInputData.ViewMask	   = 0;
		DebugCameraViewContext = render_context(Window.Gfx, ShadowSignature, {"..\\build\\shaders\\mesh.sdw.vert.spv", "..\\build\\shaders\\mesh.sdw.frag.spv"}, {}, RendererInputData);

		ColorPassContext = compute_context (Window.Gfx, ColorPassRootSignature, "..\\build\\shaders\\color_pass.comp.spv");
		AmbientOcclusionContext = compute_context (Window.Gfx, AmbientOcclusionRootSignature, "..\\build\\shaders\\screen_space_ambient_occlusion.comp.spv");
		ShadowComputeContext = compute_context(Window.Gfx, ShadowComputeRootSignature, "..\\build\\shaders\\mesh.dbg.comp.spv");
		FrustCullingContext  = compute_context(Window.Gfx, CmpIndirectFrustRootSignature, "..\\build\\shaders\\indirect_cull_frust.comp.spv");
		OcclCullingContext   = compute_context(Window.Gfx, CmpIndirectOcclRootSignature, "..\\build\\shaders\\indirect_cull_occl.comp.spv");
		DepthReduceContext   = compute_context(Window.Gfx, CmpReduceRootSignature, "..\\build\\shaders\\depth_reduce.comp.spv");
		BlurContext = compute_context (Window.Gfx, BlurRootSignature, "..\\build\\shaders\\blur.comp.spv");
	}

	void UpdateResources(window& Window, alloc_vector<light_source>& GlobalLightSources, global_world_data& WorldUpdate)
	{
		u32 PointLightSourceCount = 0;
		u32 SpotLightSourceCount = 0;

		texture::input_data TextureInputData = {};
		if((LightShadows.size() + PointLightShadows.size()) != GlobalLightSources.size())
		{
			LightShadows.clear();
			PointLightShadows.clear();

			TextureInputData.Format = VK_FORMAT_D32_SFLOAT;
			TextureInputData.Usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			TextureInputData.ImageType = VK_IMAGE_TYPE_2D;
			for(const light_source& LightSource : GlobalLightSources)
			{
				if(LightSource.Type == light_type_point)
				{
					TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_CUBE;
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 6;
					if(WorldUpdate.LightSourceShadowsEnabled)
						PointLightShadows.push_back(texture(Window.Gfx, nullptr, Window.Width, Window.Height, 1, TextureInputData));
					PointLightSourceCount++;
				}
				else if(LightSource.Type == light_type_spot)
				{
					TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 1;
					if(WorldUpdate.LightSourceShadowsEnabled)
						LightShadows.push_back(texture(Window.Gfx, nullptr, Window.Width, Window.Height, 1, TextureInputData));
					SpotLightSourceCount++;
				}
			}

			ColorPassContext.SetImageSampler(LightShadows, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
			ColorPassContext.SetImageSampler(PointLightShadows, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
			ColorPassContext.StaticUpdate();
		}

		// TODO: Update only when needed (The ammount of objects was changed)
		GfxContext.SetImageSampler(DiffuseTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		GfxContext.SetImageSampler(NormalTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		GfxContext.SetImageSampler(SpecularTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		GfxContext.SetImageSampler(HeightTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		GfxContext.StaticUpdate();
	}

	void Render(window& Window, global_pipeline_context& PipelineContext, 
				texture& GfxColorTarget, texture& GfxDepthTarget, texture& DebugCameraViewDepthTarget, 
				global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput,
				alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility, alloc_vector<light_source>& GlobalLightSources)
	{
#if 0
		for(entity& Entity : Entities)
		{
			dynamic_instances_component* InstancesComponent = Entity.GetComponent<dynamic_instances_component>();

			DynamicMeshInstances.insert(DynamicMeshInstances.end(), InstancesComponent->Data.begin(), InstancesComponent->Data.end());
			DynamicMeshVisibility.insert(DynamicMeshVisibility.end(), InstancesComponent->Visibility.begin(), InstancesComponent->Visibility.end());
		}
#endif
		{
			std::vector<std::tuple<buffer&, VkAccessFlags, VkAccessFlags>> SetupBufferBarrier = 
			{
				{WorldUpdateBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
				{MeshCommonCullingInputBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
				{LightSourcesBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
			};
			PipelineContext.SetBufferBarriers(SetupBufferBarrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			WorldUpdateBuffer.UpdateSize(Window.Gfx, &WorldUpdate, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer.UpdateSize(Window.Gfx, &MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);

			LightSourcesBuffer.UpdateSize(Window.Gfx, GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), PipelineContext);
		}

		{
			FrustCullingContext.Begin(PipelineContext);

			PipelineContext.SetBufferBarrier({MeshCommonCullingInputBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, 
											  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			FrustCullingContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			FrustCullingContext.SetStorageBufferView(GeometryOffsets);
			FrustCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
			FrustCullingContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
			FrustCullingContext.SetStorageBufferView(IndirectDrawIndexedCommands);
			FrustCullingContext.SetStorageBufferView(MeshDrawCommandBuffer);

			FrustCullingContext.Execute(StaticMeshInstances.size());

			FrustCullingContext.End();
		}

		{
			ShadowComputeContext.Begin(PipelineContext);

			ShadowComputeContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			ShadowComputeContext.SetStorageBufferView(GeometryOffsets);
			ShadowComputeContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
			ShadowComputeContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
			ShadowComputeContext.SetStorageBufferView(ShadowIndirectDrawIndexedCommands);
			ShadowComputeContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);

			ShadowComputeContext.Execute(StaticMeshInstances.size());

			ShadowComputeContext.End();
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

			PipelineContext.SetBufferBarriers({{MeshDrawShadowCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, 
											  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{ShadowIndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, 
											  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

			CascadeShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, GlobalShadow[CascadeIdx].Width, GlobalShadow[CascadeIdx].Height, GlobalShadow[CascadeIdx], {1, 0});
			CascadeShadowContext.Begin(Window.Gfx, PipelineContext, GlobalShadow[CascadeIdx].Width, GlobalShadow[CascadeIdx].Height);

			CascadeShadowContext.SetStorageBufferView(VertexBuffer);
			CascadeShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
			CascadeShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
			CascadeShadowContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

			CascadeShadowContext.End();
		}

		// TODO: something better or/and efficient here
		// TODO: render only when the actual light source have been changed
		u32 PointLightSourceIdx  = 0;
		u32 SpotLightSourceIdx   = 0;
		if(WorldUpdate.LightSourceShadowsEnabled)
		{
			for(const light_source& LightSource : GlobalLightSources)
			{
				if(LightSource.Type == light_type_point)
				{
					texture& ShadowMapTexture = PointLightShadows[PointLightSourceIdx];
					std::vector<VkImageMemoryBarrier> ShadowBarrier = 
					{
						CreateImageBarrier(ShadowMapTexture.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
					};
					ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

					for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
					{
						vec3 CubeMapFaceDir = CubeMapDirections[CubeMapFaceIdx];
						vec3 CubeMapUpVect  = CubeMapUpVectors[CubeMapFaceIdx];
						mat4 ShadowMapProj  = PerspRH(90.0f, ShadowMapTexture.Width, ShadowMapTexture.Height, WorldUpdate.NearZ, WorldUpdate.FarZ);
						mat4 ShadowMapView  = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + CubeMapFaceDir, CubeMapUpVect);
						mat4 Shadow = ShadowMapView * ShadowMapProj;
						point_shadow_input PointShadowInput = {};
						PointShadowInput.LightPos = GlobalLightSources[PointLightSourceIdx + SpotLightSourceIdx].Pos;
						PointShadowInput.LightMat = Shadow;
						PointShadowInput.FarZ = WorldUpdate.FarZ;

						CubeMapShadowContexts[CubeMapFaceIdx].SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0}, CubeMapFaceIdx, true);
						CubeMapShadowContexts[CubeMapFaceIdx].Begin(Window.Gfx, PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

						CubeMapShadowContexts[CubeMapFaceIdx].SetStorageBufferView(VertexBuffer);
						CubeMapShadowContexts[CubeMapFaceIdx].SetStorageBufferView(MeshDrawShadowCommandBuffer);
						CubeMapShadowContexts[CubeMapFaceIdx].SetConstant((void*)&PointShadowInput, sizeof(point_shadow_input));
						CubeMapShadowContexts[CubeMapFaceIdx].DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

						CubeMapShadowContexts[CubeMapFaceIdx].End();
					}
					PointLightSourceIdx++;
				}
				else if(LightSource.Type == light_type_spot)
				{
					texture& ShadowMapTexture = LightShadows[SpotLightSourceIdx];

					std::vector<VkImageMemoryBarrier> ShadowBarrier = 
					{
						CreateImageBarrier(ShadowMapTexture.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
					};
					ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

					mat4 ShadowMapProj = PerspRH(LightSource.Pos.w, ShadowMapTexture.Width, ShadowMapTexture.Height, WorldUpdate.NearZ, WorldUpdate.FarZ);
					mat4 ShadowMapView = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + vec3(LightSource.Dir), vec3(0, 1, 0));
					mat4 Shadow = ShadowMapView * ShadowMapProj;

					ShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0});
					ShadowContext.Begin(Window.Gfx, PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

					ShadowContext.SetStorageBufferView(VertexBuffer);
					ShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
					ShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
					ShadowContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

					ShadowContext.End();
					SpotLightSourceIdx++;
				}
			}
		}

		// NOTE: This is only for debug. Maybe not compile on release mode???
		{
			mat4 Shadow = WorldUpdate.DebugView * WorldUpdate.Proj;

			std::vector<VkImageMemoryBarrier> ShadowBarrier = 
			{
				CreateImageBarrier(DebugCameraViewDepthTarget.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

			PipelineContext.SetBufferBarriers({{MeshDrawCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{IndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

			DebugCameraViewContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, DebugCameraViewDepthTarget.Width, DebugCameraViewDepthTarget.Height, DebugCameraViewDepthTarget, {1, 0});
			DebugCameraViewContext.Begin(Window.Gfx, PipelineContext, DebugCameraViewDepthTarget.Width, DebugCameraViewDepthTarget.Height);

			DebugCameraViewContext.SetStorageBufferView(VertexBuffer);
			DebugCameraViewContext.SetStorageBufferView(MeshDrawCommandBuffer);
			DebugCameraViewContext.SetConstant((void*)&Shadow, sizeof(mat4));
			DebugCameraViewContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

			DebugCameraViewContext.End();
		}

		{
			std::vector<VkImageMemoryBarrier> ImageBeginRenderBarriers;
			for(u32 Idx = 0;
				Idx < DiffuseTextures.size();
				++Idx)
			{
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(DiffuseTextures[Idx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
			for(u32 Idx = 0;
				Idx < NormalTextures.size();
				++Idx)
			{
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(NormalTextures[Idx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
			for(u32 Idx = 0;
				Idx < SpecularTextures.size();
				++Idx)
			{
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(SpecularTextures[Idx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
			for(u32 Idx = 0;
				Idx < HeightTextures.size();
				++Idx)
			{
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(HeightTextures[Idx].Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
			for(texture& ColorTarget : GBuffer)
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(ColorTarget.Handle, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
			ImageBeginRenderBarriers.push_back(CreateImageBarrier(GfxDepthTarget.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ImageBeginRenderBarriers);

			PipelineContext.SetBufferBarrier({WorldUpdateBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			PipelineContext.SetBufferBarrier({MeshMaterialsBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			GfxContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx->Width, Window.Gfx->Height, GBuffer, {0, 0, 0, 0});
			GfxContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx->Width, Window.Gfx->Height, GfxDepthTarget, {1, 0});
			GfxContext.Begin(Window.Gfx, PipelineContext, Window.Gfx->Width, Window.Gfx->Height);

			GfxContext.SetUniformBufferView(WorldUpdateBuffer);
			GfxContext.SetStorageBufferView(VertexBuffer);
			GfxContext.SetStorageBufferView(MeshDrawCommandBuffer);
			GfxContext.SetStorageBufferView(MeshMaterialsBuffer);
			GfxContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

			GfxContext.End();
		}

		{
			AmbientOcclusionContext.Begin(PipelineContext);

			std::vector<VkImageMemoryBarrier> AmbientOcclusionPassBarrier = 
			{
				CreateImageBarrier(NoiseTexture.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
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
			AmbientOcclusionContext.Execute(Window.Gfx->Width, Window.Gfx->Height);

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

			vec3 BlurInput(Window.Gfx->Width, Window.Gfx->Height, 1.0);
			BlurContext.SetImageSampler({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			BlurContext.SetStorageImage({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			BlurContext.SetConstant((void*)BlurInput.E, sizeof(vec3));
			BlurContext.Execute(Window.Gfx->Width, Window.Gfx->Height);

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

			vec3 BlurInput(Window.Gfx->Width, Window.Gfx->Height, 0.0);
			BlurContext.SetImageSampler({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			BlurContext.SetStorageImage({AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			BlurContext.SetConstant((void*)BlurInput.E, sizeof(vec3));
			BlurContext.Execute(Window.Gfx->Width, Window.Gfx->Height);

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

			PipelineContext.SetBufferBarrier({LightSourcesBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			ColorPassContext.SetUniformBufferView(WorldUpdateBuffer);
			ColorPassContext.SetUniformBufferView(LightSourcesBuffer);
			ColorPassContext.SetStorageBufferView(PoissonDiskBuffer);
			ColorPassContext.SetImageSampler({RandomAnglesTexture}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			//ColorPassContext.SetImageSampler({GfxDepthTarget}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			ColorPassContext.SetImageSampler(GBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			ColorPassContext.SetImageSampler({AmbientOcclusionData}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			ColorPassContext.SetStorageImage({GfxColorTarget}, VK_IMAGE_LAYOUT_GENERAL);
			ColorPassContext.SetImageSampler(GlobalShadow, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			ColorPassContext.Execute(Window.Gfx->Width, Window.Gfx->Height);

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
			OcclCullingContext.Begin(PipelineContext);

			OcclCullingContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			OcclCullingContext.SetStorageBufferView(GeometryOffsets);
			OcclCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
			OcclCullingContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
			OcclCullingContext.SetImageSampler({DepthPyramid}, VK_IMAGE_LAYOUT_GENERAL);

			OcclCullingContext.Execute(StaticMeshInstances.size());

			OcclCullingContext.End();
		}
	}
};
