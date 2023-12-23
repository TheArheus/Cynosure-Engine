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

		GeometryDebugOffsets = Window.Gfx.PushBuffer(Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		DebugMeshDrawCommandDataBuffer = Window.Gfx.PushBuffer(StaticMeshInstances.data(), sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopyDst);
		DebugMeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer(StaticMeshVisibility.data(), sizeof(u32) * StaticMeshVisibility.size(), false, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);

		DebugIndirectDrawIndexedCommands = Window.Gfx.PushBuffer(sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, resource_flags::RF_IndirectBuffer | resource_flags::RF_CopySrc | resource_flags::RF_CopyDst);

		MeshDebugMaterialsBuffer = Window.Gfx.PushBuffer(Materials.data(), Materials.size() * sizeof(mesh::material), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		DebugVertexBuffer = Window.Gfx.PushBuffer(Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		DebugIndexBuffer = Window.Gfx.PushBuffer(Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32), false, resource_flags::RF_IndexBuffer | resource_flags::RF_CopyDst);
		MeshDrawDebugCommandBuffer = Window.Gfx.PushBuffer(sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.PushBuffer(sizeof(global_world_data), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer(sizeof(mesh_comp_culling_common_input), false, resource_flags::RF_StorageBuffer | resource_flags::RF_CopyDst);
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

#if 0
		{
			PipelineContext.SetBufferBarriers({
												{WorldUpdateBuffer, AF_TransferWrite},
												{MeshCommonCullingInputBuffer, AF_TransferWrite},
											  }, PSF_Transfer);

			WorldUpdateBuffer.UpdateSize(Window.Gfx, &WorldData, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer.UpdateSize(Window.Gfx, &MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);
		}
#endif

		{
			Window.Gfx.DebugComputeContext->Begin(PipelineContext);

			PipelineContext->SetBufferBarrier({MeshCommonCullingInputBuffer, AF_TransferWrite, AF_UniformRead}, PSF_Transfer, PSF_Compute);

			Window.Gfx.DebugComputeContext->SetStorageBufferView(MeshCommonCullingInputBuffer);
			Window.Gfx.DebugComputeContext->SetStorageBufferView(GeometryDebugOffsets);
			Window.Gfx.DebugComputeContext->SetStorageBufferView(DebugMeshDrawCommandDataBuffer);
			Window.Gfx.DebugComputeContext->SetStorageBufferView(DebugMeshDrawVisibilityDataBuffer);
			Window.Gfx.DebugComputeContext->SetStorageBufferView(DebugIndirectDrawIndexedCommands);
			Window.Gfx.DebugComputeContext->SetStorageBufferView(MeshDrawDebugCommandBuffer);

			Window.Gfx.DebugComputeContext->Execute(StaticMeshInstances.size());

			Window.Gfx.DebugComputeContext->End();
		}

		{
			u32 ColorSrcAccessMask = AF_ShaderWrite;
			u32 DepthSrcAccessMask = AF_ShaderRead;

			image_barrier_state ColorOldLayout = image_barrier_state::general;
			image_barrier_state DepthOldLayout = image_barrier_state::shader_read;

			u32 SrcStageMask = PSF_Compute;
			u32 DstStageMask = PSF_ColorAttachment | PSF_EarlyFragment | PSF_LateFragment;

			PipelineContext->SetImageBarriers({{Window.Gfx.GfxColorTarget, ColorSrcAccessMask, AF_ColorAttachmentWrite, ColorOldLayout, image_barrier_state::color_attachment},
											  {Window.Gfx.GfxDepthTarget, DepthSrcAccessMask, AF_DepthStencilAttachmentWrite, DepthOldLayout, image_barrier_state::depth_stencil_attachment}}, 
											 SrcStageMask, DstStageMask);

			PipelineContext->SetBufferBarrier({WorldUpdateBuffer, AF_TransferWrite, AF_UniformRead}, PSF_Transfer, PSF_VertexShader|PSF_FragmentShader);
			PipelineContext->SetBufferBarriers({{MeshDrawDebugCommandBuffer, AF_ShaderWrite, AF_ShaderRead}}, PSF_Compute, PSF_VertexShader);
			PipelineContext->SetBufferBarriers({{DebugIndirectDrawIndexedCommands, AF_ShaderWrite, AF_IndirectCommandRead}}, PSF_Compute, PSF_DrawIndirect);

			Window.Gfx.DebugContext->SetColorTarget(load_op::load, store_op::store, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, {Window.Gfx.GfxColorTarget}, {0, 0, 0, 1});
			Window.Gfx.DebugContext->SetDepthTarget(load_op::load, store_op::store, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GfxDepthTarget, {1, 0});
			Window.Gfx.DebugContext->Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.DebugContext->SetStorageBufferView(WorldUpdateBuffer);
			Window.Gfx.DebugContext->SetStorageBufferView(DebugVertexBuffer);
			Window.Gfx.DebugContext->SetStorageBufferView(MeshDrawDebugCommandBuffer);
			Window.Gfx.DebugContext->SetStorageBufferView(MeshDebugMaterialsBuffer);
			Window.Gfx.DebugContext->DrawIndirect(Geometries.MeshCount, DebugIndexBuffer, DebugIndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command));

			Window.Gfx.DebugContext->End();
		}
	}
};
