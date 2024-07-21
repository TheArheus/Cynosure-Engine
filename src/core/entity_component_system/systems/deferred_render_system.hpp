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

					vec3 Scale = EntityInstance.Scale.xyz;
					vec3 Trans = EntityInstance.Translate.xyz;
					vec3 WorldCenter = MeshCenter * Scale + Trans;
					vec3 WorldMin = MeshMin * Scale + Trans;
					vec3 WorldMax = MeshMax * Scale + Trans;

					if(SceneMin.x > WorldMin.x) SceneMin.x = WorldMin.x;
					if(SceneMin.y > WorldMin.y) SceneMin.y = WorldMin.y;
					if(SceneMin.z > WorldMin.z) SceneMin.z = WorldMin.z;

					if(SceneMax.x < WorldMax.x) SceneMax.x = WorldMax.x;
					if(SceneMax.y < WorldMax.y) SceneMax.y = WorldMax.y;
					if(SceneMax.z < WorldMax.z) SceneMax.z = WorldMax.z;
				}
			}

			Materials.push_back(NewMaterial);
		}

		MeshCommonCullingInput.DrawCount = StaticMeshInstances.size();
		MeshCommonCullingInput.MeshCount = Geometries.MeshCount;

		MeshMaterialsBuffer = Window.Gfx.PushBuffer("MeshMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		LightSourcesBuffer = Window.Gfx.PushBuffer("LightSourcesBuffer", sizeof(light_source), LIGHT_SOURCES_MAX_COUNT, false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		VertexBuffer = Window.Gfx.PushBuffer("VertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		IndexBuffer = Window.Gfx.PushBuffer("IndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), false, resource_flags::RF_IndexBuffer | resource_flags::RF_CopyDst);

		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		GeometryOffsets = Window.Gfx.PushBuffer("GeometryOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		MeshDrawCommandDataBuffer = Window.Gfx.PushBuffer("MeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
		MeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer("MeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);
		IndirectDrawIndexedCommands = Window.Gfx.PushBuffer("IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), true, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);

		MeshDrawCommandBuffer = Window.Gfx.PushBuffer("MeshDrawCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		MeshDrawShadowVisibilityDataBuffer = Window.Gfx.PushBuffer("MeshDrawShadowVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);
		ShadowIndirectDrawIndexedCommands = Window.Gfx.PushBuffer("ShadowIndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), true, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
		MeshDrawShadowCommandBuffer = Window.Gfx.PushBuffer("MeshDrawShadowCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.PushBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
	}

	void Render(global_graphics_context& Gfx, global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput, alloc_vector<light_source>& GlobalLightSources)
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
#if 0
			WorldUpdateBuffer->UpdateSize(Gfx.Backend, &WorldUpdate, sizeof(global_world_data));
			MeshCommonCullingInputBuffer->UpdateSize(Gfx.Backend, &MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input));
			LightSourcesBuffer->UpdateSize(Gfx.Backend, GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source));

#else
			Gfx.AddTransferPass("Data upload",
			[this, &WorldUpdate, &MeshCommonCullingInput, &GlobalLightSources](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				WorldUpdateBuffer->UpdateSize(&WorldUpdate, sizeof(global_world_data), Cmd);
				MeshCommonCullingInputBuffer->UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), Cmd);
				LightSourcesBuffer->UpdateSize(GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), Cmd);
			});
#endif
		}

		{
			shader_parameter<frustum_culling> Parameters;
			Parameters.Input.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;
			Parameters.Input.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.Input.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;
			Parameters.Output.IndirectDrawIndexedCommands = IndirectDrawIndexedCommands;
			Parameters.Output.MeshDrawCommandBuffer = MeshDrawCommandBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<frustum_culling>("Frustum culling/indirect command generation", Parameters, pass_type::compute,
			[this, &Input](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<frustum_culling>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}

		{
			shader_parameter<generate_all> Parameters;
			Parameters.Input.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;
			Parameters.Input.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.Input.MeshDrawVisibilityDataBuffer = MeshDrawShadowVisibilityDataBuffer;
			Parameters.Output.IndirectDrawIndexedCommands = ShadowIndirectDrawIndexedCommands;
			Parameters.Output.MeshDrawCommandBuffer = MeshDrawShadowCommandBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<generate_all>("Frustum culling/indirect command generation for shadow maps", Parameters, pass_type::compute,
			[this, Input](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<generate_all>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}

		{
			shader_parameter<mesh_shadow> Parameters;
			Parameters.Input.VertexBuffer = VertexBuffer;
			Parameters.Input.CommandBuffer = MeshDrawShadowCommandBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;

			for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
			{
				mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];
				texture* CascadeShadowTarget = Gfx.GlobalShadow[CascadeIdx];

				Gfx.AddPass<mesh_shadow>("Cascade shadow map generation: #" + std::to_string(CascadeIdx), Parameters, pass_type::graphics, 
				[this, Shadow, CascadeShadowTarget](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
				{
					Gfx.SetGraphicsContext<mesh_shadow>(Cmd);

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
			Parameters.Input.VertexBuffer = VertexBuffer;
			Parameters.Input.CommandBuffer = MeshDrawCommandBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;

			mat4 Shadow = WorldUpdate.DebugView * WorldUpdate.Proj;
			texture* DebugCameraViewDepthTarget = Gfx.DebugCameraViewDepthTarget;

			Gfx.AddPass<depth_prepass>("Depth prepass", Parameters, pass_type::graphics, 
			[this, Shadow, DebugCameraViewDepthTarget](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetGraphicsContext<depth_prepass>(Cmd);

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
			Parameters.Input.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.Input.VertexBuffer = VertexBuffer;
			Parameters.Input.MeshDrawCommandBuffer = MeshDrawCommandBuffer;
			Parameters.Input.MeshMaterialsBuffer = MeshMaterialsBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;

			Parameters.StaticStorage.DiffuseTextures = Gfx.UseTextureArray(DiffuseTextures);
			Parameters.StaticStorage.NormalTextures = Gfx.UseTextureArray(NormalTextures);
			Parameters.StaticStorage.SpecularTextures = Gfx.UseTextureArray(SpecularTextures);
			Parameters.StaticStorage.HeightTextures = Gfx.UseTextureArray(HeightTextures);

			Gfx.AddPass<gbuffer_raster>("GBuffer generation", Parameters, pass_type::graphics, 
			[this, GBuffer = Gfx.GBuffer, GfxDepthTarget = Gfx.GfxDepthTarget](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetGraphicsContext<gbuffer_raster>(Cmd);

				Cmd->BindShaderParameters(Parameters);

				Cmd->SetViewport(0, 0, GBuffer[0]->Width, GBuffer[0]->Height);
				Cmd->SetColorTarget(GBuffer);
				Cmd->SetDepthTarget(GfxDepthTarget);

				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}

		{
			shader_parameter<voxelization> Parameters;
			Parameters.Input.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.Input.VertexBuffer = VertexBuffer;
			Parameters.Input.MeshDrawCommandBuffer = MeshDrawCommandBuffer;
			Parameters.Input.MeshMaterialsBuffer = MeshMaterialsBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;
			Parameters.Input.LightSources = LightSourcesBuffer;

			Parameters.Output.VoxelGrid = Gfx.UseTexture(Gfx.VoxelGridTarget);

			Parameters.StaticStorage.DiffuseTextures = Gfx.UseTextureArray(DiffuseTextures);
			Parameters.StaticStorage.NormalTextures = Gfx.UseTextureArray(NormalTextures);
			Parameters.StaticStorage.SpecularTextures = Gfx.UseTextureArray(SpecularTextures);
			Parameters.StaticStorage.HeightTextures = Gfx.UseTextureArray(HeightTextures);

			Gfx.AddPass<voxelization>("Voxelization", Parameters, pass_type::graphics, 
			[this, VoxelGridTarget = Gfx.VoxelGridTarget](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetGraphicsContext<voxelization>(Cmd);

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
				ReduceParameters.Input.Input   = Gfx.UseTextureMip(Gfx.VoxelGridTarget, MipIdx - 1);
				ReduceParameters.Output.Output = Gfx.UseTextureMip(Gfx.VoxelGridTarget, MipIdx);

				Gfx.AddPass<texel_reduce_3d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), ReduceParameters, pass_type::compute,
				[this, VecDims](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
				{
					Gfx.SetComputeContext<texel_reduce_3d>(Cmd);
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec3));
					Cmd->Dispatch(VecDims.x, VecDims.y, VecDims.z);
				});
			}
		}

		{
			shader_parameter<ssao> Parameters;
			Parameters.Input.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.Input.RandomSamplesBuffer = Gfx.RandomSamplesBuffer;
			Parameters.Input.NoiseTexture = Gfx.UseTexture(Gfx.NoiseTexture);
			Parameters.Input.DepthTarget = Gfx.UseTexture(Gfx.GfxDepthTarget);
			Parameters.Input.GBuffer = Gfx.UseTextureArray(Gfx.GBuffer);

			Parameters.Output.Output = Gfx.UseTexture(Gfx.AmbientOcclusionData);

			Gfx.AddPass<ssao>("Screen Space Ambient Occlusion", Parameters, pass_type::compute,
			[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<ssao>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			shader_parameter<blur> Parameters;
			Parameters.Input.Input   = Gfx.UseTexture(Gfx.AmbientOcclusionData);
			Parameters.Output.Output = Gfx.UseTexture(Gfx.BlurTemp);

			vec3 BlurInput(Gfx.Backend->Width, Gfx.Backend->Height, 1.0);
			Gfx.AddPass<blur>("Blur Vertical", Parameters, pass_type::compute,
			[this, BlurInput](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<blur>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});

			Parameters.Input.Input   = Gfx.UseTexture(Gfx.BlurTemp);
			Parameters.Output.Output = Gfx.UseTexture(Gfx.AmbientOcclusionData);

			BlurInput.z = 0.0;
			Gfx.AddPass<blur>("Blur Horizontal", Parameters, pass_type::compute,
			[this, BlurInput](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<blur>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&BlurInput, sizeof(vec3));
				Cmd->Dispatch(BlurInput.x, BlurInput.y);
			});
		}

		{
			shader_parameter<color_pass> Parameters;
			Parameters.Input.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.Input.LightSourcesBuffer = LightSourcesBuffer;
			Parameters.Input.PoissonDiskBuffer = Gfx.PoissonDiskBuffer;
			Parameters.Input.RandomSamplesBuffer = Gfx.RandomSamplesBuffer;
			Parameters.Input.GfxColorTarget = Gfx.UseTexture(Gfx.GfxColorTarget[(Gfx.BackBufferIndex + 1) % 2]);
			Parameters.Input.GfxDepthTarget = Gfx.UseTexture(Gfx.GfxDepthTarget);
			Parameters.Input.VoxelGridTarget = Gfx.UseTexture(Gfx.VoxelGridTarget);
			Parameters.Input.RandomAnglesTexture = Gfx.UseTexture(Gfx.RandomAnglesTexture);
			Parameters.Input.GBuffer = Gfx.UseTextureArray(Gfx.GBuffer);
			Parameters.Input.AmbientOcclusionData = Gfx.UseTexture(Gfx.AmbientOcclusionData);
			Parameters.Input.GlobalShadow = Gfx.UseTextureArray(Gfx.GlobalShadow);

			Parameters.Output.HdrOutput = Gfx.UseTexture(Gfx.HdrColorTarget);
			Parameters.Output.BrightOutput = Gfx.UseTexture(Gfx.BrightTarget);

			Parameters.StaticStorage.LightShadows = Gfx.UseTextureArray(LightShadows);
			Parameters.StaticStorage.PointLightShadows = Gfx.UseTextureArray(PointLightShadows);

			Gfx.AddPass<color_pass>("Deferred Color Pass", Parameters, pass_type::compute,
			[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<color_pass>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->Dispatch(Width, Height);
			});
		}

		{
			for(u32 MipIdx = 1; MipIdx < Gfx.BrightTarget->Info.MipLevels; ++MipIdx)
			{
				shader_parameter<bloom_downsample> Parameters;
				Parameters.Input.Input   = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx - 1);
				Parameters.Output.Output = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx);

				vec2 VecDims(Max(1u, Gfx.BrightTarget->Width  >> MipIdx),
							 Max(1u, Gfx.BrightTarget->Height >> MipIdx));

				Gfx.AddPass<bloom_downsample>("Bloom Downsample Mip: #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[this, VecDims](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
				{
					Gfx.SetComputeContext<bloom_downsample>(Cmd);
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			for(s32 MipIdx = Gfx.BrightTarget->Info.MipLevels - 2; MipIdx >= 0; --MipIdx)
			{
				shader_parameter<bloom_upsample> Parameters;
				if(MipIdx < (Gfx.BrightTarget->Info.MipLevels - 2))
					Parameters.Input.A   = Gfx.UseTextureMip(Gfx.TempBrTarget, MipIdx + 1);
				else
					Parameters.Input.A   = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx + 1);
				Parameters.Input.B       = Gfx.UseTextureMip(Gfx.BrightTarget, MipIdx);
				Parameters.Output.Output = Gfx.UseTextureMip(Gfx.TempBrTarget, MipIdx);

				vec2 VecDims(Max(1u, Gfx.BrightTarget->Width  >> MipIdx),
							 Max(1u, Gfx.BrightTarget->Height >> MipIdx));

				Gfx.AddPass<bloom_upsample>("Bloom Upsample Mip: #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[this, VecDims](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
				{
					Gfx.SetComputeContext<bloom_upsample>(Cmd);
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}

			{
				shader_parameter<bloom_combine> Parameters;
				Parameters.Input.A       = Gfx.UseTexture(Gfx.HdrColorTarget);
				Parameters.Input.B       = Gfx.UseTexture(Gfx.TempBrTarget);
				Parameters.Output.Output = Gfx.UseTexture(Gfx.GfxColorTarget[Gfx.BackBufferIndex]);

				Gfx.AddPass<bloom_combine>("Bloom Combine", Parameters, pass_type::compute,
				[this, Width = Gfx.Backend->Width, Height = Gfx.Backend->Height](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
				{
					Gfx.SetComputeContext<bloom_combine>(Cmd);
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
					Parameters.Input.Input = Gfx.UseTextureMip(Gfx.GfxDepthTarget, MipIdx);
				else
					Parameters.Input.Input = Gfx.UseTextureMip(Gfx.DepthPyramid, MipIdx - 1);
				Parameters.Output.Output = Gfx.UseTextureMip(Gfx.DepthPyramid, MipIdx);

				Gfx.AddPass<texel_reduce_2d>("Voxel grid mip generation pass #" + std::to_string(MipIdx), Parameters, pass_type::compute,
				[this, VecDims](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
				{
					Gfx.SetComputeContext<texel_reduce_2d>(Cmd);
					Cmd->BindShaderParameters(Parameters);
					Cmd->SetConstant((void*)&VecDims, sizeof(vec2));
					Cmd->Dispatch(VecDims.x, VecDims.y);
				});
			}
		}

		{
			shader_parameter<occlusion_culling> Parameters;
			Parameters.Input.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.Input.GeometryOffsets = GeometryOffsets;
			Parameters.Input.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.Input.DepthPyramid = Gfx.UseTexture(Gfx.DepthPyramid);
			Parameters.Output.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DrawCount, MeshCommonCullingInput.MeshCount};
			Gfx.AddPass<occlusion_culling>("Occlusion culling", Parameters, pass_type::compute,
			[this, Input](global_graphics_context& Gfx, command_list* Cmd, void* Parameters)
			{
				Gfx.SetComputeContext<occlusion_culling>(Cmd);
				Cmd->BindShaderParameters(Parameters);
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}
	}
};
