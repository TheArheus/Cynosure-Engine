#pragma once

struct deferred_raster_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh::material> Materials;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	std::vector<texture*> DiffuseTextures;
	std::vector<texture*> NormalTextures;
	std::vector<texture*> SpecularTextures;
	std::vector<texture*> HeightTextures;

	std::vector<texture*> LightShadows;
	std::vector<texture*> PointLightShadows;

	buffer* WorldUpdateBuffer;
	buffer* MeshCommonCullingInputBuffer;
	buffer* MeshDrawCommandDataBuffer;

	buffer* GeometryOffsets;
	buffer* VertexBuffer;
	buffer* IndexBuffer;

	buffer* MeshMaterialsBuffer;
	buffer* LightSourcesBuffer;

	buffer* MeshDrawVisibilityDataBuffer;
	buffer* IndirectDrawIndexedCommands;
	buffer* MeshDrawCommandBuffer;

	buffer* MeshDrawShadowVisibilityDataBuffer;
	buffer* ShadowIndirectDrawIndexedCommands;
	buffer* MeshDrawShadowCommandBuffer;

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
				DiffuseTextures.push_back(Window.Gfx.PushTexture("DiffuseTexture" + std::to_string(TextureIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasTexture = true;
				NewMaterial.TextureIdx = TextureIdx++;
				Texture.Delete();
			}
			if(NormalMap)
			{
				Texture.Load(NormalMap->Data);
				NormalTextures.push_back(Window.Gfx.PushTexture("NormalTexture" + std::to_string(NormalMapIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasNormalMap = true;
				NewMaterial.NormalMapIdx = NormalMapIdx++;
				Texture.Delete();
			}
			if(SpecularMap)
			{
				Texture.Load(SpecularMap->Data);
				SpecularTextures.push_back(Window.Gfx.PushTexture("SpecularTexture" + std::to_string(SpecularIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasSpecularMap = true;
				NewMaterial.SpecularMapIdx = SpecularIdx++;
				Texture.Delete();
			}
			if(HeightMap)
			{
				Texture.Load(HeightMap->Data);
				HeightTextures.push_back(Window.Gfx.PushTexture("HeightTexture" + std::to_string(HeightMapIdx), Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
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
		MeshMaterialsBuffer = Window.Gfx.PushBuffer("MeshMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), RF_StorageBuffer | RF_CopyDst);
		LightSourcesBuffer = Window.Gfx.PushBuffer("LightSourcesBuffer", sizeof(light_source), LIGHT_SOURCES_MAX_COUNT, RF_StorageBuffer | RF_CopyDst);

		VertexBuffer = Window.Gfx.PushBuffer("VertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), RF_StorageBuffer | RF_CopyDst);
		IndexBuffer = Window.Gfx.PushBuffer("IndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), RF_IndexBuffer | RF_CopyDst);

		GeometryOffsets = Window.Gfx.PushBuffer("GeometryOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), RF_StorageBuffer | RF_CopyDst);
		MeshDrawCommandDataBuffer = Window.Gfx.PushBuffer("MeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_CopyDst);
		MeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer("MeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopySrc | RF_CopyDst);
		IndirectDrawIndexedCommands = Window.Gfx.PushBuffer("IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopyDst | RF_WithCounter);

		MeshDrawCommandBuffer = Window.Gfx.PushBuffer("MeshDrawCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		MeshDrawShadowVisibilityDataBuffer = Window.Gfx.PushBuffer("MeshDrawShadowVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer | RF_CopySrc | RF_CopyDst);
		ShadowIndirectDrawIndexedCommands = Window.Gfx.PushBuffer("ShadowIndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopyDst | RF_WithCounter);
		MeshDrawShadowCommandBuffer = Window.Gfx.PushBuffer("MeshDrawShadowCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_StorageBuffer | RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.PushBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, RF_StorageBuffer | RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, RF_StorageBuffer | RF_CopyDst);
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
							PointLightShadows.push_back(Gfx.PushTexture("PointLightShadow" + std::to_string(PointLightSourceCount), nullptr, Gfx.GfxColorTarget[0]->Width, Gfx.GfxColorTarget[0]->Width, 1, TextureInputData));
						PointLightSourceCount++;
					}
					else if(LightSource.Type == light_type_spot)
					{
						TextureInputData.MipLevels = 1;
						TextureInputData.Layers    = 1;
						if(WorldUpdate.LightSourceShadowsEnabled)
							LightShadows.push_back(Gfx.PushTexture("LightShadow" + std::to_string(SpotLightSourceCount), nullptr, Gfx.GfxColorTarget[0]->Width, Gfx.GfxColorTarget[0]->Height, 1, TextureInputData));
						SpotLightSourceCount++;
					}
				}
			}
		}

		{
			Gfx.AddTransferPass("Data upload",
			[this, &WorldUpdate, &MeshCommonCullingInput, &GlobalLightSources](command_list* Cmd, void* Parameters)
			{
				WorldUpdateBuffer->UpdateSize(&WorldUpdate, sizeof(global_world_data), Cmd);
				MeshCommonCullingInputBuffer->UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), Cmd);
				LightSourcesBuffer->UpdateSize(GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), Cmd);
			});
		}

		{
			shader_parameter<frustum_culling> Parameters;
			Parameters.Param.MeshCommonCullingInputBuffer = Gfx.UseBuffer(MeshCommonCullingInputBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);
			Parameters.Param.MeshDrawCommandDataBuffer = Gfx.UseBuffer(MeshDrawCommandDataBuffer);
			Parameters.Param.MeshDrawVisibilityDataBuffer = Gfx.UseBuffer(MeshDrawVisibilityDataBuffer);
			Parameters.Param.IndirectDrawIndexedCommands = Gfx.UseBuffer(IndirectDrawIndexedCommands);
			Parameters.Param.MeshDrawCommandBuffer = Gfx.UseBuffer(MeshDrawCommandBuffer);

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<frustum_culling>("Frustum culling/indirect command generation", Parameters, pass_type::compute, 
			[this, Input](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}

#if 0
		{
			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<frustum_culling>("Frustum culling/indirect command generation", Parameters, pass_type::compute, 
			[](resource_scheduler* Scheduler)
			{
				Parameters.MeshCommonCullingInputBuffer = Scheduler.UseBuffer("MeshCommonCullingInputBuffer", resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
				Parameters.GeometryOffsets = Scheduler.UseBuffer("GeometryOffsets", resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
				Parameters.MeshDrawCommandDataBuffer = Scheduler.UseBuffer("MeshDrawCommandDataBuffer", resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
				Parameters.MeshDrawVisibilityDataBuffer = Scheduler.UseBuffer("MeshDrawVisibilityDataBuffer", resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);
				Parameters.IndirectDrawIndexedCommands = Scheduler.UseBuffer("IndirectDrawIndexedCommands", resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst | resource_flags::RF_WithCounter);
				Parameters.MeshDrawCommandBuffer = Scheduler.UseBuffer("MeshDrawCommandBuffer", resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
			},
			[this, Input](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}
#endif

		{
			shader_parameter<generate_all> Parameters;
			Parameters.Param.MeshCommonCullingInputBuffer = Gfx.UseBuffer(MeshCommonCullingInputBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);
			Parameters.Param.MeshDrawCommandDataBuffer = Gfx.UseBuffer(MeshDrawCommandDataBuffer);
			Parameters.Param.MeshDrawVisibilityDataBuffer = Gfx.UseBuffer(MeshDrawShadowVisibilityDataBuffer);
			Parameters.Param.IndirectDrawIndexedCommands = Gfx.UseBuffer(ShadowIndirectDrawIndexedCommands);
			Parameters.Param.MeshDrawCommandBuffer = Gfx.UseBuffer(MeshDrawShadowCommandBuffer);

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<generate_all>("Frustum culling/indirect command generation for shadow maps", Parameters, pass_type::compute,
			[this, Input](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}

		{
			shader_parameter<mesh_shadow> Parameters;
			Parameters.Param.VertexBuffer = Gfx.UseBuffer(VertexBuffer);
			Parameters.Param.CommandBuffer = Gfx.UseBuffer(MeshDrawShadowCommandBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);

			for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
			{
				mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];
				texture* CascadeShadowTarget = Gfx.GlobalShadow[CascadeIdx];

				Gfx.AddPass<mesh_shadow>("Cascade shadow map generation: #" + std::to_string(CascadeIdx), Parameters, pass_type::graphics, 
				[this, Shadow, CascadeShadowTarget](command_list* Cmd, void* Parameters)
				{
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetViewport(0, 0, CascadeShadowTarget->Width, CascadeShadowTarget->Height);
					Cmd->SetDepthTarget(CascadeShadowTarget);

					Cmd->SetConstant((void*)&Shadow, sizeof(mat4));

					Cmd->SetIndexBuffer(IndexBuffer);
					Cmd->DrawIndirect(ShadowIndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
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
			[this, Shadow, DebugCameraViewDepthTarget](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);

				Cmd->SetViewport(0, 0, DebugCameraViewDepthTarget->Width, DebugCameraViewDepthTarget->Height);
				Cmd->SetDepthTarget(DebugCameraViewDepthTarget);

				Cmd->SetConstant((void*)&Shadow, sizeof(mat4));

				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}
#endif

		{
			shader_parameter<gbuffer_raster> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.VertexBuffer = Gfx.UseBuffer(VertexBuffer);
			Parameters.Param.MeshDrawCommandBuffer = Gfx.UseBuffer(MeshDrawCommandBuffer);
			Parameters.Param.MeshMaterialsBuffer = Gfx.UseBuffer(MeshMaterialsBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);

			Parameters.StaticStorage.DiffuseTextures = Gfx.UseTextureArray(DiffuseTextures);
			Parameters.StaticStorage.NormalTextures = Gfx.UseTextureArray(NormalTextures);
			Parameters.StaticStorage.SpecularTextures = Gfx.UseTextureArray(SpecularTextures);
			Parameters.StaticStorage.HeightTextures = Gfx.UseTextureArray(HeightTextures);

			Gfx.AddPass<gbuffer_raster>("GBuffer generation", Parameters, pass_type::graphics, 
			[this, GBuffer = Gfx.GBuffer, GfxDepthTarget = Gfx.GfxDepthTarget](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);

				Cmd->SetViewport(0, 0, GBuffer[0]->Width, GBuffer[0]->Height);
				Cmd->SetColorTarget(GBuffer);
				Cmd->SetDepthTarget(GfxDepthTarget);

				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}

		// TODO: Another type of global illumination for weaker systems
		{
			shader_parameter<voxelization> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.VertexBuffer = Gfx.UseBuffer(VertexBuffer);
			Parameters.Param.MeshDrawCommandBuffer = Gfx.UseBuffer(MeshDrawCommandBuffer);
			Parameters.Param.MeshMaterialsBuffer = Gfx.UseBuffer(MeshMaterialsBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);
			Parameters.Param.LightSources = Gfx.UseBuffer(LightSourcesBuffer);
			Parameters.Param.VoxelGrid = Gfx.UseTexture(Gfx.VoxelGridTarget);

			Parameters.StaticStorage.DiffuseTextures = Gfx.UseTextureArray(DiffuseTextures);
			Parameters.StaticStorage.NormalTextures = Gfx.UseTextureArray(NormalTextures);
			Parameters.StaticStorage.SpecularTextures = Gfx.UseTextureArray(SpecularTextures);
			Parameters.StaticStorage.HeightTextures = Gfx.UseTextureArray(HeightTextures);

			Gfx.AddPass<voxelization>("Voxelization", Parameters, pass_type::graphics, 
			[this, VoxelGridTarget = Gfx.VoxelGridTarget](command_list* Cmd, void* Parameters)
			{
				Cmd->FillTexture(VoxelGridTarget, vec4(0));
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetViewport(0, 0, VoxelGridTarget->Width, VoxelGridTarget->Height);
				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
			});

			for(u32 MipIdx = 1; MipIdx < Gfx.VoxelGridTarget->Info.MipLevels; ++MipIdx)
			{
				vec3 VecDims(Max(1u, Gfx.VoxelGridTarget->Width  >> MipIdx),
							 Max(1u, Gfx.VoxelGridTarget->Height >> MipIdx),
							 Max(1u, Gfx.VoxelGridTarget->Depth  >> MipIdx));

				shader_parameter<texel_reduce_3d> ReduceParameters;
				ReduceParameters.Param.Input  = Gfx.UseTextureMip(Gfx.VoxelGridTarget, MipIdx - 1);
				ReduceParameters.Param.Output = Gfx.UseTextureMip(Gfx.VoxelGridTarget, MipIdx);

				Gfx.AddPass<texel_reduce_3d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), ReduceParameters, pass_type::compute,
				[this, VecDims](command_list* Cmd, void* Parameters)
				{
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec3));
					Cmd->Dispatch(VecDims.x, VecDims.y, VecDims.z);
				});
			}
		}

		{
			shader_parameter<ssao> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.RandomSamplesBuffer = Gfx.UseBuffer(Gfx.RandomSamplesBuffer);
			Parameters.Param.NoiseTexture = Gfx.UseTexture(Gfx.NoiseTexture);
			Parameters.Param.DepthTarget = Gfx.UseTexture(Gfx.GfxDepthTarget);
			Parameters.Param.GBuffer = Gfx.UseTextureArray(Gfx.GBuffer);
			Parameters.Param.Output = Gfx.UseTexture(Gfx.AmbientOcclusionData);

			Gfx.AddPass<ssao>("Screen Space Ambient Occlusion", Parameters, pass_type::compute,
			[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			shader_parameter<blur> Parameters;
			Parameters.Param.Input   = Gfx.UseTexture(Gfx.AmbientOcclusionData);
			Parameters.Param.Output = Gfx.UseTexture(Gfx.BlurTemp);

			vec3 BlurInput(Gfx.Backend->Width, Gfx.Backend->Height, 1.0);
			Gfx.AddPass<blur>("Blur Vertical", Parameters, pass_type::compute,
			[this, BlurInput](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});

			Parameters.Param.Input   = Gfx.UseTexture(Gfx.BlurTemp);
			Parameters.Param.Output = Gfx.UseTexture(Gfx.AmbientOcclusionData);

			BlurInput.z = 0.0;
			Gfx.AddPass<blur>("Blur Horizontal", Parameters, pass_type::compute,
			[this, BlurInput](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});
		}

		{
			shader_parameter<volumetric_light_calc> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.DepthTarget = Gfx.UseTexture(Gfx.GfxDepthTarget);
			Parameters.Param.GBuffer = Gfx.UseTextureArray(Gfx.GBuffer);
			Parameters.Param.GlobalShadow = Gfx.UseTextureArray(Gfx.GlobalShadow);
			Parameters.Param.Out = Gfx.UseTexture(Gfx.VolumetricLightOut);

			float GScat = 0.7;
			Gfx.AddPass<volumetric_light_calc>("Draw volumetric light", Parameters, pass_type::compute,
			[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height, GScat](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&GScat, sizeof(float));
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			shader_parameter<voxel_indirect_light_calc> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.DepthTarget = Gfx.UseTexture(Gfx.GfxDepthTarget);
			Parameters.Param.GBuffer = Gfx.UseTextureArray(Gfx.GBuffer);
			Parameters.Param.VoxelGrid = Gfx.UseTexture(Gfx.VoxelGridTarget);
			Parameters.Param.Out = Gfx.UseTexture(Gfx.IndirectLightOut);

			Gfx.AddPass<voxel_indirect_light_calc>("Draw voxel indirect light", Parameters, pass_type::compute,
			[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			// TODO: Reduce the ammount of the input parameters/symplify color pass shader
			shader_parameter<color_pass> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.LightSourcesBuffer = Gfx.UseBuffer(LightSourcesBuffer);
			Parameters.Param.PoissonDiskBuffer = Gfx.UseBuffer(Gfx.PoissonDiskBuffer);
			Parameters.Param.RandomSamplesBuffer = Gfx.UseBuffer(Gfx.RandomSamplesBuffer);
			Parameters.Param.PrevColorTarget = Gfx.UseTexture(Gfx.GfxColorTarget[(Gfx.BackBufferIndex + 1) % 2]);
			Parameters.Param.GfxDepthTarget = Gfx.UseTexture(Gfx.GfxDepthTarget);
			Parameters.Param.VolumetricLightTexture = Gfx.UseTexture(Gfx.VolumetricLightOut);
			Parameters.Param.IndirectLightTexture = Gfx.UseTexture(Gfx.IndirectLightOut);
			Parameters.Param.RandomAnglesTexture = Gfx.UseTexture(Gfx.RandomAnglesTexture);
			Parameters.Param.GBuffer = Gfx.UseTextureArray(Gfx.GBuffer);
			Parameters.Param.AmbientOcclusionData = Gfx.UseTexture(Gfx.AmbientOcclusionData);
			Parameters.Param.GlobalShadow = Gfx.UseTextureArray(Gfx.GlobalShadow);

			Parameters.Param.HdrOutput = Gfx.UseTexture(Gfx.HdrColorTarget);
			Parameters.Param.BrightOutput = Gfx.UseTexture(Gfx.BrightTarget);

			Parameters.StaticStorage.LightShadows = Gfx.UseTextureArray(LightShadows);
			Parameters.StaticStorage.PointLightShadows = Gfx.UseTextureArray(PointLightShadows);

			Gfx.AddPass<color_pass>("Deferred Color Pass", Parameters, pass_type::compute,
			[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			for(u32 MipIdx = 1; MipIdx < Gfx.BrightTarget->Info.MipLevels; ++MipIdx)
			{
				shader_parameter<bloom_downsample> Parameters;
				Parameters.Param.Input  = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx - 1);
				Parameters.Param.Output = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx);

				vec2 VecDims(Max(1u, Gfx.BrightTarget->Width  >> MipIdx),
							 Max(1u, Gfx.BrightTarget->Height >> MipIdx));

				Gfx.AddPass<bloom_downsample>("Bloom Downsample Mip: #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[this, VecDims](command_list* Cmd, void* Parameters)
				{
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			for(s32 MipIdx = Gfx.BrightTarget->Info.MipLevels - 2; MipIdx >= 0; --MipIdx)
			{
				shader_parameter<bloom_upsample> Parameters;
				Parameters.Param.A      = Gfx.UseTextureMip(Gfx.TempBrTarget, MipIdx + 1);
				Parameters.Param.B      = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx);
				Parameters.Param.Output = Gfx.UseTextureMip(Gfx.TempBrTarget, MipIdx);

				vec2 VecDims(Max(1u, Gfx.BrightTarget->Width  >> MipIdx),
							 Max(1u, Gfx.BrightTarget->Height >> MipIdx));

				Gfx.AddPass<bloom_upsample>("Bloom Upsample Mip: #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[this, VecDims](command_list* Cmd, void* Parameters)
				{
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			{
				shader_parameter<bloom_combine> Parameters;
				Parameters.Param.A       = Gfx.UseTexture(Gfx.HdrColorTarget);
				Parameters.Param.B       = Gfx.UseTexture(Gfx.TempBrTarget);
				Parameters.Param.Output = Gfx.UseTexture(Gfx.GfxColorTarget[Gfx.BackBufferIndex]);

				Gfx.AddPass<bloom_combine>("Bloom Combine", Parameters, pass_type::compute,
				[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](command_list* Cmd, void* Parameters)
				{
					Cmd->BindShaderParameters(Parameters);
					Cmd->Dispatch(Width, Height);
				});
			}
		}

		{
			u32 MipCount = GetImageMipLevels(PreviousPowerOfTwo(Gfx.DepthPyramid->Width), PreviousPowerOfTwo(Gfx.DepthPyramid->Height));
			for(u32 MipIdx = 0; MipIdx < MipCount; ++MipIdx)
			{
				vec2 VecDims(Max(1u, Gfx.DepthPyramid->Width  >> MipIdx),
							 Max(1u, Gfx.DepthPyramid->Height >> MipIdx));

				shader_parameter<texel_reduce_2d> Parameters;
				if(MipIdx == 0)
					Parameters.Param.Input = Gfx.UseTextureMip(Gfx.GfxDepthTarget, MipIdx);
				else
					Parameters.Param.Input = Gfx.UseTextureMip(Gfx.DepthPyramid, MipIdx - 1);
				Parameters.Param.Output = Gfx.UseTextureMip(Gfx.DepthPyramid, MipIdx);

				Gfx.AddPass<texel_reduce_2d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[this, VecDims](command_list* Cmd, void* Parameters)
				{
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}
		}

		{
			shader_parameter<occlusion_culling> Parameters;
			Parameters.Param.MeshCommonCullingInputBuffer = Gfx.UseBuffer(MeshCommonCullingInputBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryOffsets);
			Parameters.Param.MeshDrawCommandDataBuffer = Gfx.UseBuffer(MeshDrawCommandDataBuffer);
			Parameters.Param.DepthPyramid = Gfx.UseTexture(Gfx.DepthPyramid);
			Parameters.Param.MeshDrawVisibilityDataBuffer = Gfx.UseBuffer(MeshDrawVisibilityDataBuffer);

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<occlusion_culling>("Occlusion culling", Parameters, pass_type::compute,
			[this, Input](command_list* Cmd, void* Parameters)
			{
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}
	}
};
