#pragma once

// TODO: Recreate buffers on updates
// TODO: Don't forget about object destruction
struct render_debug_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh::material> Materials;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	buffer* GeometryDebugOffsets;
	buffer* WorldUpdateBuffer;
	buffer* MeshCommonCullingInputBuffer;

	buffer* DebugMeshDrawCommandDataBuffer;
	buffer* DebugMeshDrawVisibilityDataBuffer;

	buffer* DebugIndirectDrawIndexedCommands;

	buffer* MeshDebugMaterialsBuffer;

	buffer* DebugVertexBuffer;
	buffer* DebugIndexBuffer;

	buffer* MeshDrawDebugCommandBuffer;

	system_constructor(render_debug_system)
	{
		RequireComponent<mesh_component>();
		RequireComponent<debug_component>();
		RequireComponent<static_instances_component>();
	}

	void SubscribeToEvents(event_bus& Events)
	{
	}

	void Setup(window& Window, mesh_comp_culling_common_input& MeshCommonCullingInput)
	{
		for(entity& Entity : Entities)
		{
			mesh NewDebugMesh;
			mesh_component* MeshComponent = Entity.GetComponent<mesh_component>();
			static_instances_component* InstancesComponent = Entity.GetComponent<static_instances_component>();

			NewDebugMesh.GenerateNTBDebug(MeshComponent->Data);
			Geometries.Load(NewDebugMesh);

			for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityInstance.MeshIndex - 1) * 3 + 1 });
				StaticMeshVisibility.push_back(true);
			}

			for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityInstance.MeshIndex - 1) * 3 + 2 });
				StaticMeshVisibility.push_back(true);
			}

			for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityInstance.MeshIndex - 1) * 3 + 3 });
				StaticMeshVisibility.push_back(true);
			}

			Materials.push_back({ vec4(vec3(1, 0, 0), 1) });
			Materials.push_back({ vec4(vec3(0, 1, 0), 1) });
			Materials.push_back({ vec4(vec3(0, 0, 1), 1) });
		}

		MeshCommonCullingInput.DebugDrawCount = StaticMeshInstances.size();
		MeshCommonCullingInput.DebugMeshCount = Geometries.MeshCount;

		if (!Geometries.MeshCount) return;

		GeometryDebugOffsets = Window.Gfx.PushBuffer("GeometryDebugOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		DebugMeshDrawCommandDataBuffer = Window.Gfx.PushBuffer("DebugMeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
		DebugMeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer("DebugMeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);

		DebugIndirectDrawIndexedCommands = Window.Gfx.PushBuffer("DebugIndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), true, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);

		MeshDebugMaterialsBuffer = Window.Gfx.PushBuffer("MeshDebugMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		DebugVertexBuffer = Window.Gfx.PushBuffer("DebugVertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		DebugIndexBuffer = Window.Gfx.PushBuffer("DebugIndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), false, resource_flags::RF_IndexBuffer | resource_flags::RF_CopyDst);
		MeshDrawDebugCommandBuffer = Window.Gfx.PushBuffer("MeshDrawDebugCommandBuffer", sizeof(mesh_draw_command), StaticMeshInstances.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.PushBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
	}

	void UpdateResources(window& Window, alloc_vector<light_source>& GlobalLightSources, global_world_data& WorldUpdate)
	{
		Window.Gfx.DebugComputeContext->SetStorageBufferView(MeshCommonCullingInputBuffer);
		Window.Gfx.DebugComputeContext->SetStorageBufferView(GeometryDebugOffsets);
		Window.Gfx.DebugComputeContext->SetStorageBufferView(DebugMeshDrawCommandDataBuffer);
		Window.Gfx.DebugComputeContext->SetStorageBufferView(DebugMeshDrawVisibilityDataBuffer);
		Window.Gfx.DebugComputeContext->SetStorageBufferView(DebugIndirectDrawIndexedCommands);
		Window.Gfx.DebugComputeContext->SetStorageBufferView(MeshDrawDebugCommandBuffer);
		Window.Gfx.DebugComputeContext->StaticUpdate();

		Window.Gfx.DebugContext->SetStorageBufferView(WorldUpdateBuffer);
		Window.Gfx.DebugContext->SetStorageBufferView(DebugVertexBuffer);
		Window.Gfx.DebugContext->SetStorageBufferView(MeshDrawDebugCommandBuffer);
		Window.Gfx.DebugContext->SetStorageBufferView(MeshDebugMaterialsBuffer);
		Window.Gfx.DebugContext->SetStorageBufferView(GeometryDebugOffsets);
		Window.Gfx.DebugContext->StaticUpdate();
	}

	void Render(window& Window, global_pipeline_context* PipelineContext, 
				global_world_data& WorldData, mesh_comp_culling_common_input& MeshCommonCullingInput,
				alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility)
	{
		if (!Geometries.MeshCount) return;

#if 0
		for(entity& Entity : Entities)
		{
			dynamic_instances_component* InstancesComponent = Entity.GetComponent<dynamic_instances_component>();

			for(mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				DynamicMeshInstances.push_back({EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityInstance.MeshIndex-1)*3 + 1});
				DynamicMeshVisibility.push_back(true);
			}

			for(mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				DynamicMeshInstances.push_back({EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityInstance.MeshIndex-1)*3 + 2});
				DynamicMeshVisibility.push_back(true);
			}

			for(mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				DynamicMeshInstances.push_back({EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityInstance.MeshIndex-1)*3 + 3});
				DynamicMeshVisibility.push_back(true);
			}
		}
#endif

		{
			PipelineContext->SetBufferBarriers({
												{WorldUpdateBuffer, 0, AF_TransferWrite},
												{MeshCommonCullingInputBuffer, 0, AF_TransferWrite},
											  }, PSF_TopOfPipe, PSF_Transfer);

			WorldUpdateBuffer->UpdateSize(&WorldData, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer->UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);
		}

		{
			PipelineContext->SetBufferBarrier({GeometryDebugOffsets, 0, AF_ShaderWrite}, PSF_TopOfPipe, PSF_Compute);
			PipelineContext->SetBufferBarrier({MeshDrawDebugCommandBuffer, 0, AF_ShaderWrite}, PSF_TopOfPipe, PSF_Compute);
			PipelineContext->SetBufferBarrier({MeshCommonCullingInputBuffer, AF_TransferWrite, AF_UniformRead}, PSF_Transfer, PSF_Compute);
			PipelineContext->SetBufferBarrier({DebugIndirectDrawIndexedCommands, 0, AF_ShaderWrite}, PSF_TopOfPipe, PSF_Compute);

			Window.Gfx.DebugComputeContext->Begin(PipelineContext);
			Window.Gfx.DebugComputeContext->Execute(StaticMeshInstances.size());
			Window.Gfx.DebugComputeContext->End();
			Window.Gfx.DebugComputeContext->Clear();
		}

		{
			u32 ColorSrcAccessMask = AF_ShaderWrite;
			u32 DepthSrcAccessMask = AF_ShaderRead;

			barrier_state ColorOldLayout = barrier_state::general;
			barrier_state DepthOldLayout = barrier_state::shader_read;

			u32 SrcStageMask = PSF_Compute;
			u32 DstStageMask = PSF_ColorAttachment | PSF_EarlyFragment | PSF_LateFragment;

			PipelineContext->SetImageBarriers({{Window.Gfx.GfxColorTarget, ColorSrcAccessMask, AF_ColorAttachmentWrite, ColorOldLayout, barrier_state::color_attachment, ~0u},
											  {Window.Gfx.GfxDepthTarget, DepthSrcAccessMask, AF_DepthStencilAttachmentWrite, DepthOldLayout, barrier_state::depth_stencil_attachment, ~0u}}, 
											 SrcStageMask, DstStageMask);

			PipelineContext->SetBufferBarrier({WorldUpdateBuffer, AF_TransferWrite, AF_UniformRead}, PSF_Transfer, PSF_VertexShader|PSF_FragmentShader);
			PipelineContext->SetBufferBarrier({GeometryDebugOffsets, AF_ShaderWrite, AF_ShaderRead}, PSF_Compute, PSF_VertexShader);
			PipelineContext->SetBufferBarrier({MeshDrawDebugCommandBuffer, AF_ShaderWrite, AF_ShaderRead}, PSF_Compute, PSF_VertexShader);
			PipelineContext->SetBufferBarrier({DebugIndirectDrawIndexedCommands, AF_ShaderWrite, AF_IndirectCommandRead}, PSF_Compute, PSF_DrawIndirect);

			Window.Gfx.DebugContext->Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);
			Window.Gfx.DebugContext->SetColorTarget(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, {Window.Gfx.GfxColorTarget}, {0, 0, 0, 1});
			Window.Gfx.DebugContext->SetDepthTarget(Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GfxDepthTarget, {1, 0});
			Window.Gfx.DebugContext->DrawIndirect(Geometries.MeshCount, DebugIndexBuffer, DebugIndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));
			Window.Gfx.DebugContext->End();
			Window.Gfx.DebugContext->Clear();
		}
	}
};
