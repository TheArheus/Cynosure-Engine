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

	std::vector<mesh::material> Materials;
	std::vector<texture> DiffuseTextures;
	std::vector<texture> NormalTextures;
	std::vector<texture> SpecularTextures;
	std::vector<texture> HeightTextures;

	std::vector<texture> LightShadows;
	std::vector<texture> PointLightShadows;

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

	buffer MeshDrawCommandBuffer;
	buffer MeshDrawShadowCommandBuffer;

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
		GeometryOffsets = Window.Gfx.PushBuffer(Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshDrawCommandDataBuffer = Window.Gfx.PushBuffer(StaticMeshInstances.data(), sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer(StaticMeshVisibility.data(), sizeof(u32) * StaticMeshVisibility.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		IndirectDrawIndexedCommands = Window.Gfx.PushBuffer(sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		ShadowIndirectDrawIndexedCommands = Window.Gfx.PushBuffer(sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshMaterialsBuffer = Window.Gfx.PushBuffer(Materials.data(), Materials.size() * sizeof(mesh::material), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		VertexBuffer = Window.Gfx.PushBuffer(Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		IndexBuffer = Window.Gfx.PushBuffer(Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32), false, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshDrawCommandBuffer = Window.Gfx.PushBuffer(sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshDrawShadowCommandBuffer = Window.Gfx.PushBuffer(sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		LightSourcesBuffer = Window.Gfx.PushBuffer(sizeof(light_source) * LIGHT_SOURCES_MAX_COUNT, false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		WorldUpdateBuffer = Window.Gfx.PushBuffer(sizeof(global_world_data), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer(sizeof(mesh_comp_culling_common_input), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
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
						PointLightShadows.push_back(Window.Gfx.PushTexture(nullptr, Window.Width, Window.Height, 1, TextureInputData));
					PointLightSourceCount++;
				}
				else if(LightSource.Type == light_type_spot)
				{
					TextureInputData.ViewType  = VK_IMAGE_VIEW_TYPE_2D;
					TextureInputData.MipLevels = 1;
					TextureInputData.Layers    = 1;
					if(WorldUpdate.LightSourceShadowsEnabled)
						LightShadows.push_back(Window.Gfx.PushTexture(nullptr, Window.Width, Window.Height, 1, TextureInputData));
					SpotLightSourceCount++;
				}
			}

			Window.Gfx.ColorPassContext.SetImageSampler(LightShadows, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
			Window.Gfx.ColorPassContext.SetImageSampler(PointLightShadows, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
			Window.Gfx.ColorPassContext.StaticUpdate();
		}

		// TODO: Update only when needed (The ammount of objects was changed)
		Window.Gfx.GfxContext.SetImageSampler(DiffuseTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		Window.Gfx.GfxContext.SetImageSampler(NormalTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		Window.Gfx.GfxContext.SetImageSampler(SpecularTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		Window.Gfx.GfxContext.SetImageSampler(HeightTextures, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
		Window.Gfx.GfxContext.StaticUpdate();
	}

	void Render(window& Window, global_pipeline_context& PipelineContext, 
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

			WorldUpdateBuffer.UpdateSize(&WorldUpdate, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer.UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);

			LightSourcesBuffer.UpdateSize(GlobalLightSources.data(), GlobalLightSources.size() * sizeof(light_source), PipelineContext);
		}

		{
			Window.Gfx.FrustCullingContext.Begin(PipelineContext);

			PipelineContext.SetBufferBarrier({MeshCommonCullingInputBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, 
											  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			Window.Gfx.FrustCullingContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			Window.Gfx.FrustCullingContext.SetStorageBufferView(GeometryOffsets);
			Window.Gfx.FrustCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
			Window.Gfx.FrustCullingContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
			Window.Gfx.FrustCullingContext.SetStorageBufferView(IndirectDrawIndexedCommands);
			Window.Gfx.FrustCullingContext.SetStorageBufferView(MeshDrawCommandBuffer);

			Window.Gfx.FrustCullingContext.Execute(StaticMeshInstances.size());

			Window.Gfx.FrustCullingContext.End();
		}

		{
			Window.Gfx.ShadowComputeContext.Begin(PipelineContext);

			Window.Gfx.ShadowComputeContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			Window.Gfx.ShadowComputeContext.SetStorageBufferView(GeometryOffsets);
			Window.Gfx.ShadowComputeContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
			Window.Gfx.ShadowComputeContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
			Window.Gfx.ShadowComputeContext.SetStorageBufferView(ShadowIndirectDrawIndexedCommands);
			Window.Gfx.ShadowComputeContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);

			Window.Gfx.ShadowComputeContext.Execute(StaticMeshInstances.size());

			Window.Gfx.ShadowComputeContext.End();
		}

		// TODO: this should be moved to directional light calculations I guess
		//		 or make it possible to turn off
		for(u32 CascadeIdx = 0; CascadeIdx < DEPTH_CASCADES_COUNT; ++CascadeIdx)
		{
			mat4 Shadow = WorldUpdate.LightView[CascadeIdx] * WorldUpdate.LightProj[CascadeIdx];

			std::vector<VkImageMemoryBarrier> ShadowBarrier = 
			{
				CreateImageBarrier(Window.Gfx.GlobalShadow[CascadeIdx].Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

			PipelineContext.SetBufferBarriers({{MeshDrawShadowCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, 
											  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{ShadowIndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, 
											  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

			Window.Gfx.CascadeShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx.GlobalShadow[CascadeIdx].Width, Window.Gfx.GlobalShadow[CascadeIdx].Height, Window.Gfx.GlobalShadow[CascadeIdx], {1, 0});
			Window.Gfx.CascadeShadowContext.Begin(PipelineContext, Window.Gfx.GlobalShadow[CascadeIdx].Width, Window.Gfx.GlobalShadow[CascadeIdx].Height);

			Window.Gfx.CascadeShadowContext.SetStorageBufferView(VertexBuffer);
			Window.Gfx.CascadeShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
			Window.Gfx.CascadeShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
			Window.Gfx.CascadeShadowContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

			Window.Gfx.CascadeShadowContext.End();
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

						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0}, CubeMapFaceIdx, true);
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].Begin(PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].SetStorageBufferView(VertexBuffer);
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].SetStorageBufferView(MeshDrawShadowCommandBuffer);
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].SetConstant((void*)&PointShadowInput, sizeof(point_shadow_input));
						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

						Window.Gfx.CubeMapShadowContexts[CubeMapFaceIdx].End();
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

					Window.Gfx.ShadowContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, ShadowMapTexture.Width, ShadowMapTexture.Height, ShadowMapTexture, {1, 0});
					Window.Gfx.ShadowContext.Begin(PipelineContext, ShadowMapTexture.Width, ShadowMapTexture.Height);

					Window.Gfx.ShadowContext.SetStorageBufferView(VertexBuffer);
					Window.Gfx.ShadowContext.SetStorageBufferView(MeshDrawShadowCommandBuffer);
					Window.Gfx.ShadowContext.SetConstant((void*)&Shadow, sizeof(mat4));
					Window.Gfx.ShadowContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, ShadowIndirectDrawIndexedCommands);

					Window.Gfx.ShadowContext.End();
					SpotLightSourceIdx++;
				}
			}
		}

		// NOTE: This is only for debug. Maybe not compile on release mode???
		{
			mat4 Shadow = WorldUpdate.DebugView * WorldUpdate.Proj;

			std::vector<VkImageMemoryBarrier> ShadowBarrier = 
			{
				CreateImageBarrier(Window.Gfx.DebugCameraViewDepthTarget.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ShadowBarrier);

			PipelineContext.SetBufferBarriers({{MeshDrawCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{IndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

			Window.Gfx.DebugCameraViewContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx.DebugCameraViewDepthTarget.Width, Window.Gfx.DebugCameraViewDepthTarget.Height, Window.Gfx.DebugCameraViewDepthTarget, {1, 0});
			Window.Gfx.DebugCameraViewContext.Begin(PipelineContext, Window.Gfx.DebugCameraViewDepthTarget.Width, Window.Gfx.DebugCameraViewDepthTarget.Height);

			Window.Gfx.DebugCameraViewContext.SetStorageBufferView(VertexBuffer);
			Window.Gfx.DebugCameraViewContext.SetStorageBufferView(MeshDrawCommandBuffer);
			Window.Gfx.DebugCameraViewContext.SetConstant((void*)&Shadow, sizeof(mat4));
			Window.Gfx.DebugCameraViewContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

			Window.Gfx.DebugCameraViewContext.End();
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
			for(texture& ColorTarget : Window.Gfx.GBuffer)
				ImageBeginRenderBarriers.push_back(CreateImageBarrier(ColorTarget.Handle, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
			ImageBeginRenderBarriers.push_back(CreateImageBarrier(Window.Gfx.GfxDepthTarget.Handle, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ImageBeginRenderBarriers);

			PipelineContext.SetBufferBarrier({WorldUpdateBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			PipelineContext.SetBufferBarrier({MeshMaterialsBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			Window.Gfx.GfxContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GBuffer, {0, 0, 0, 0});
			Window.Gfx.GfxContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GfxDepthTarget, {1, 0});
			Window.Gfx.GfxContext.Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.GfxContext.SetUniformBufferView(WorldUpdateBuffer);
			Window.Gfx.GfxContext.SetStorageBufferView(VertexBuffer);
			Window.Gfx.GfxContext.SetStorageBufferView(MeshDrawCommandBuffer);
			Window.Gfx.GfxContext.SetStorageBufferView(MeshMaterialsBuffer);
			Window.Gfx.GfxContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, IndexBuffer, IndirectDrawIndexedCommands);

			Window.Gfx.GfxContext.End();
		}

		{
			Window.Gfx.AmbientOcclusionContext.Begin(PipelineContext);

			std::vector<VkImageMemoryBarrier> AmbientOcclusionPassBarrier = 
			{
				CreateImageBarrier(Window.Gfx.NoiseTexture.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				CreateImageBarrier(Window.Gfx.AmbientOcclusionData.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
				CreateImageBarrier(Window.Gfx.GfxDepthTarget.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
			};
			for(u32 Idx = 0; Idx < GBUFFER_COUNT; Idx++)
			{
				AmbientOcclusionPassBarrier.push_back(CreateImageBarrier(Window.Gfx.GBuffer[Idx].Handle, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, AmbientOcclusionPassBarrier);

			Window.Gfx.AmbientOcclusionContext.SetUniformBufferView(WorldUpdateBuffer);
			Window.Gfx.AmbientOcclusionContext.SetStorageBufferView(Window.Gfx.RandomSamplesBuffer);
			Window.Gfx.AmbientOcclusionContext.SetImageSampler({Window.Gfx.NoiseTexture}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			//Window.Gfx.AmbientOcclusionContext.SetImageSampler({Window.Gfx.GfxDepthTarget}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Window.Gfx.AmbientOcclusionContext.SetImageSampler(Window.Gfx.GBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Window.Gfx.AmbientOcclusionContext.SetStorageImage({Window.Gfx.AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			Window.Gfx.AmbientOcclusionContext.Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.AmbientOcclusionContext.End();
		}

		// NOTE: Horizontal blur
		{
			Window.Gfx.BlurContext.Begin(PipelineContext);

			std::vector<VkImageMemoryBarrier> BlurPassBarrier = 
			{
				CreateImageBarrier(Window.Gfx.AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, BlurPassBarrier);

			vec3 BlurInput(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, 1.0);
			Window.Gfx.BlurContext.SetImageSampler({Window.Gfx.AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			Window.Gfx.BlurContext.SetStorageImage({Window.Gfx.AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			Window.Gfx.BlurContext.SetConstant((void*)BlurInput.E, sizeof(vec3));
			Window.Gfx.BlurContext.Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.BlurContext.End();
		}

		// NOTE: Vertical blur
		{
			Window.Gfx.BlurContext.Begin(PipelineContext);

			std::vector<VkImageMemoryBarrier> BlurPassBarrier = 
			{
				CreateImageBarrier(Window.Gfx.AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, BlurPassBarrier);

			vec3 BlurInput(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, 0.0);
			Window.Gfx.BlurContext.SetImageSampler({Window.Gfx.AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			Window.Gfx.BlurContext.SetStorageImage({Window.Gfx.AmbientOcclusionData}, VK_IMAGE_LAYOUT_GENERAL);
			Window.Gfx.BlurContext.SetConstant((void*)BlurInput.E, sizeof(vec3));
			Window.Gfx.BlurContext.Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.BlurContext.End();
		}

		{
			Window.Gfx.ColorPassContext.Begin(PipelineContext);

			std::vector<VkImageMemoryBarrier> ColorPassBarrier = 
			{
				CreateImageBarrier(Window.Gfx.GfxColorTarget.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
				CreateImageBarrier(Window.Gfx.AmbientOcclusionData.Handle, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				CreateImageBarrier(Window.Gfx.RandomAnglesTexture.Handle, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
			};
			for(u32 CascadeIdx = 0;
				CascadeIdx < DEPTH_CASCADES_COUNT;
				++CascadeIdx)
			{
				ColorPassBarrier.push_back(CreateImageBarrier(Window.Gfx.GlobalShadow[CascadeIdx].Handle, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT));
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

			Window.Gfx.ColorPassContext.SetUniformBufferView(WorldUpdateBuffer);
			Window.Gfx.ColorPassContext.SetUniformBufferView(LightSourcesBuffer);
			Window.Gfx.ColorPassContext.SetStorageBufferView(Window.Gfx.PoissonDiskBuffer);
			Window.Gfx.ColorPassContext.SetImageSampler({Window.Gfx.RandomAnglesTexture}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			//Window.Gfx.ColorPassContext.SetImageSampler({Window.Gfx.GfxDepthTarget}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Window.Gfx.ColorPassContext.SetImageSampler(Window.Gfx.GBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Window.Gfx.ColorPassContext.SetImageSampler({Window.Gfx.AmbientOcclusionData}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Window.Gfx.ColorPassContext.SetStorageImage({Window.Gfx.GfxColorTarget}, VK_IMAGE_LAYOUT_GENERAL);
			Window.Gfx.ColorPassContext.SetImageSampler(Window.Gfx.GlobalShadow, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			Window.Gfx.ColorPassContext.Execute(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.ColorPassContext.End();
		}

		{
			Window.Gfx.DepthReduceContext.Begin(PipelineContext);

			std::vector<VkImageMemoryBarrier> DepthReadBarriers = 
			{
				CreateImageBarrier(Window.Gfx.DebugCameraViewDepthTarget.Handle, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
				CreateImageBarrier(Window.Gfx.DepthPyramid.Handle, 0, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
			};
			ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DepthReadBarriers);

			for(u32 MipIdx = 0;
				MipIdx < Window.Gfx.DepthPyramid.Info.MipLevels;
				++MipIdx)
			{
				vec2 VecDims(Max(1, Window.Gfx.DepthPyramid.Width  >> MipIdx),
							 Max(1, Window.Gfx.DepthPyramid.Height >> MipIdx));

				Window.Gfx.DepthReduceContext.SetImageSampler({MipIdx == 0 ? Window.Gfx.DebugCameraViewDepthTarget : Window.Gfx.DepthPyramid}, MipIdx == 0 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, MipIdx == 0 ? MipIdx : (MipIdx - 1));
				Window.Gfx.DepthReduceContext.SetStorageImage({Window.Gfx.DepthPyramid}, VK_IMAGE_LAYOUT_GENERAL, MipIdx);
				Window.Gfx.DepthReduceContext.SetConstant((void*)VecDims.E, sizeof(vec2));

				Window.Gfx.DepthReduceContext.Execute(VecDims.x, VecDims.y);

				std::vector<VkImageMemoryBarrier> DepthPyramidBarrier = 
				{
					CreateImageBarrier(Window.Gfx.DepthPyramid.Handle, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL),
				};
				ImageBarrier(*PipelineContext.CommandList, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DepthPyramidBarrier);
			}
			Window.Gfx.DepthReduceContext.End();
		}

		{
			Window.Gfx.OcclCullingContext.Begin(PipelineContext);

			Window.Gfx.OcclCullingContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			Window.Gfx.OcclCullingContext.SetStorageBufferView(GeometryOffsets);
			Window.Gfx.OcclCullingContext.SetStorageBufferView(MeshDrawCommandDataBuffer);
			Window.Gfx.OcclCullingContext.SetStorageBufferView(MeshDrawVisibilityDataBuffer);
			Window.Gfx.OcclCullingContext.SetImageSampler({Window.Gfx.DepthPyramid}, VK_IMAGE_LAYOUT_GENERAL);

			Window.Gfx.OcclCullingContext.Execute(StaticMeshInstances.size());

			Window.Gfx.OcclCullingContext.End();
		}
	}
};
