#pragma once

// TODO: Recreate buffers on updates
// TODO: Don't forget about object destruction
struct render_debug_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh::material> Materials;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	buffer GeometryDebugOffsets;
	buffer WorldUpdateBuffer;
	buffer MeshCommonCullingInputBuffer;

	buffer DebugMeshDrawCommandDataBuffer;
	buffer DebugMeshDrawVisibilityDataBuffer;

	buffer DebugIndirectDrawIndexedCommands;

	buffer MeshDebugMaterialsBuffer;

	buffer DebugVertexBuffer;
	buffer DebugIndexBuffer;

	buffer MeshDrawDebugCommandBuffer;

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

		GeometryDebugOffsets = Window.Gfx.PushBuffer(Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		DebugMeshDrawCommandDataBuffer = Window.Gfx.PushBuffer(StaticMeshInstances.data(), sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		DebugMeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer(StaticMeshVisibility.data(), sizeof(u32) * StaticMeshVisibility.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		DebugIndirectDrawIndexedCommands = Window.Gfx.PushBuffer(sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshDebugMaterialsBuffer = Window.Gfx.PushBuffer(Materials.data(), Materials.size() * sizeof(mesh::material), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		DebugVertexBuffer = Window.Gfx.PushBuffer(Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		DebugIndexBuffer = Window.Gfx.PushBuffer(Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32), false, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshDrawDebugCommandBuffer = Window.Gfx.PushBuffer(sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		WorldUpdateBuffer = Window.Gfx.PushBuffer(sizeof(global_world_data), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer(sizeof(mesh_comp_culling_common_input), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	void Render(window& Window, global_pipeline_context& PipelineContext, 
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
												{WorldUpdateBuffer, VK_ACCESS_TRANSFER_WRITE_BIT},
												{MeshCommonCullingInputBuffer, VK_ACCESS_TRANSFER_WRITE_BIT},
											  }, VK_PIPELINE_STAGE_TRANSFER_BIT);

			WorldUpdateBuffer.UpdateSize(Window.Gfx, &WorldData, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer.UpdateSize(Window.Gfx, &MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);
		}
#endif

		{
			Window.Gfx.DebugComputeContext.Begin(PipelineContext);

			PipelineContext.SetBufferBarrier({MeshCommonCullingInputBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			Window.Gfx.DebugComputeContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			Window.Gfx.DebugComputeContext.SetStorageBufferView(GeometryDebugOffsets);
			Window.Gfx.DebugComputeContext.SetStorageBufferView(DebugMeshDrawCommandDataBuffer);
			Window.Gfx.DebugComputeContext.SetStorageBufferView(DebugMeshDrawVisibilityDataBuffer);
			Window.Gfx.DebugComputeContext.SetStorageBufferView(DebugIndirectDrawIndexedCommands);
			Window.Gfx.DebugComputeContext.SetStorageBufferView(MeshDrawDebugCommandBuffer);

			Window.Gfx.DebugComputeContext.Execute(StaticMeshInstances.size());

			Window.Gfx.DebugComputeContext.End();
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
				CreateImageBarrier(Window.Gfx.GfxColorTarget.Handle, ColorSrcAccessMask, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, ColorOldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
				CreateImageBarrier(Window.Gfx.GfxDepthTarget.Handle, DepthSrcAccessMask, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, DepthOldLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT)
			};
			ImageBarrier(*PipelineContext.CommandList, SrcStageMask, DstStageMask, ImageBeginRenderBarriers);

			PipelineContext.SetBufferBarrier({WorldUpdateBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{MeshDrawDebugCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{DebugIndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

			Window.Gfx.DebugContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, {Window.Gfx.GfxColorTarget}, {0, 0, 0, 1});
			Window.Gfx.DebugContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height, Window.Gfx.GfxDepthTarget, {1, 0});
			Window.Gfx.DebugContext.Begin(PipelineContext, Window.Gfx.Backend->Width, Window.Gfx.Backend->Height);

			Window.Gfx.DebugContext.SetUniformBufferView(WorldUpdateBuffer);
			Window.Gfx.DebugContext.SetStorageBufferView(DebugVertexBuffer);
			Window.Gfx.DebugContext.SetStorageBufferView(MeshDrawDebugCommandBuffer);
			Window.Gfx.DebugContext.SetStorageBufferView(MeshDebugMaterialsBuffer);
			Window.Gfx.DebugContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, DebugIndexBuffer, DebugIndirectDrawIndexedCommands);

			Window.Gfx.DebugContext.End();
		}
	}
};
