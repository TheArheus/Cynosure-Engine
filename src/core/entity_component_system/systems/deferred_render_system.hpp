#pragma once

struct deferred_raster_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh::material> Materials;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	// TODO: Arrays
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

	resource_descriptor MeshDrawVisibilityDataBuffer;
	resource_descriptor IndirectDrawIndexedCommands;
	resource_descriptor MeshDrawCommandBuffer;

	resource_descriptor MeshDrawShadowVisibilityDataBuffer;
	resource_descriptor ShadowIndirectDrawIndexedCommands;
	resource_descriptor MeshDrawShadowCommandBuffer;

	system_constructor(deferred_raster_system)
	{
		RequireComponent<mesh_component>();
		RequireComponent<static_instances_component>();
	}

	// TODO: Move to/create a asset storage
	void Setup(window& Window, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput)
	{
		texture_data Texture = {};
		mesh::material NewMaterial = {};
		utils::texture::input_data TextureInputData = {};
		TextureInputData.Format    = image_format::R8G8B8A8_SRGB;
		TextureInputData.Usage     = image_flags::TF_Sampled | image_flags::TF_CopyDst | image_flags::TF_ColorTexture;
		TextureInputData.Type	   = image_type::Texture2D;
		TextureInputData.MipLevels = 1;
		TextureInputData.Layers    = 1;

		vec3 SceneMin = vec3( FLT_MAX);
		vec3 SceneMax = vec3(-FLT_MAX);

		u32 TextureIdx   = 0;
		u32 NormalMapIdx = 0;
		u32 SpecularIdx  = 0;
		u32 HeightMapIdx = 0;
		u32 InstanceOffset = 0;
		for(entity& Entity : Entities)
		{
			NewMaterial = {};
			u32 EntityIdx = *(u32*)&Entity.Handle;
			color_component* Color = Entity.GetComponent<color_component>();
			emmit_component* Emmit = Entity.GetComponent<emmit_component>();
			mesh_component* Mesh = Entity.GetComponent<mesh_component>();
			debug_component* Debug = Entity.GetComponent<debug_component>();
			diffuse_component* Diffuse = Entity.GetComponent<diffuse_component>();
			normal_map_component* NormalMap = Entity.GetComponent<normal_map_component>();
			specular_map_component* SpecularMap = Entity.GetComponent<specular_map_component>();
			height_map_component* HeightMap = Entity.GetComponent<height_map_component>();
			static_instances_component* InstancesComponent = Entity.GetComponent<static_instances_component>();

			NewMaterial.LightDiffuse = vec4(vec3(1.0), 1.0);
			NewMaterial.LightEmmit   = vec4(vec3(0.0), 1.0);
			if(Color)
			{
				NewMaterial.LightDiffuse = vec4(Color->Data, 1.0);
			}
			if(Emmit)
			{
				NewMaterial.LightEmmit   = vec4(Emmit->Data, Emmit->Intensity);
			}
			if(Diffuse)
			{
				Texture.Load(Diffuse->Data);
				DiffuseTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("DiffuseTexture" + std::to_string(TextureIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasTexture = true;
				NewMaterial.TextureIdx = TextureIdx++;
				Texture.Delete();
			}
			if(NormalMap)
			{
				Texture.Load(NormalMap->Data);
				NormalTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("NormalTexture" + std::to_string(NormalMapIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasNormalMap = true;
				NewMaterial.NormalMapIdx = NormalMapIdx++;
				Texture.Delete();
			}
			if(SpecularMap)
			{
				Texture.Load(SpecularMap->Data);
				SpecularTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("SpecularTexture" + std::to_string(SpecularIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasSpecularMap = true;
				NewMaterial.SpecularMapIdx = SpecularIdx++;
				Texture.Delete();
			}
			if(HeightMap)
			{
				Texture.Load(HeightMap->Data);
				HeightTextures.push_back(Window.Gfx.GpuMemoryHeap->CreateTexture("HeightTexture" + std::to_string(HeightMapIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasHeightMap = true;
				NewMaterial.HeightMapIdx = HeightMapIdx++;
				Texture.Delete();
			}

			vec3 MeshCenter = Mesh->Data.Offsets[0].BoundingSphere.Center.xyz;
			vec3 MeshMin = Mesh->Data.Offsets[0].AABB.Min.xyz;
			vec3 MeshMax = Mesh->Data.Offsets[0].AABB.Max.xyz;
			if(Mesh)
			{
				Geometries.Load(Mesh->Data);
			}

			if(InstancesComponent)
			{
				Geometries.Offsets[EntityIdx - 1].InstanceOffset = InstanceOffset;
				InstanceOffset += InstancesComponent->Data.size();
				for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
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
		MeshMaterialsBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), RF_StorageBuffer | RF_CopyDst);
		LightSourcesBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("LightSourcesBuffer", sizeof(light_source), LIGHT_SOURCES_MAX_COUNT, RF_StorageBuffer | RF_CopyDst);

		VertexBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("VertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), RF_StorageBuffer | RF_CopyDst);
		IndexBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("IndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), RF_IndexBuffer | RF_CopyDst);

		GeometryOffsets = Window.Gfx.GpuMemoryHeap->CreateBuffer("GeometryOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), RF_StorageBuffer | RF_CopyDst);
		MeshDrawCommandDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_CopyDst);
		MeshDrawVisibilityDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopySrc | RF_CopyDst);
		IndirectDrawIndexedCommands = Window.Gfx.GpuMemoryHeap->CreateBuffer("IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopyDst | RF_WithCounter);

		MeshDrawCommandBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		MeshDrawShadowVisibilityDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawShadowVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer | RF_CopySrc | RF_CopyDst);
		ShadowIndirectDrawIndexedCommands = Window.Gfx.GpuMemoryHeap->CreateBuffer("ShadowIndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopyDst | RF_WithCounter);
		MeshDrawShadowCommandBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawShadowCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_StorageBuffer | RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, RF_StorageBuffer | RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, RF_StorageBuffer | RF_CopyDst);
	}

	// TODO: Move out all render commands. There will be only a geometry render command generation I guess
	void Render(global_graphics_context& Gfx, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput, alloc_vector<light_source>& GlobalLightSources)
	{
		{
			u32 PointLightSourceCount = 0;
			u32 SpotLightSourceCount = 0;

			utils::texture::input_data TextureInputData = {};
			if((LightShadows.size() + PointLightShadows.size()) != GlobalLightSources.size())
			{
				LightShadows.clear();
				PointLightShadows.clear();

				TextureInputData.Format = image_format::D32_SFLOAT;
				TextureInputData.Usage  = image_flags::TF_DepthTexture | image_flags::TF_Sampled;
				TextureInputData.Type = image_type::Texture2D;
				for(const light_source& LightSource : GlobalLightSources)
				{
					if(LightSource.Type == light_type_point)
					{
						TextureInputData.Usage |= image_flags::TF_CubeMap;
						TextureInputData.MipLevels = 1;
						TextureInputData.Layers    = 6;
						if(WorldUpdate.LightSourceShadowsEnabled)
							PointLightShadows.push_back(Gfx.GpuMemoryHeap->CreateTexture("PointLightShadow" + std::to_string(PointLightSourceCount), Gfx.Backend->Width, Gfx.Backend->Width, 1, TextureInputData));
						PointLightSourceCount++;
					}
					else if(LightSource.Type == light_type_spot)
					{
						TextureInputData.MipLevels = 1;
						TextureInputData.Layers    = 1;
						if(WorldUpdate.LightSourceShadowsEnabled)
							LightShadows.push_back(Gfx.GpuMemoryHeap->CreateTexture("LightShadow" + std::to_string(SpotLightSourceCount), Gfx.Backend->Width, Gfx.Backend->Height, 1, TextureInputData));
						SpotLightSourceCount++;
					}
				}
			}
		}

		{
			Gfx.AddTransferPass("Data upload",
			[&WorldUpdate, &MeshCommonCullingInput, &GlobalLightSources, WorldUpdateBuffer = Gfx.GpuMemoryHeap->GetBuffer(WorldUpdateBuffer), MeshCommonCullingInputBuffer = Gfx.GpuMemoryHeap->GetBuffer(MeshCommonCullingInputBuffer), LightSourcesBuffer = Gfx.GpuMemoryHeap->GetBuffer(LightSourcesBuffer)](command_list* Cmd)
			{
				WorldUpdateBuffer->UpdateSize(&WorldUpdate, sizeof(global_world_data), Cmd);
				MeshCommonCullingInputBuffer->UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), Cmd);
				LightSourcesBuffer->UpdateSize(GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), Cmd);
			});
		}

		{
			frustum_culling::parameters Parameters;
			Parameters.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;
			Parameters.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;
			Parameters.IndirectDrawIndexedCommands = IndirectDrawIndexedCommands;
			Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<frustum_culling>("Frustum culling/indirect command generation", Parameters, pass_type::compute, 
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size()](command_list* Cmd)
			{
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
			Gfx.AddPass<generate_all>("Frustum culling/indirect command generation for shadow maps", Parameters, pass_type::compute,
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size()](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstancesCount);
			});
		}

		{
			mesh_shadow::parameters Parameters;
			Parameters.VertexBuffer = VertexBuffer;
			Parameters.CommandBuffer = MeshDrawShadowCommandBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;

			for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
			{
				mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];
				texture* CascadeShadowTarget = Gfx.GpuMemoryHeap->GetTexture(Gfx.GlobalShadow[CascadeIdx]);

				Gfx.AddPass<mesh_shadow>("Cascade shadow map generation: #" + std::to_string(CascadeIdx), Parameters, pass_type::graphics, 
				[MeshCount = Geometries.MeshCount, Shadow, CascadeShadowTarget, IndexBuffer = Gfx.GpuMemoryHeap->GetBuffer(IndexBuffer), ShadowIndirectDrawIndexedCommands = Gfx.GpuMemoryHeap->GetBuffer(ShadowIndirectDrawIndexedCommands)](command_list* Cmd)
				{
					Cmd->SetViewport(0, 0, CascadeShadowTarget->Width, CascadeShadowTarget->Height);
					Cmd->SetDepthTarget(CascadeShadowTarget);

					Cmd->SetConstant((void*)&Shadow, sizeof(mat4));

					Cmd->SetIndexBuffer(IndexBuffer);
					Cmd->DrawIndirect(ShadowIndirectDrawIndexedCommands, MeshCount, sizeof(indirect_draw_indexed_command));
				});
			}
		}

#if 0
		{
			shader_parameter<depth_prepass> Parameters;
			Parameters.Param.VertexBuffer = Gfx.UseBuffer(VertexBuffer);
			Parameters.Param.CommandBuffer = Gfx.UseBuffer(MeshDrawCommandBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);

			mat4 Shadow = WorldUpdate.DebugView * WorldUpdate.Proj;
			texture* DebugCameraViewDepthTarget = Gfx.DebugCameraViewDepthTarget;

			Gfx.AddPass<depth_prepass>("Depth prepass", Parameters, pass_type::graphics, 
			[Shadow, DebugCameraViewDepthTarget](command_list* Cmd)
			{
				Cmd->SetViewport(0, 0, DebugCameraViewDepthTarget->Width, DebugCameraViewDepthTarget->Height);
				Cmd->SetDepthTarget(DebugCameraViewDepthTarget);

				Cmd->SetConstant((void*)&Shadow, sizeof(mat4));

				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}
#endif

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

			Gfx.AddPass<gbuffer_raster>("GBuffer generation", Parameters, pass_type::graphics, 
			[MeshCount = Geometries.MeshCount, IndirectDrawIndexedCommands = Gfx.GpuMemoryHeap->GetBuffer(IndirectDrawIndexedCommands), IndexBuffer = Gfx.GpuMemoryHeap->GetBuffer(IndexBuffer), GBuffer = Gfx.GpuMemoryHeap->GetTexture(Gfx.GBuffer), GfxDepthTarget = Gfx.GpuMemoryHeap->GetTexture(Gfx.GfxDepthTarget)](command_list* Cmd)
			{

				Cmd->SetViewport(0, 0, GBuffer[0]->Width, GBuffer[0]->Height);
				Cmd->SetColorTarget(GBuffer);
				Cmd->SetDepthTarget(GfxDepthTarget);

				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}

		// TODO: Another type of global illumination for weaker systems
		{
			voxelization::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.VertexBuffer = VertexBuffer;
			Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;
			Parameters.MeshMaterialsBuffer = MeshMaterialsBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;
			Parameters.LightSources = LightSourcesBuffer;
			Parameters.VoxelGrid = Gfx.VoxelGridTarget;
			Parameters.VoxelGridNormal = Gfx.VoxelGridNormal;

			Parameters.DiffuseTextures = DiffuseTextures;
			Parameters.NormalTextures = NormalTextures;
			Parameters.SpecularTextures = SpecularTextures;
			Parameters.HeightTextures = HeightTextures;

			Gfx.AddPass<voxelization>("Voxelization", Parameters, pass_type::graphics, 
			[MeshCount = Geometries.MeshCount, IndexBuffer = Gfx.GpuMemoryHeap->GetBuffer(IndexBuffer), IndirectDrawIndexedCommands = Gfx.GpuMemoryHeap->GetBuffer(IndirectDrawIndexedCommands), VoxelGridTarget = Gfx.GpuMemoryHeap->GetTexture(Gfx.VoxelGridTarget), VoxelGridNormal = Gfx.GpuMemoryHeap->GetTexture(Gfx.VoxelGridNormal)](command_list* Cmd)
			{
				Cmd->FillTexture(VoxelGridTarget, vec4(0));
				Cmd->FillTexture(VoxelGridNormal, vec4(0));
				Cmd->SetViewport(0, 0, VoxelGridTarget->Width, VoxelGridTarget->Height);
				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, MeshCount, sizeof(indirect_draw_indexed_command));
			});

			for(u32 MipIdx = 1; MipIdx < Gfx.VoxelGridTarget.Info.MipLevels; ++MipIdx)
			{
				vec3 VecDims(Max(1u, Gfx.VoxelGridTarget.Width  >> MipIdx),
							 Max(1u, Gfx.VoxelGridTarget.Height >> MipIdx),
							 Max(1u, Gfx.VoxelGridTarget.Depth  >> MipIdx));

				texel_reduce_3d::parameters ReduceParameters;
				ReduceParameters.Input  = Gfx.VoxelGridTarget.UseSubresource(MipIdx - 1);
				ReduceParameters.Output = Gfx.VoxelGridTarget.UseSubresource(MipIdx);

				Gfx.AddPass<texel_reduce_3d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), ReduceParameters, pass_type::compute,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec3));
					Cmd->Dispatch(VecDims.x, VecDims.y, VecDims.z);
				});


				ReduceParameters.Input  = Gfx.VoxelGridNormal.UseSubresource(MipIdx - 1);
				ReduceParameters.Output = Gfx.VoxelGridNormal.UseSubresource(MipIdx);

				Gfx.AddPass<texel_reduce_3d>("Voxel normals mip generation pass #" + std::to_string(MipIdx), ReduceParameters, pass_type::compute,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec3));
					Cmd->Dispatch(VecDims.x, VecDims.y, VecDims.z);
				});
			}
		}

		{
			ssao::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.RandomSamplesBuffer = Gfx.RandomSamplesBuffer;
			Parameters.NoiseTexture = Gfx.NoiseTexture;
			Parameters.DepthTarget = Gfx.GfxDepthTarget;
			Parameters.GBuffer = Gfx.GBuffer;
			Parameters.Output = Gfx.AmbientOcclusionData;

			Gfx.AddPass<ssao>("Screen Space Ambient Occlusion", Parameters, pass_type::compute,
			[Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd)
			{
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			blur::parameters Parameters;
			Parameters.Input  = Gfx.AmbientOcclusionData;
			Parameters.Output = Gfx.BlurTemp;

			vec3 BlurInput(Gfx.Backend->Width, Gfx.Backend->Height, 1.0);
			Gfx.AddPass<blur>("Blur Vertical", Parameters, pass_type::compute,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});

			Parameters.Input  = Gfx.BlurTemp;
			Parameters.Output = Gfx.AmbientOcclusionData;

			BlurInput.z = 0.0;
			Gfx.AddPass<blur>("Blur Horizontal", Parameters, pass_type::compute,
			[BlurInput](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});
		}

		{
			volumetric_light_calc::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.DepthTarget = Gfx.GfxDepthTarget;
			Parameters.GBuffer = Gfx.GBuffer;
			Parameters.GlobalShadow = Gfx.GlobalShadow;
			Parameters.Output = Gfx.VolumetricLightOut;

			float GScat = 0.7;
			Gfx.AddPass<volumetric_light_calc>("Draw volumetric light", Parameters, pass_type::compute,
			[Width = Gfx.Backend->Width, Height = Gfx.Backend->Height, GScat](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&GScat, sizeof(float));
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			voxel_indirect_light_calc::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.DepthTarget = Gfx.GfxDepthTarget;
			Parameters.GBuffer = Gfx.GBuffer;
			Parameters.VoxelGrid = Gfx.VoxelGridTarget;
			Parameters.VoxelGridNormal = Gfx.VoxelGridNormal;
			Parameters.Out = Gfx.IndirectLightOut;

			Gfx.AddPass<voxel_indirect_light_calc>("Draw voxel indirect light", Parameters, pass_type::compute,
			[Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd)
			{
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			// TODO: Reduce the ammount of the input parameters/symplify color pass shader
			color_pass::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.LightSourcesBuffer = LightSourcesBuffer;
			Parameters.PoissonDiskBuffer = Gfx.PoissonDiskBuffer;
			Parameters.RandomSamplesBuffer = Gfx.RandomSamplesBuffer;
			Parameters.PrevColorTarget = Gfx.GfxColorTarget[(Gfx.BackBufferIndex + 1) % 2];
			Parameters.GfxDepthTarget = Gfx.GfxDepthTarget;
			Parameters.VolumetricLightTexture = Gfx.VolumetricLightOut;
			Parameters.IndirectLightTexture = Gfx.IndirectLightOut;
			Parameters.RandomAnglesTexture = Gfx.RandomAnglesTexture;
			Parameters.GBuffer = Gfx.GBuffer;
			Parameters.AmbientOcclusionData = Gfx.AmbientOcclusionData;
			Parameters.GlobalShadow = Gfx.GlobalShadow;

			Parameters.HdrOutput = Gfx.HdrColorTarget;
			Parameters.BrightOutput = Gfx.BrightTarget;

			Parameters.LightShadows = LightShadows;
			Parameters.PointLightShadows = PointLightShadows;

			Gfx.AddPass<color_pass>("Deferred Color Pass", Parameters, pass_type::compute,
			[Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd)
			{
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			for(u32 MipIdx = 1; MipIdx < Gfx.BrightTarget.Info.MipLevels; ++MipIdx)
			{
				bloom_downsample::parameters Parameters;
				Parameters.Input  = Gfx.BrightTarget.UseSubresource(MipIdx - 1);
				Parameters.Output = Gfx.BrightTarget.UseSubresource(MipIdx);

				vec2 VecDims(Max(1u, Gfx.BrightTarget.Width  >> MipIdx),
							 Max(1u, Gfx.BrightTarget.Height >> MipIdx));

				Gfx.AddPass<bloom_downsample>("Bloom Downsample Mip: #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			for(s32 MipIdx = Gfx.BrightTarget.Info.MipLevels - 2; MipIdx >= 0; --MipIdx)
			{
				bloom_upsample::parameters Parameters;
				Parameters.A      = Gfx.TempBrTarget.UseSubresource(MipIdx + 1);
				Parameters.B      = Gfx.BrightTarget.UseSubresource(MipIdx);
				Parameters.Output = Gfx.TempBrTarget.UseSubresource(MipIdx);

				vec2 VecDims(Max(1u, Gfx.BrightTarget.Width  >> MipIdx),
							 Max(1u, Gfx.BrightTarget.Height >> MipIdx));

				Gfx.AddPass<bloom_upsample>("Bloom Upsample Mip: #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[VecDims](command_list* Cmd)
				{
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			{
				bloom_combine::parameters Parameters;
				Parameters.A      = Gfx.HdrColorTarget;
				Parameters.B      = Gfx.TempBrTarget;
				Parameters.Output = Gfx.GfxColorTarget[Gfx.BackBufferIndex];

				Gfx.AddPass<bloom_combine>("Bloom Combine", Parameters, pass_type::compute,
				[Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd)
				{
					Cmd->Dispatch(Width, Height);
				});
			}
		}

		{
			u32 MipCount = GetImageMipLevels(PreviousPowerOfTwo(Gfx.DepthPyramid.Width), PreviousPowerOfTwo(Gfx.DepthPyramid.Height));
			for(u32 MipIdx = 0; MipIdx < MipCount; ++MipIdx)
			{
				vec2 VecDims(Max(1u, Gfx.DepthPyramid.Width  >> MipIdx),
							 Max(1u, Gfx.DepthPyramid.Height >> MipIdx));

				texel_reduce_2d::parameters Parameters;
				if(MipIdx == 0)
					Parameters.Input = Gfx.GfxDepthTarget.UseSubresource(MipIdx);
				else
					Parameters.Input = Gfx.DepthPyramid.UseSubresource(MipIdx - 1);
				Parameters.Output = Gfx.DepthPyramid.UseSubresource(MipIdx);

				Gfx.AddPass<texel_reduce_2d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), Parameters, pass_type::compute,
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
			Parameters.DepthPyramid = Gfx.DepthPyramid;
			Parameters.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<occlusion_culling>("Occlusion culling", Parameters, pass_type::compute,
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size()](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstancesCount);
			});
		}
	}
};
