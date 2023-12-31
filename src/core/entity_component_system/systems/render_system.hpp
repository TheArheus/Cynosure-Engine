#pragma once

// TODO: Recreate buffers on updates(something like streaming)
// TODO: Don't forget about object destruction
struct render_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	vec3 CubeMapDirections[6];
	vec3 CubeMapUpVectors[6];

	std::vector<mesh::material> Materials;
	std::vector<texture*> DiffuseTextures;
	std::vector<texture*> NormalTextures;
	std::vector<texture*> SpecularTextures;
	std::vector<texture*> HeightTextures;

	std::vector<texture*> LightShadows;
	std::vector<texture*> PointLightShadows;

	texture* SkyBox;

	buffer* GeometryOffsets;
	buffer* WorldUpdateBuffer;
	buffer* MeshCommonCullingInputBuffer;

	buffer* MeshDrawCommandDataBuffer;
	buffer* MeshDrawVisibilityDataBuffer;

	buffer* IndirectDrawIndexedCommands;
	buffer* ShadowIndirectDrawIndexedCommands;

	buffer* MeshMaterialsBuffer;
	buffer* LightSourcesBuffer;

	buffer* VertexBuffer;
	buffer* IndexBuffer;

	buffer* MeshDrawCommandBuffer;
	buffer* MeshDrawShadowCommandBuffer;

	system_constructor(render_system)
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

	void SubscribeToEvents(event_bus& Events)
	{
	}

	void Setup(window& Window, mesh_comp_culling_common_input& MeshCommonCullingInput)
	{
		texture_data Texture = {};
		mesh::material NewMaterial = {};
		utils::texture::input_data TextureInputData = {};
		TextureInputData.Format    = image_format::R8G8B8A8_SRGB;
		TextureInputData.Usage     = image_flags::TF_Sampled | image_flags::TF_CopyDst | image_flags::TF_ColorTexture;
		TextureInputData.Type	   = image_type::Texture2D;
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
				DiffuseTextures.push_back(Window.Gfx.PushTexture(Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasTexture = true;
				NewMaterial.TextureIdx = TextureIdx++;
				Texture.Delete();
			}
			if(NormalMap)
			{
				Texture.Load(NormalMap->Data);
				NormalTextures.push_back(Window.Gfx.PushTexture(Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasNormalMap = true;
				NewMaterial.NormalMapIdx = NormalMapIdx++;
				Texture.Delete();
			}
			if(SpecularMap)
			{
				Texture.Load(SpecularMap->Data);
				SpecularTextures.push_back(Window.Gfx.PushTexture(Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
				NewMaterial.HasSpecularMap = true;
				NewMaterial.SpecularMapIdx = SpecularIdx++;
				Texture.Delete();
			}
			if(HeightMap)
			{
				Texture.Load(HeightMap->Data);
				HeightTextures.push_back(Window.Gfx.PushTexture(Texture.Data, Texture.Width, Texture.Height, 1, TextureInputData));
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
		GeometryOffsets = Window.Gfx.PushBuffer(Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		MeshDrawCommandDataBuffer = Window.Gfx.PushBuffer(StaticMeshInstances.data(), sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
		MeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer(StaticMeshVisibility.data(), sizeof(u32) * StaticMeshVisibility.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);

		IndirectDrawIndexedCommands = Window.Gfx.PushBuffer(sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
		ShadowIndirectDrawIndexedCommands = Window.Gfx.PushBuffer(sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);

		MeshMaterialsBuffer = Window.Gfx.PushBuffer(Materials.data(), Materials.size() * sizeof(mesh::material), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		VertexBuffer = Window.Gfx.PushBuffer(Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		IndexBuffer = Window.Gfx.PushBuffer(Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32), false, resource_flags::RF_IndexBuffer | resource_flags::RF_CopyDst);

		MeshDrawCommandBuffer = Window.Gfx.PushBuffer(sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		MeshDrawShadowCommandBuffer = Window.Gfx.PushBuffer(sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		LightSourcesBuffer = Window.Gfx.PushBuffer(sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT, false, resource_flags::RF_ConstantBuffer | resource_flags::RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.PushBuffer(sizeof(global_world_data), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer(sizeof(mesh_comp_culling_common_input), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
	}

	void UpdateResources(window& Window, alloc_vector<light_source>& GlobalLightSources, global_world_data& WorldUpdate)
	{
		Window.Gfx.ColorPassContext->SetStorageBufferView(WorldUpdateBuffer);
		Window.Gfx.ColorPassContext->SetUniformBufferView(LightSourcesBuffer);
		Window.Gfx.ColorPassContext->SetStorageBufferView(Window.Gfx.PoissonDiskBuffer);
		Window.Gfx.ColorPassContext->SetImageSampler({Window.Gfx.RandomAnglesTexture}, image_barrier_state::shader_read);
		//Window.Gfx.ColorPassContext->SetImageSampler({Window.Gfx.GfxDepthTarget}, image_barrier_state::shader_read);
		Window.Gfx.ColorPassContext->SetImageSampler(Window.Gfx.GBuffer, image_barrier_state::shader_read);
		Window.Gfx.ColorPassContext->SetImageSampler({Window.Gfx.AmbientOcclusionData}, image_barrier_state::shader_read);
		Window.Gfx.ColorPassContext->SetStorageImage({Window.Gfx.GfxColorTarget}, image_barrier_state::general);
		Window.Gfx.ColorPassContext->SetImageSampler(Window.Gfx.GlobalShadow, image_barrier_state::shader_read);

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
						PointLightShadows.push_back(Window.Gfx.PushTexture(nullptr, Window.Width, Window.Width, 1, TextureInputData));
					PointLightSourceCount++;
				}
				else if(LightSource.Type == light_type_spot)
				{
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 1;
					if(WorldUpdate.LightSourceShadowsEnabled)
						LightShadows.push_back(Window.Gfx.PushTexture(nullptr, Window.Width, Window.Height, 1, TextureInputData));
					SpotLightSourceCount++;
				}
			}

			// TODO: Think about abstracting this usage???
			Window.Gfx.ColorPassContext->SetImageSampler(LightShadows, image_barrier_state::shader_read, 0, 1);
			Window.Gfx.ColorPassContext->SetImageSampler(PointLightShadows, image_barrier_state::shader_read, 0, 1);
			Window.Gfx.ColorPassContext->StaticUpdate();
		}

		Window.Gfx.FrustCullingContext->SetStorageBufferView(MeshCommonCullingInputBuffer);
		Window.Gfx.FrustCullingContext->SetStorageBufferView(GeometryOffsets);
		Window.Gfx.FrustCullingContext->SetStorageBufferView(MeshDrawCommandDataBuffer);
		Window.Gfx.FrustCullingContext->SetStorageBufferView(MeshDrawVisibilityDataBuffer);
		Window.Gfx.FrustCullingContext->SetStorageBufferView(IndirectDrawIndexedCommands);
		Window.Gfx.FrustCullingContext->SetStorageBufferView(MeshDrawCommandBuffer);
		Window.Gfx.FrustCullingContext->StaticUpdate();

		Window.Gfx.ShadowComputeContext->SetStorageBufferView(MeshCommonCullingInputBuffer);
		Window.Gfx.ShadowComputeContext->SetStorageBufferView(GeometryOffsets);
		Window.Gfx.ShadowComputeContext->SetStorageBufferView(MeshDrawCommandDataBuffer);
		Window.Gfx.ShadowComputeContext->SetStorageBufferView(MeshDrawVisibilityDataBuffer);
		Window.Gfx.ShadowComputeContext->SetStorageBufferView(ShadowIndirectDrawIndexedCommands);
		Window.Gfx.ShadowComputeContext->SetStorageBufferView(MeshDrawShadowCommandBuffer);
		Window.Gfx.ShadowComputeContext->StaticUpdate();

		Window.Gfx.CascadeShadowContext->SetStorageBufferView(VertexBuffer);
		Window.Gfx.CascadeShadowContext->SetStorageBufferView(MeshDrawShadowCommandBuffer);
		Window.Gfx.CascadeShadowContext->StaticUpdate();

		for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; ++CubeMapFaceIdx)
		{
			Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->SetStorageBufferView(VertexBuffer);
			Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->SetStorageBufferView(MeshDrawShadowCommandBuffer);
			Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->StaticUpdate();
		}

		Window.Gfx.ShadowContext->SetStorageBufferView(VertexBuffer);
		Window.Gfx.ShadowContext->SetStorageBufferView(MeshDrawShadowCommandBuffer);
		Window.Gfx.ShadowContext->StaticUpdate();

		Window.Gfx.DebugCameraViewContext->SetStorageBufferView(VertexBuffer);
		Window.Gfx.DebugCameraViewContext->SetStorageBufferView(MeshDrawCommandBuffer);
		Window.Gfx.DebugCameraViewContext->StaticUpdate();

		// TODO: Update only when needed (The ammount of objects was changed)
		Window.Gfx.GfxContext->SetStorageBufferView(WorldUpdateBuffer);
		Window.Gfx.GfxContext->SetStorageBufferView(VertexBuffer);
		Window.Gfx.GfxContext->SetStorageBufferView(MeshDrawCommandBuffer);
		Window.Gfx.GfxContext->SetStorageBufferView(MeshMaterialsBuffer);
		Window.Gfx.GfxContext->SetImageSampler(DiffuseTextures, image_barrier_state::shader_read, 0, 1);
		Window.Gfx.GfxContext->SetImageSampler(NormalTextures, image_barrier_state::shader_read, 0, 1);
		Window.Gfx.GfxContext->SetImageSampler(SpecularTextures, image_barrier_state::shader_read, 0, 1);
		Window.Gfx.GfxContext->SetImageSampler(HeightTextures, image_barrier_state::shader_read, 0, 1);
		Window.Gfx.GfxContext->StaticUpdate();

		Window.Gfx.AmbientOcclusionContext->SetStorageBufferView(WorldUpdateBuffer);
		Window.Gfx.AmbientOcclusionContext->SetStorageBufferView(Window.Gfx.RandomSamplesBuffer);
		Window.Gfx.AmbientOcclusionContext->SetImageSampler({Window.Gfx.NoiseTexture}, image_barrier_state::shader_read);
		//Window.Gfx.AmbientOcclusionContext->SetImageSampler({Window.Gfx.GfxDepthTarget}, image_barrier_state::shader_read);
		Window.Gfx.AmbientOcclusionContext->SetImageSampler(Window.Gfx.GBuffer, image_barrier_state::shader_read);
		Window.Gfx.AmbientOcclusionContext->SetStorageImage({Window.Gfx.AmbientOcclusionData}, image_barrier_state::general);
		Window.Gfx.AmbientOcclusionContext->StaticUpdate();

		Window.Gfx.BlurContext->SetImageSampler({Window.Gfx.AmbientOcclusionData}, image_barrier_state::general);
		Window.Gfx.BlurContext->SetStorageImage({Window.Gfx.AmbientOcclusionData}, image_barrier_state::general);
		Window.Gfx.BlurContext->StaticUpdate();

		Window.Gfx.OcclCullingContext->SetStorageBufferView(MeshCommonCullingInputBuffer);
		Window.Gfx.OcclCullingContext->SetStorageBufferView(GeometryOffsets);
		Window.Gfx.OcclCullingContext->SetStorageBufferView(MeshDrawCommandDataBuffer);
		Window.Gfx.OcclCullingContext->SetStorageBufferView(MeshDrawVisibilityDataBuffer);
		Window.Gfx.OcclCullingContext->SetImageSampler({Window.Gfx.DepthPyramid}, image_barrier_state::general);
		Window.Gfx.OcclCullingContext->StaticUpdate();
	}

	void Render(window& Window, global_pipeline_context* PipelineContext, 
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
			std::vector<std::tuple<buffer*, u32, u32>> SetupBufferBarrier = 
			{
				{WorldUpdateBuffer, 0, AF_TransferWrite},
				{MeshCommonCullingInputBuffer, 0, AF_TransferWrite},
				{LightSourcesBuffer, 0, AF_TransferWrite},
			};
			PipelineContext->SetBufferBarriers(SetupBufferBarrier, PSF_TopOfPipe, PSF_Transfer);

			WorldUpdateBuffer->UpdateSize(&WorldUpdate, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer->UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);

			LightSourcesBuffer->UpdateSize(GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), PipelineContext);
		}

		{
			PipelineContext->SetBufferBarrier({MeshCommonCullingInputBuffer, AF_TransferWrite, AF_UniformRead}, 
											  PSF_Transfer, PSF_Compute);

			Window.Gfx.FrustCullingContext->Begin(PipelineContext);
			Window.Gfx.FrustCullingContext->Execute(StaticMeshInstances.size());
			Window.Gfx.FrustCullingContext->End();
		}

		{
			Window.Gfx.ShadowComputeContext->Begin(PipelineContext);
			Window.Gfx.ShadowComputeContext->Execute(StaticMeshInstances.size());
			Window.Gfx.ShadowComputeContext->End();
		}

		// TODO: this should be moved to directional light calculations I guess
		//		 or make it possible to turn off
		for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
		{
			mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];

			PipelineContext->SetImageBarriers({{Window.Gfx.GlobalShadow, 0, AF_DepthStencilAttachmentWrite, image_barrier_state::undefined, image_barrier_state::depth_stencil_attachment}}, 
											PSF_Compute, PSF_EarlyFragment);

			PipelineContext->SetBufferBarriers({{MeshDrawShadowCommandBuffer, AF_ShaderWrite, AF_ShaderRead}}, 
											  PSF_Compute, PSF_VertexShader);
			PipelineContext->SetBufferBarriers({{ShadowIndirectDrawIndexedCommands, AF_ShaderWrite, AF_IndirectCommandRead}}, 
											  PSF_Compute, PSF_DrawIndirect);

			Window.Gfx.CascadeShadowContext->SetDepthTarget(load_op::clear, store_op::store, Window.Gfx.GlobalShadow[CascadeIdx]->Width, Window.Gfx.GlobalShadow[CascadeIdx]->Height, Window.Gfx.GlobalShadow[CascadeIdx], {1, 0});

			Window.Gfx.CascadeShadowContext->Begin(PipelineContext, Window.Gfx.GlobalShadow[CascadeIdx]->Width, Window.Gfx.GlobalShadow[CascadeIdx]->Height);
			Window.Gfx.CascadeShadowContext->SetConstant((void*)&Shadow, sizeof(mat4));
			Window.Gfx.CascadeShadowContext->DrawIndirect(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));
			Window.Gfx.CascadeShadowContext->End();
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
					texture* ShadowMapTexture = PointLightShadows[PointLightSourceIdx];

					PipelineContext->SetImageBarriers({{ShadowMapTexture, 0, AF_DepthStencilAttachmentWrite, image_barrier_state::undefined, image_barrier_state::depth_stencil_attachment}}, 
													PSF_Compute, PSF_EarlyFragment);

					for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
					{
						vec3 CubeMapFaceDir = CubeMapDirections[CubeMapFaceIdx];
						vec3 CubeMapUpVect  = CubeMapUpVectors[CubeMapFaceIdx];
						mat4 ShadowMapProj  = PerspRH(90.0f, ShadowMapTexture->Width, ShadowMapTexture->Height, WorldUpdate.NearZ, WorldUpdate.FarZ);
						mat4 ShadowMapView  = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + CubeMapFaceDir, CubeMapUpVect);
						mat4 Shadow = ShadowMapView * ShadowMapProj;
						point_shadow_input PointShadowInput = {};
						PointShadowInput.LightPos = GlobalLightSources[PointLightSourceIdx + SpotLightSourceIdx].Pos;
						PointShadowInput.LightMat = Shadow;
						PointShadowInput.FarZ = WorldUpdate.FarZ;

						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->SetDepthTarget(load_op::clear, store_op::store, ShadowMapTexture->Width, ShadowMapTexture->Height, ShadowMapTexture, {1, 0}, CubeMapFaceIdx, true);

						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->Begin(PipelineContext, ShadowMapTexture->Width, ShadowMapTexture->Height);
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->SetConstant((void*)&PointShadowInput, sizeof(point_shadow_input));
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->DrawIndirect(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->End();
					}
					PointLightSourceIdx++;
				}
				else if(LightSource.Type == light_type_spot)
				{
					texture* ShadowMapTexture = LightShadows[SpotLightSourceIdx];

					PipelineContext->SetImageBarriers({{ShadowMapTexture, 0, AF_DepthStencilAttachmentWrite, image_barrier_state::undefined, image_barrier_state::depth_stencil_attachment}}, 
												  PSF_Compute, PSF_EarlyFragment);

					mat4 ShadowMapProj = PerspRH(LightSource.Pos.w, ShadowMapTexture->Width, ShadowMapTexture->Height, WorldUpdate.NearZ, WorldUpdate.FarZ);
					mat4 ShadowMapView = LookAtRH(vec3(LightSource.Pos), vec3(LightSource.Pos) + vec3(LightSource.Dir), vec3(0, 1, 0));
					mat4 Shadow = ShadowMapView * ShadowMapProj;

					Window.Gfx.ShadowContext->SetDepthTarget(load_op::clear, store_op::store, ShadowMapTexture->Width, ShadowMapTexture->Height, ShadowMapTexture, {1, 0});

					Window.Gfx.ShadowContext->Begin(PipelineContext, ShadowMapTexture->Width, ShadowMapTexture->Height);
					Window.Gfx.ShadowContext->SetConstant((void*)&Shadow, sizeof(mat4));
					Window.Gfx.ShadowContext->DrawIndirect(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));
					Window.Gfx.ShadowContext->End();
					SpotLightSourceIdx++;
				}
			}
		}
		else
		{
			for(u32 CubeMapFaceIdx = 0; CubeMapFaceIdx < 6; CubeMapFaceIdx++)
			{
				Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
				Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx]->End();
			}
			Window.Gfx.ShadowContext->Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.ShadowContext->End();
		}

		// NOTE: This is only for debug. Maybe not compile on release mode???
		{
			mat4 Shadow = WorldUpdate.DebugView * WorldUpdate.Proj;

			PipelineContext->SetImageBarriers({{Window.Gfx.DebugCameraViewDepthTarget, 0, AF_DepthStencilAttachmentWrite, image_barrier_state::undefined, image_barrier_state::depth_stencil_attachment}}, 
										  PSF_Compute, PSF_EarlyFragment);

			PipelineContext->SetBufferBarriers({{MeshDrawCommandBuffer, AF_ShaderWrite, AF_ShaderRead}}, PSF_Compute, PSF_VertexShader);
			PipelineContext->SetBufferBarriers({{IndirectDrawIndexedCommands, AF_ShaderWrite, AF_IndirectCommandRead}}, PSF_Compute, PSF_DrawIndirect);

			Window.Gfx.DebugCameraViewContext->SetDepthTarget(load_op::clear, store_op::store, Window.Gfx.DebugCameraViewDepthTarget->Width, Window.Gfx.DebugCameraViewDepthTarget->Height, Window.Gfx.DebugCameraViewDepthTarget, {1, 0});


			Window.Gfx.DebugCameraViewContext->Begin(PipelineContext, Window.Gfx.DebugCameraViewDepthTarget->Width, Window.Gfx.DebugCameraViewDepthTarget->Height);
			Window.Gfx.DebugCameraViewContext->SetConstant((void*)&Shadow, sizeof(mat4));
			Window.Gfx.DebugCameraViewContext->DrawIndirect(Geometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));
			Window.Gfx.DebugCameraViewContext->End();
		}

		{
			PipelineContext->SetImageBarriers({{DiffuseTextures, 0, AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::shader_read}, 
											  {NormalTextures, 0, AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::shader_read},
											  {SpecularTextures, 0, AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::shader_read},
											  {HeightTextures, 0, AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::shader_read},
											  {Window.Gfx.GBuffer, 0, AF_ColorAttachmentWrite, image_barrier_state::undefined, image_barrier_state::color_attachment},
											  {{Window.Gfx.GfxDepthTarget}, 0, AF_DepthStencilAttachmentWrite, image_barrier_state::undefined, image_barrier_state::depth_stencil_attachment}},
											 PSF_Compute, PSF_FragmentShader | PSF_ColorAttachment | PSF_EarlyFragment);

			PipelineContext->SetBufferBarrier({WorldUpdateBuffer, AF_TransferWrite, AF_UniformRead}, PSF_Transfer, PSF_VertexShader|PSF_FragmentShader);
			PipelineContext->SetBufferBarrier({MeshMaterialsBuffer, AF_TransferWrite, AF_ShaderRead}, PSF_Transfer, PSF_VertexShader|PSF_FragmentShader);

			Window.Gfx.GfxContext->SetColorTarget(load_op::clear, store_op::store, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GBuffer, {0, 0, 0, 0});
			Window.Gfx.GfxContext->SetDepthTarget(load_op::clear, store_op::store, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GfxDepthTarget, {1, 0});

			Window.Gfx.GfxContext->Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.GfxContext->DrawIndirect(Geometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));
			Window.Gfx.GfxContext->End();
		}

		{
			PipelineContext->SetImageBarriers({{Window.Gfx.GBuffer, AF_ColorAttachmentWrite, AF_ShaderRead, image_barrier_state::color_attachment, image_barrier_state::shader_read},
										   {{Window.Gfx.NoiseTexture}, 0, AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::shader_read},
										   {{Window.Gfx.AmbientOcclusionData}, 0, AF_ShaderWrite, image_barrier_state::undefined, image_barrier_state::general},
										   {{Window.Gfx.GfxDepthTarget}, 0, AF_ShaderRead, image_barrier_state::depth_stencil_attachment, image_barrier_state::shader_read}}, 
										  PSF_ColorAttachment, PSF_Compute);

			Window.Gfx.AmbientOcclusionContext->Begin(PipelineContext);
			Window.Gfx.AmbientOcclusionContext->Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.AmbientOcclusionContext->End();
		}

		// NOTE: Horizontal blur
		{

			PipelineContext->SetImageBarriers({{Window.Gfx.AmbientOcclusionData, AF_ShaderWrite, AF_ShaderWrite|AF_ShaderRead, image_barrier_state::general, image_barrier_state::general}}, 
											 PSF_Compute, PSF_Compute);

			vec3 BlurInput(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, 1.0);

			Window.Gfx.BlurContext->Begin(PipelineContext);
			Window.Gfx.BlurContext->SetConstant((void*)BlurInput.E, sizeof(vec3));
			Window.Gfx.BlurContext->Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.BlurContext->End();
		}

		// NOTE: Vertical blur
		{

			PipelineContext->SetImageBarriers({{Window.Gfx.AmbientOcclusionData, AF_ShaderWrite|AF_ShaderRead, AF_ShaderWrite|AF_ShaderRead, image_barrier_state::general, image_barrier_state::general}}, 
											 PSF_Compute, PSF_Compute);

			vec3 BlurInput(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, 0.0);

			Window.Gfx.BlurContext->Begin(PipelineContext);
			Window.Gfx.BlurContext->SetConstant((void*)BlurInput.E, sizeof(vec3));
			Window.Gfx.BlurContext->Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.BlurContext->End();
		}

		{
			std::vector<std::tuple<std::vector<texture*>, u32, u32, image_barrier_state, image_barrier_state>> ColorPassBarrier = 
			{
				{{Window.Gfx.GfxColorTarget}, 0, AF_ShaderWrite, image_barrier_state::undefined, image_barrier_state::general},
				{{Window.Gfx.AmbientOcclusionData}, AF_ShaderWrite|AF_ShaderRead, AF_ShaderRead, image_barrier_state::general, image_barrier_state::shader_read},
				{{Window.Gfx.RandomAnglesTexture}, 0, AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::shader_read},
				{Window.Gfx.GlobalShadow, AF_DepthStencilAttachmentWrite, AF_ShaderRead, image_barrier_state::depth_stencil_attachment, image_barrier_state::shader_read},
				{LightShadows, AF_DepthStencilAttachmentWrite, AF_ShaderRead, image_barrier_state::depth_stencil_attachment, image_barrier_state::shader_read},
				{PointLightShadows, AF_DepthStencilAttachmentWrite, AF_ShaderRead, image_barrier_state::depth_stencil_attachment, image_barrier_state::shader_read},
			};
			PipelineContext->SetImageBarriers(ColorPassBarrier, PSF_EarlyFragment|PSF_Compute, PSF_Compute);

			PipelineContext->SetBufferBarrier({LightSourcesBuffer, AF_TransferWrite, AF_ShaderRead}, PSF_Transfer, PSF_Compute);

			Window.Gfx.ColorPassContext->Begin(PipelineContext);
			Window.Gfx.ColorPassContext->Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.ColorPassContext->End();
		}

		{
			Window.Gfx.DepthReduceContext->Begin(PipelineContext);

			PipelineContext->SetImageBarriers({{Window.Gfx.DebugCameraViewDepthTarget, AF_DepthStencilAttachmentWrite, AF_ShaderRead, image_barrier_state::depth_stencil_attachment, image_barrier_state::shader_read},
											  {Window.Gfx.DepthPyramid, 0, AF_ShaderWrite|AF_ShaderRead, image_barrier_state::undefined, image_barrier_state::general}}, 
											  PSF_LateFragment, PSF_Compute);

#if 0
			for(u32 MipIdx = 0;
				MipIdx < Window.Gfx.DepthPyramid->Info.MipLevels;
				++MipIdx)
			{
				vec2 VecDims(Max(1, Window.Gfx.DepthPyramid->Width  >> MipIdx),
							 Max(1, Window.Gfx.DepthPyramid->Height >> MipIdx));

				Window.Gfx.DepthReduceContext->SetImageSampler({MipIdx == 0 ? Window.Gfx.DebugCameraViewDepthTarget : Window.Gfx.DepthPyramid}, MipIdx == 0 ? image_barrier_state::shader_read: image_barrier_state::general, MipIdx == 0 ? MipIdx : (MipIdx - 1));
				Window.Gfx.DepthReduceContext->SetStorageImage({Window.Gfx.DepthPyramid}, image_barrier_state::general, MipIdx);
				Window.Gfx.DepthReduceContext->SetConstant((void*)VecDims.E, sizeof(vec2));

				Window.Gfx.DepthReduceContext->Execute(VecDims.x, VecDims.y);

				PipelineContext->SetImageBarriers({{Window.Gfx.DepthPyramid, AF_ShaderWrite|AF_ShaderRead, AF_ShaderWrite|AF_ShaderRead, image_barrier_state::general, image_barrier_state::general}}, 
												 PSF_Compute, PSF_Compute);
			}
#endif
			Window.Gfx.DepthReduceContext->End();
		}

		{
			Window.Gfx.OcclCullingContext->Begin(PipelineContext);
			Window.Gfx.OcclCullingContext->Execute(StaticMeshInstances.size());
			Window.Gfx.OcclCullingContext->End();
		}
	}
};
