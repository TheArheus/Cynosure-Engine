#pragma once

// TODO: Resource optimization
struct deferred_raster_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh::material> Materials;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	vec3 CubeMapDirections[6];
	vec3 CubeMapUpVectors[6];

	std::vector<resource_descriptor> DiffuseTextures;
	std::vector<resource_descriptor> NormalTextures;
	std::vector<resource_descriptor> SpecularTextures;
	std::vector<resource_descriptor> HeightTextures;

	std::vector<resource_descriptor> LightShadows;
	std::vector<resource_descriptor> PointLightShadows;

	resource_descriptor WorldUpdateBuffer;
	resource_descriptor MeshCommonCullingInputBuffer;
	resource_descriptor MeshDrawCommandDataBuffer;

	resource_descriptor GeometryOffsets;
	resource_descriptor VertexBuffer;
	resource_descriptor IndexBuffer;

	resource_descriptor MeshMaterialsBuffer;
	resource_descriptor LightSourcesBuffer;
	resource_descriptor LightSourcesMatrixBuffer;

	resource_descriptor MeshDrawVisibilityDataBuffer;
	resource_descriptor IndirectDrawIndexedCommands;
	resource_descriptor MeshDrawCommandBuffer;

	resource_descriptor MeshDrawShadowVisibilityDataBuffer;
	resource_descriptor ShadowIndirectDrawIndexedCommands;
	resource_descriptor MeshDrawShadowCommandBuffer;

	resource_descriptor PoissonDiskBuffer;
	resource_descriptor RandomSamplesBuffer;

	// ---------------------------------------------

	resource_descriptor DebugCameraViewDepthTarget;

	resource_descriptor VoxelGridTarget;
	resource_descriptor VoxelGridNormal;
	resource_descriptor HdrColorTarget;
	resource_descriptor BrightTarget;
	resource_descriptor TempBrTarget;

	resource_descriptor GlobalShadowDepth;
	std::vector<resource_descriptor> GlobalShadowEVSM;
	std::vector<resource_descriptor> GBuffer;

	resource_descriptor AmbientOcclusionData;
	resource_descriptor TempTexture;
	resource_descriptor DepthPyramid;
	resource_descriptor RandomAnglesTexture;
	resource_descriptor NoiseTexture;

	resource_descriptor VolumetricLightOut;
	resource_descriptor IndirectLightOut;
	resource_descriptor LightColor;

	deferred_raster_system()
	{
		RequireComponent<mesh_component>();
		RequireComponent<static_instances_component>();

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

	// TODO: Move to/create a asset storage
	void Setup(u32 RenderWidth, u32 RenderHeight, window& Window, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput)
	{
		texture_data Texture = {};
		mesh::material NewMaterial = {};
		utils::texture::input_data TextureInputData = {};
		TextureInputData.Format    = image_format::R8G8B8A8_SRGB;
		TextureInputData.Usage     = TF_Sampled | TF_ColorTexture;
		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.MipLevels = 1;

		vec3 SceneMin = vec3( FLT_MAX);
		vec3 SceneMax = vec3(-FLT_MAX);

		u32 MeshIdx      = 0;
		u32 TextureIdx   = 0;
		u32 NormalMapIdx = 0;
		u32 SpecularIdx  = 0;
		u32 HeightMapIdx = 0;
		u32 InstanceOffset = 0;
		for(entity& Entity : Entities)
		{
			NewMaterial = {};
			u32 EntityIdx = *(u32*)&Entity.Handle;

			NewMaterial.LightDiffuse = vec4(vec3(1.0), 1.0);
			NewMaterial.LightEmmit   = vec4(vec3(0.0), 1.0);
			if(Entity.HasComponent<color_component>())
			{
				color_component& Color   = Entity.GetComponent<color_component>();
				NewMaterial.LightDiffuse = vec4(Color.Data, 1.0);
			}
			if(Entity.HasComponent<emmit_component>())
			{
				emmit_component& Emmit = Entity.GetComponent<emmit_component>();
				NewMaterial.LightEmmit = vec4(Emmit.Data, Emmit.Intensity);
			}
			if(Entity.HasComponent<diffuse_component>())
			{
				diffuse_component& Diffuse = Entity.GetComponent<diffuse_component>();
				Texture.Load(Diffuse.Data);
				DiffuseTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("DiffuseTexture" + std::to_string(TextureIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasTexture = true;
				NewMaterial.TextureIdx = TextureIdx++;
				Texture.Delete();
			}
			if(Entity.HasComponent<normal_map_component>())
			{
				normal_map_component& NormalMap = Entity.GetComponent<normal_map_component>();
				Texture.Load(NormalMap.Data);
				NormalTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("NormalTexture" + std::to_string(NormalMapIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasNormalMap = true;
				NewMaterial.NormalMapIdx = NormalMapIdx++;
				Texture.Delete();
			}
			if(Entity.HasComponent<specular_map_component>())
			{
				specular_map_component& SpecularMap = Entity.GetComponent<specular_map_component>();
				Texture.Load(SpecularMap.Data);
				SpecularTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("SpecularTexture" + std::to_string(SpecularIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasSpecularMap = true;
				NewMaterial.SpecularMapIdx = SpecularIdx++;
				Texture.Delete();
			}
			if(Entity.HasComponent<height_map_component>())
			{
				height_map_component& HeightMap = Entity.GetComponent<height_map_component>();
				Texture.Load(HeightMap.Data);
				HeightTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("HeightTexture" + std::to_string(HeightMapIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasHeightMap = true;
				NewMaterial.HeightMapIdx = HeightMapIdx++;
				Texture.Delete();
			}

			vec3 MeshCenter;
			vec3 MeshMin;
			vec3 MeshMax;
			if(Entity.HasComponent<mesh_component>())
			{
				mesh_component& Mesh = Entity.GetComponent<mesh_component>();

				MeshCenter = Mesh.Data.Offsets[0].BoundingSphere.Center.xyz;
				MeshMin = Mesh.Data.Offsets[0].AABB.Min.xyz;
				MeshMax = Mesh.Data.Offsets[0].AABB.Max.xyz;

				Geometries.Load(Mesh.Data);
			}

			if(Entity.HasComponent<static_instances_component>())
			{
				static_instances_component& InstancesComponent = Entity.GetComponent<static_instances_component>();
				Geometries.Offsets[EntityIdx].InstanceOffset = InstanceOffset;
				InstanceOffset += InstancesComponent.Data.size();
				for (mesh_draw_command& EntityInstance : InstancesComponent.Data)
				{
					StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), EntityIdx });
					StaticMeshVisibility.push_back(true);
				}
			}

			Materials.push_back(NewMaterial);
		}

		MeshCommonCullingInput.DrawCount = StaticMeshInstances.size();
		MeshCommonCullingInput.MeshCount = Geometries.MeshCount;

		// TODO: Move resource management into renderer_backend class
		MeshMaterialsBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), RF_StorageBuffer);
		LightSourcesBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("LightSourcesBuffer", sizeof(light_source), LIGHT_SOURCES_MAX_COUNT, RF_StorageBuffer);
		LightSourcesMatrixBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("LightSourcesMatrixBuffer", sizeof(mat4), LIGHT_SOURCES_MAX_COUNT, RF_StorageBuffer);

		VertexBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("VertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), RF_StorageBuffer);
		IndexBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("IndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), RF_IndexBuffer);

		GeometryOffsets = Window.Gfx.GpuMemoryHeap->CreateBuffer("GeometryOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), RF_StorageBuffer);
		MeshDrawCommandDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_IndirectBuffer);
		MeshDrawVisibilityDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer | RF_StorageBuffer);
		IndirectDrawIndexedCommands = Window.Gfx.GpuMemoryHeap->CreateBuffer("IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), Geometries.MeshCount, RF_IndirectBuffer | RF_StorageBuffer | RF_WithCounter);

		MeshDrawCommandBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_StorageBuffer);

		MeshDrawShadowVisibilityDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawShadowVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer);
		ShadowIndirectDrawIndexedCommands = Window.Gfx.GpuMemoryHeap->CreateBuffer("ShadowIndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), Geometries.MeshCount, RF_IndirectBuffer | RF_StorageBuffer | RF_WithCounter);
		MeshDrawShadowCommandBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawShadowCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_StorageBuffer);

		WorldUpdateBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, RF_StorageBuffer);
		MeshCommonCullingInputBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, RF_StorageBuffer);

		// -------------------------------

		TextureInputData.Format    = image_format::D32_SFLOAT;
		TextureInputData.Usage     = TF_DepthTexture | TF_Sampled;
		DebugCameraViewDepthTarget = Window.Gfx.GpuMemoryHeap->CreateTexture("DebugCameraViewDepthTarget", PreviousPowerOfTwo(RenderWidth) * 2, PreviousPowerOfTwo(RenderHeight) * 2, 1, TextureInputData);

		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Format    = image_format::B8G8R8A8_UNORM;
		TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled;
		VolumetricLightOut = Window.Gfx.GpuMemoryHeap->CreateTexture("Volumetric light Result", RenderWidth, RenderHeight, 1, TextureInputData);
		IndirectLightOut   = Window.Gfx.GpuMemoryHeap->CreateTexture("Indirect light result"  , RenderWidth, RenderHeight, 1, TextureInputData);
		LightColor         = Window.Gfx.GpuMemoryHeap->CreateTexture("Light color",  RenderWidth, RenderHeight, 1, TextureInputData);

		TextureInputData.Type	   = image_type::Texture3D;
		TextureInputData.Format    = image_format::R32G32B32A32_SFLOAT;
		TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled;
		TextureInputData.MipLevels = GetImageMipLevels(VOXEL_SIZE, VOXEL_SIZE);
		TextureInputData.SamplerInfo.AddressMode = sampler_address_mode::clamp_to_border;
		VoxelGridTarget = Window.Gfx.GpuMemoryHeap->CreateTexture("VoxelGridTarget", VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE, TextureInputData);
		VoxelGridNormal = Window.Gfx.GpuMemoryHeap->CreateTexture("VoxelGridNormal", VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE, TextureInputData);

		TextureInputData.MipLevels = 1;
		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.SamplerInfo = {};
		GlobalShadowEVSM.resize(DEPTH_CASCADES_COUNT);
		for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; CascadeIdx++)
		{
			GlobalShadowEVSM[CascadeIdx] = Window.Gfx.GpuMemoryHeap->CreateTexture("GlobalShadowEVSM" + std::to_string(CascadeIdx), PreviousPowerOfTwo(RenderWidth) * 2, PreviousPowerOfTwo(RenderWidth) * 2, 1, TextureInputData);
		}

		TextureInputData.Format    = image_format::D32_SFLOAT;
		TextureInputData.Usage     = TF_DepthTexture | TF_Sampled;
		TextureInputData.MipLevels = 1;
		GlobalShadowDepth = Window.Gfx.GpuMemoryHeap->CreateTexture("GlobalShadowDepth", PreviousPowerOfTwo(RenderWidth) * 2, PreviousPowerOfTwo(RenderWidth) * 2, 1, TextureInputData);

		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.Format    = image_format::R11G11B10_SFLOAT;
		TextureInputData.Usage     = TF_ColorAttachment | TF_Storage | TF_Sampled;
		TextureInputData.MipLevels = 1;
		HdrColorTarget = Window.Gfx.GpuMemoryHeap->CreateTexture("HdrColorTarget", RenderWidth, RenderHeight, 1, TextureInputData);
		TextureInputData.MipLevels = 6;
		TextureInputData.SamplerInfo.AddressMode = sampler_address_mode::clamp_to_border;
		BrightTarget   = Window.Gfx.GpuMemoryHeap->CreateTexture("BrightTarget", RenderWidth, RenderHeight, 1, TextureInputData);
		TempBrTarget   = Window.Gfx.GpuMemoryHeap->CreateTexture("TempBrTarget", RenderWidth, RenderHeight, 1, TextureInputData);

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
		PoissonDiskBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("PoissonDiskBuffer", PoissonDisk, sizeof(vec2), 64, RF_StorageBuffer);

		// TODO: Fix those buffers to textures
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
		TextureInputData.Usage     = TF_Storage | TF_Sampled | TF_ColorTexture;
		TextureInputData.Type	   = image_type::Texture3D;
		TextureInputData.MipLevels = 1;
		RandomAnglesTexture = Window.Gfx.GpuMemoryHeap->CreateTexture("RandomAnglesTexture", RandomAngles, Res, Res, Res, TextureInputData);
		TextureInputData.Format    = image_format::R32G32_SFLOAT;
		TextureInputData.Type	   = image_type::Texture2D;
		NoiseTexture        = Window.Gfx.GpuMemoryHeap->CreateTexture("NoiseTexture", RandomRotations, 4, 4, 1, TextureInputData);

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
		RandomSamplesBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("RandomSamplesBuffer", RandomSamples, sizeof(vec4), 64, RF_StorageBuffer);

		TextureInputData.Format    = image_format::R32_SFLOAT;
		TextureInputData.Usage     = TF_Sampled | TF_Storage | TF_ColorTexture;
		TextureInputData.MipLevels = GetImageMipLevels(PreviousPowerOfTwo(RenderWidth), PreviousPowerOfTwo(RenderHeight));
		TextureInputData.SamplerInfo.ReductionMode = sampler_reduction_mode::max;
		TextureInputData.SamplerInfo.MinFilter  = filter::nearest;
		TextureInputData.SamplerInfo.MagFilter  = filter::nearest;
		TextureInputData.SamplerInfo.MipmapMode = mipmap_mode::nearest;
		DepthPyramid = Window.Gfx.GpuMemoryHeap->CreateTexture("DepthPyramid", DebugCameraViewDepthTarget.Width / 2, DebugCameraViewDepthTarget.Height / 2, 1, TextureInputData);

		GBuffer.resize(GBUFFER_COUNT);
		TextureInputData = {};
		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Usage     = TF_ColorAttachment | TF_Sampled | TF_Storage;
		TextureInputData.Format    = image_format::R16G16B16A16_SNORM;
		GBuffer[0] = Window.Gfx.GpuMemoryHeap->CreateTexture("GBuffer_VertexNormals"  , RenderWidth, RenderHeight, 1, TextureInputData); // Vertex   Normals
		GBuffer[1] = Window.Gfx.GpuMemoryHeap->CreateTexture("GBuffer_FragmentNormals", RenderWidth, RenderHeight, 1, TextureInputData); // Fragment Normals
		TextureInputData.Format    = image_format::R8G8B8A8_UNORM;
		GBuffer[2] = Window.Gfx.GpuMemoryHeap->CreateTexture("GBuffer_Diffuse", RenderWidth, RenderHeight, 1, TextureInputData); // Diffuse Color
		GBuffer[3] = Window.Gfx.GpuMemoryHeap->CreateTexture("GBuffer_Emmit"  , RenderWidth, RenderHeight, 1, TextureInputData); // Emmit Color
		TextureInputData.Format    = image_format::R32G32_SFLOAT;
		GBuffer[4] = Window.Gfx.GpuMemoryHeap->CreateTexture("GBuffer_Specular", RenderWidth, RenderHeight, 1, TextureInputData); // Specular + Light Emmission Ammount

		TextureInputData.Format    = image_format::R32_SFLOAT;
		AmbientOcclusionData = Window.Gfx.GpuMemoryHeap->CreateTexture("AmbientOcclusionData", RenderWidth, RenderHeight, 1, TextureInputData);
		TextureInputData.Format    = image_format::R32G32B32A32_SFLOAT;
		TempTexture          = Window.Gfx.GpuMemoryHeap->CreateTexture("TempTexture", PreviousPowerOfTwo(RenderWidth) * 2, PreviousPowerOfTwo(RenderWidth) * 2, 1, TextureInputData);
	}

	// TODO: Move out all render commands. There will be only a geometry render command generation I guess
	void Render(u32 RenderWidth, u32 RenderHeight, global_graphics_context& Gfx, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput, alloc_vector<light_source>& GlobalLightSources, float GScat)
	{
		{
			u32 PointLightSourceCount = 0;
			u32 SpotLightSourceCount = 0;

			utils::texture::input_data TextureInputData = {};
			// TODO: Implement this to be more dynamic
			// TODO: Better Implementation for the lights
			if((LightShadows.size() + PointLightShadows.size()) != GlobalLightSources.size())
			{
				LightShadows.clear();
				PointLightShadows.clear();
				std::vector<mat4> PointLightMatrices;

				TextureInputData.Format = image_format::D32_SFLOAT;
				TextureInputData.Usage  = TF_DepthTexture | TF_Sampled;
				TextureInputData.Type   = image_type::Texture2D;
				for(const light_source& LightSource : GlobalLightSources)
				{
					if(LightSource.Type == light_type_point)
					{
						for(int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
						{
							vec3 CubeMapFaceDir = CubeMapDirections[FaceIdx];
							vec3 CubeMapUpVect  = CubeMapUpVectors[FaceIdx];
							mat4 ShadowMapProj  = PerspRH(90.0f, RenderWidth, RenderHeight, WorldUpdate.NearZ, WorldUpdate.FarZ);
							mat4 ShadowMapView  = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + CubeMapFaceDir, CubeMapUpVect);
							mat4 Shadow = ShadowMapView * ShadowMapProj;
							PointLightMatrices.push_back(Shadow);
						}

						TextureInputData.Usage |= TF_CubeMap;
						TextureInputData.MipLevels = 1;
						if(WorldUpdate.LightSourceShadowsEnabled)
							PointLightShadows.push_back(Gfx.GpuMemoryHeap->CreateTexture("PointLightShadow" + std::to_string(PointLightSourceCount), RenderWidth, RenderWidth, 6, TextureInputData));
						PointLightSourceCount++;
					}
					else if(LightSource.Type == light_type_spot)
					{
						TextureInputData.MipLevels = 1;
						if(WorldUpdate.LightSourceShadowsEnabled)
							LightShadows.push_back(Gfx.GpuMemoryHeap->CreateTexture("LightShadow" + std::to_string(SpotLightSourceCount), RenderWidth, RenderHeight, 1, TextureInputData));
						SpotLightSourceCount++;
					}
				}
				Gfx.GpuMemoryHeap->UpdateBuffer(LightSourcesMatrixBuffer, PointLightMatrices.data(), sizeof(mat4), PointLightMatrices.size());
			}
		}

		Gfx.GpuMemoryHeap->UpdateBuffer(WorldUpdateBuffer, (void*)&WorldUpdate, sizeof(global_world_data));
		Gfx.GpuMemoryHeap->UpdateBuffer(MeshCommonCullingInputBuffer, (void*)&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input));
		Gfx.GpuMemoryHeap->UpdateBuffer(LightSourcesBuffer, GlobalLightSources.data(), sizeof(light_source), GlobalLightSources.size());

		{
			frustum_culling::parameters Parameters;
			Parameters.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;
			Parameters.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;
			Parameters.IndirectDrawIndexedCommands = IndirectDrawIndexedCommands;
			Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddComputePass<frustum_culling>("Frustum culling/indirect command generation", Parameters,
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size(), IndirectCommands = Gfx.GpuMemoryHeap->GetBuffer(IndirectDrawIndexedCommands)](command_list* Cmd)
			{
				Cmd->FillBuffer(IndirectCommands, 0);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstancesCount);
			});
		}

		{
			generate_all::parameters Parameters;
			Parameters.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;
			Parameters.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.MeshDrawVisibilityDataBuffer = MeshDrawShadowVisibilityDataBuffer;
			Parameters.IndirectDrawIndexedCommands = ShadowIndirectDrawIndexedCommands;
			Parameters.MeshDrawCommandBuffer = MeshDrawShadowCommandBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddComputePass<generate_all>("Frustum culling/indirect command generation for shadow maps", Parameters,
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size(), IndirectCommands = Gfx.GpuMemoryHeap->GetBuffer(ShadowIndirectDrawIndexedCommands)](command_list* Cmd)
			{
				Cmd->FillBuffer(IndirectCommands, 0);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstancesCount);
			});
		}

		{
			mesh_depth_variance_exp::parameters Parameters;
			Parameters.VertexBuffer = VertexBuffer;
			Parameters.CommandBuffer = MeshDrawShadowCommandBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;

			mesh_depth_variance_exp::raster_parameters RasterParameters;
			RasterParameters.IndexBuffer = IndexBuffer;
			RasterParameters.IndirectBuffer = ShadowIndirectDrawIndexedCommands;

			for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
			{
				mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];

				RasterParameters.ColorTarget = GlobalShadowEVSM[CascadeIdx];
				RasterParameters.DepthTarget = GlobalShadowDepth;

				Gfx.AddRasterPass<mesh_depth_variance_exp>("Cascade shadow map generation: #" + std::to_string(CascadeIdx), GlobalShadowEVSM[CascadeIdx].Width, GlobalShadowEVSM[CascadeIdx].Height, Parameters, RasterParameters,
				[MeshCount = Geometries.MeshCount, Width = GlobalShadowEVSM[CascadeIdx].Width, Height = GlobalShadowEVSM[CascadeIdx].Height, Shadow](command_list* Cmd)
				{
					Cmd->SetViewport(0, 0, Width, Height);
					Cmd->SetConstant((void*)&Shadow, sizeof(mat4));
					Cmd->DrawIndirect(MeshCount, sizeof(indirect_draw_indexed_command));
				});
			}
		}

		{
			u32 PointLightSourceIdx  = 0;
			u32 SpotLightSourceIdx   = 0;
			if(WorldUpdate.LightSourceShadowsEnabled)
			{
				for(const light_source& LightSource : GlobalLightSources)
				{
					if(LightSource.Type == light_type_point)
					{
						resource_descriptor& ShadowMapTexture = PointLightShadows[PointLightSourceIdx];

						point_shadow_input PointShadowInput = {};
						PointShadowInput.LightPos = GlobalLightSources[PointLightSourceIdx + SpotLightSourceIdx].Pos;
						PointShadowInput.FarZ = WorldUpdate.FarZ;
						PointShadowInput.LightIdx = PointLightSourceIdx;

						mesh_depth_cubemap::parameters Parameters;
						Parameters.VertexBuffer = VertexBuffer;
						Parameters.CommandBuffer = MeshDrawShadowCommandBuffer;
						Parameters.GeometryOffsets = GeometryOffsets;
						Parameters.LightSourcesMatrixBuffer = LightSourcesMatrixBuffer;

						mesh_depth_cubemap::raster_parameters RasterParameters;
						RasterParameters.IndexBuffer = IndexBuffer;
						RasterParameters.IndirectBuffer = ShadowIndirectDrawIndexedCommands;

						RasterParameters.DepthTarget = ShadowMapTexture;

						Gfx.AddRasterPass<mesh_depth_cubemap>("Shadow Map Point Light #" + std::to_string(SpotLightSourceIdx) + " Generation", ShadowMapTexture.Width, ShadowMapTexture.Height, Parameters, RasterParameters,
						[MeshCount = Geometries.MeshCount, Width = ShadowMapTexture.Width, Height = ShadowMapTexture.Height, PointShadowInput](command_list* Cmd)
						{
							Cmd->SetViewport(0, 0, Width, Height);
							Cmd->SetConstant((void*)&PointShadowInput, sizeof(point_shadow_input));
							Cmd->DrawIndirect(MeshCount, sizeof(indirect_draw_indexed_command));
						});

						PointLightSourceIdx++;
					}
					else if(LightSource.Type == light_type_spot)
					{
						resource_descriptor& ShadowMapTexture = LightShadows[SpotLightSourceIdx];

						mat4 ShadowMapProj = PerspRH(LightSource.Pos.w, ShadowMapTexture.Width, ShadowMapTexture.Height, WorldUpdate.NearZ, WorldUpdate.FarZ);
						mat4 ShadowMapView = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + vec3(LightSource.Dir), vec3(0, 1, 0));
						mat4 Shadow = ShadowMapProj * ShadowMapView;

						mesh_depth::parameters Parameters;
						Parameters.VertexBuffer = VertexBuffer;
						Parameters.CommandBuffer = MeshDrawShadowCommandBuffer;
						Parameters.GeometryOffsets = GeometryOffsets;

						mesh_depth::raster_parameters RasterParameters;
						RasterParameters.IndexBuffer = IndexBuffer;
						RasterParameters.IndirectBuffer = ShadowIndirectDrawIndexedCommands;
						RasterParameters.DepthTarget = ShadowMapTexture;

						Gfx.AddRasterPass<mesh_depth>("Shadow Map Spot Light #" + std::to_string(SpotLightSourceIdx) + " Generation", ShadowMapTexture.Width, ShadowMapTexture.Height, Parameters, RasterParameters,
						[MeshCount = Geometries.MeshCount, Width = ShadowMapTexture.Width, Height = ShadowMapTexture.Height, Shadow](command_list* Cmd)
						{
							Cmd->SetViewport(0, 0, Width, Height);
							Cmd->SetConstant((void*)&Shadow, sizeof(mat4));
							Cmd->DrawIndirect(MeshCount, sizeof(indirect_draw_indexed_command));
						});

						SpotLightSourceIdx++;
					}
				}
			}
		}

		for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
		{
			blur::parameters Parameters;
			Parameters.Input  = GlobalShadowEVSM[CascadeIdx];
			Parameters.Output = TempTexture;

			vec3 BlurInput(GlobalShadowEVSM[CascadeIdx].Width, GlobalShadowEVSM[CascadeIdx].Height, 1.0);
			Gfx.AddComputePass<blur>("Blur Vertical", Parameters,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});

			Parameters.Input  = TempTexture;
			Parameters.Output = GlobalShadowEVSM[CascadeIdx];

			BlurInput.z = 0.0;
			Gfx.AddComputePass<blur>("Blur Horizontal", Parameters,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});
		}

		{
			mesh_depth::parameters Parameters = {};
			Parameters.VertexBuffer = VertexBuffer;
			Parameters.CommandBuffer = MeshDrawCommandBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;

			mesh_depth::raster_parameters RasterParameters = {};
			RasterParameters.IndexBuffer = IndexBuffer;
			RasterParameters.IndirectBuffer = IndirectDrawIndexedCommands;
			RasterParameters.DepthTarget = DebugCameraViewDepthTarget;

			mat4 Shadow = WorldUpdate.DebugView * WorldUpdate.Proj;

			Gfx.AddRasterPass<mesh_depth>("Depth prepass", DebugCameraViewDepthTarget.Width, DebugCameraViewDepthTarget.Height, Parameters, RasterParameters,
			[MeshCount = Geometries.MeshCount, Shadow, Width = DebugCameraViewDepthTarget.Width, Height = DebugCameraViewDepthTarget.Height](command_list* Cmd)
			{
				Cmd->SetViewport(0, 0, Width, Height);
				Cmd->SetConstant((void*)&Shadow, sizeof(mat4));
				Cmd->DrawIndirect(MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}

		{
			gbuffer_raster::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.VertexBuffer = VertexBuffer;
			Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;
			Parameters.MeshMaterialsBuffer = MeshMaterialsBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;

			Parameters.DiffuseTextures = DiffuseTextures;
			Parameters.NormalTextures = NormalTextures;
			Parameters.SpecularTextures = SpecularTextures;
			Parameters.HeightTextures = HeightTextures;

			gbuffer_raster::raster_parameters RasterParameters;
			RasterParameters.IndexBuffer = IndexBuffer;
			RasterParameters.IndirectBuffer = IndirectDrawIndexedCommands;
			RasterParameters.ColorTarget = GBuffer;
			RasterParameters.DepthTarget = Gfx.DepthTarget;

			Gfx.AddRasterPass<gbuffer_raster>("GBuffer generation", RenderWidth, RenderHeight, Parameters, RasterParameters,
			[MeshCount = Geometries.MeshCount, RenderWidth, RenderHeight](command_list* Cmd)
			{
				Cmd->SetViewport(0, 0, RenderWidth, RenderHeight);
				Cmd->DrawIndirect(MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}

		// TODO: Another type of global illumination for weaker systems
		if(GlobalLightSources.size())
		{
			{
				voxelization::parameters Parameters;
				Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
				Parameters.VertexBuffer = VertexBuffer;
				Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;
				Parameters.MeshMaterialsBuffer = MeshMaterialsBuffer;
				Parameters.GeometryOffsets = GeometryOffsets;
				Parameters.LightSources = LightSourcesBuffer;
				Parameters.VoxelGrid = VoxelGridTarget;
				Parameters.VoxelGridNormal = VoxelGridNormal;

				Parameters.DiffuseTextures = DiffuseTextures;
				Parameters.NormalTextures = NormalTextures;
				Parameters.SpecularTextures = SpecularTextures;
				Parameters.HeightTextures = HeightTextures;

				voxelization::raster_parameters RasterParameters = {};
				RasterParameters.IndexBuffer = IndexBuffer;
				RasterParameters.IndirectBuffer = IndirectDrawIndexedCommands;

				Gfx.AddRasterPass<voxelization>("Voxelization", VoxelGridTarget.Width, VoxelGridTarget.Height, Parameters, RasterParameters,
				[MeshCount = Geometries.MeshCount, VoxelWidth = VoxelGridTarget.Width, VoxelHeight = VoxelGridTarget.Height](command_list* Cmd)
				{
					Cmd->SetViewport(0, 0, VoxelWidth, VoxelHeight);
					Cmd->DrawIndirect(MeshCount, sizeof(indirect_draw_indexed_command));
				}, 
				[_VoxelGridTarget = Gfx.GpuMemoryHeap->GetTexture(VoxelGridTarget), _VoxelGridNormal = Gfx.GpuMemoryHeap->GetTexture(VoxelGridNormal)](command_list* Cmd)
				{
					Cmd->FillTexture(_VoxelGridTarget, vec4(0));
					Cmd->FillTexture(_VoxelGridNormal, vec4(0));
				});
			}

			for(u32 MipIdx = 1; MipIdx < VoxelGridTarget.Info.MipLevels; ++MipIdx)
			{
				vec3 VecDims(Max(1u, VoxelGridTarget.Width  >> MipIdx),
							 Max(1u, VoxelGridTarget.Height >> MipIdx),
							 Max(1u, VoxelGridTarget.Depth  >> MipIdx));

				texel_reduce_3d::parameters ReduceParameters;
				ReduceParameters.Input  = VoxelGridTarget.UseSubresource(MipIdx - 1);
				ReduceParameters.Output = VoxelGridTarget.UseSubresource(MipIdx);

				Gfx.AddComputePass<texel_reduce_3d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), ReduceParameters,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec3));
					Cmd->Dispatch(VecDims.x, VecDims.y, VecDims.z);
				});


				ReduceParameters.Input  = VoxelGridNormal.UseSubresource(MipIdx - 1);
				ReduceParameters.Output = VoxelGridNormal.UseSubresource(MipIdx);

				Gfx.AddComputePass<texel_reduce_3d>("Voxel normals mip generation pass #" + std::to_string(MipIdx), ReduceParameters,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec3));
					Cmd->Dispatch(VecDims.x, VecDims.y, VecDims.z);
				});
			}

			{
				voxel_indirect_light_calc::parameters Parameters;
				Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
				Parameters.DepthTarget = Gfx.DepthTarget;
				Parameters.GBuffer = GBuffer;
				Parameters.VoxelGrid = VoxelGridTarget;
				Parameters.VoxelGridNormal = VoxelGridNormal;
				Parameters.Out = IndirectLightOut;

				Gfx.AddComputePass<voxel_indirect_light_calc>("Draw voxel indirect light", Parameters,
				[RenderWidth, RenderHeight](command_list* Cmd)
				{
					Cmd->Dispatch(RenderWidth, RenderHeight);
				});
			}
		}

		{
			ssao::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.RandomSamplesBuffer = RandomSamplesBuffer;
			Parameters.NoiseTexture = NoiseTexture;
			Parameters.DepthTarget = Gfx.DepthTarget;
			Parameters.GBuffer = GBuffer;
			Parameters.Output = AmbientOcclusionData;

			Gfx.AddComputePass<ssao>("Screen Space Ambient Occlusion", Parameters,
			[RenderWidth, RenderHeight](command_list* Cmd)
			{
				Cmd->Dispatch(RenderWidth, RenderHeight);
			});
		}

		{
			blur::parameters Parameters;
			Parameters.Input  = AmbientOcclusionData;
			Parameters.Output = TempTexture;

			vec3 BlurInput(RenderWidth, RenderHeight, 1.0);
			Gfx.AddComputePass<blur>("Blur Vertical", Parameters,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});

			Parameters.Input  = TempTexture;
			Parameters.Output = AmbientOcclusionData;

			BlurInput.z = 0.0;
			Gfx.AddComputePass<blur>("Blur Horizontal", Parameters,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});
		}

		{
			volumetric_light_calc::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.DepthTarget = Gfx.DepthTarget;
			Parameters.GBuffer = GBuffer;
			Parameters.GlobalShadow = GlobalShadowEVSM;
			Parameters.Output = VolumetricLightOut;

			Gfx.AddComputePass<volumetric_light_calc>("Draw volumetric light", Parameters,
			[RenderWidth, RenderHeight, GScat](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&GScat, sizeof(float));
				Cmd->Dispatch(RenderWidth, RenderHeight);
			});
		}

		{
			blur::parameters Parameters;
			Parameters.Input  = VolumetricLightOut;
			Parameters.Output = TempTexture;

			vec3 BlurInput(RenderWidth, RenderHeight, 1.0);
			Gfx.AddComputePass<blur>("Blur Vertical", Parameters,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});

			Parameters.Input  = TempTexture;
			Parameters.Output = VolumetricLightOut;

			BlurInput.z = 0.0;
			Gfx.AddComputePass<blur>("Blur Horizontal", Parameters,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});
		}

		{
			// TODO: Reduce the ammount of the input parameters/symplify color pass shader
			color_pass::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.LightSourcesBuffer = LightSourcesBuffer;
			Parameters.PoissonDiskBuffer = PoissonDiskBuffer;
			Parameters.PrevColorTarget = Gfx.ColorTarget[(Gfx.BackBufferIndex + 1) % 2];
			Parameters.DepthTarget = Gfx.DepthTarget;
			Parameters.VolumetricLightTexture = VolumetricLightOut;
			Parameters.IndirectLightTexture = IndirectLightOut;
			Parameters.RandomAnglesTexture = RandomAnglesTexture;
			Parameters.GBuffer = GBuffer;
			Parameters.AmbientOcclusionData = AmbientOcclusionData;
			Parameters.GlobalShadow = GlobalShadowEVSM;

			Parameters.HdrOutput = HdrColorTarget;
			Parameters.BrightOutput = BrightTarget;

			Parameters.LightShadows = LightShadows;
			Parameters.PointLightShadows = PointLightShadows;

			Gfx.AddComputePass<color_pass>("Deferred Color Pass", Parameters,
			[RenderWidth, RenderHeight](command_list* Cmd)
			{
				Cmd->Dispatch(RenderWidth, RenderHeight);
			});
		}

		{
			for(u32 MipIdx = 1; MipIdx < BrightTarget.Info.MipLevels; ++MipIdx)
			{
				bloom_downsample::parameters Parameters;
				Parameters.Input  = BrightTarget.UseSubresource(MipIdx - 1);
				Parameters.Output = BrightTarget.UseSubresource(MipIdx);

				vec2 VecDims(Max(1u, BrightTarget.Width  >> MipIdx),
							 Max(1u, BrightTarget.Height >> MipIdx));

				Gfx.AddComputePass<bloom_downsample>("Bloom Downsample Mip: #" + std::to_string(MipIdx), Parameters,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			for(s32 MipIdx = BrightTarget.Info.MipLevels - 2; MipIdx >= 0; --MipIdx)
			{
				bloom_upsample::parameters Parameters;
				Parameters.A      = TempBrTarget.UseSubresource(MipIdx + 1);
				Parameters.B      = BrightTarget.UseSubresource(MipIdx);
				Parameters.Output = TempBrTarget.UseSubresource(MipIdx);

				vec2 VecDims(Max(1u, BrightTarget.Width  >> MipIdx),
							 Max(1u, BrightTarget.Height >> MipIdx));

				Gfx.AddComputePass<bloom_upsample>("Bloom Upsample Mip: #" + std::to_string(MipIdx), Parameters,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			{
				bloom_combine::parameters Parameters;
				Parameters.A      = HdrColorTarget;
				Parameters.B      = TempBrTarget;
				Parameters.Output = TempTexture;

				Gfx.AddComputePass<bloom_combine>("Bloom Combine", Parameters,
				[RenderWidth, RenderHeight](command_list* Cmd)
				{
					Cmd->Dispatch(RenderWidth, RenderHeight);
				});
			}
		}

		{
			fxaa::parameters Parameters;
			Parameters.Input  = TempTexture;
			Parameters.Output = Gfx.ColorTarget[Gfx.BackBufferIndex];

			Gfx.AddComputePass<fxaa>("FXAA Pass", Parameters,
			[RenderWidth, RenderHeight](command_list* Cmd)
			{
				Cmd->Dispatch(RenderWidth, RenderHeight);
			});
		}

		{
			for(u32 MipIdx = 0; MipIdx < DepthPyramid.Info.MipLevels; ++MipIdx)
			{
				vec2 VecDims(Max(1u, DepthPyramid.Width  >> MipIdx),
						     Max(1u, DepthPyramid.Height >> MipIdx));

				texel_reduce_2d::parameters Parameters;
				if(MipIdx == 0)
					Parameters.Input = DebugCameraViewDepthTarget.UseSubresource(MipIdx);
				else
					Parameters.Input = DepthPyramid.UseSubresource(MipIdx - 1);
				Parameters.Output = DepthPyramid.UseSubresource(MipIdx);

				Gfx.AddComputePass<texel_reduce_2d>("Depth Pyramid Generation: #" + std::to_string(MipIdx), Parameters,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}
		}

		{
			occlusion_culling::parameters Parameters;
			Parameters.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;
			Parameters.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.DepthPyramid = DepthPyramid;
			Parameters.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddComputePass<occlusion_culling>("Occlusion culling", Parameters,
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size()](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstancesCount);
			});
		}
	}
};
