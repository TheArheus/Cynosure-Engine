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

	shader_input DebugRootSignature;
	shader_input DebugComputeRootSignature;

	render_context DebugContext;
	compute_context DebugComputeContext;

	system_constructor(render_debug_system)
	{
		RequireComponent<mesh_component>();
		RequireComponent<debug_component>();
		RequireComponent<static_instances_component>();
	}

	void Setup(window& Window, memory_heap& GlobalHeap, mesh_comp_culling_common_input& MeshCommonCullingInput, VkFormat ColorTargetFormat)
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

		GeometryDebugOffsets = GlobalHeap.PushBuffer(Window.Gfx, Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		DebugMeshDrawCommandDataBuffer = GlobalHeap.PushBuffer(Window.Gfx, StaticMeshInstances.data(), sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		DebugMeshDrawVisibilityDataBuffer = GlobalHeap.PushBuffer(Window.Gfx, StaticMeshVisibility.data(), sizeof(u32) * StaticMeshVisibility.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		DebugIndirectDrawIndexedCommands = GlobalHeap.PushBuffer(Window.Gfx, sizeof(indirect_draw_indexed_command) * StaticMeshInstances.size(), true, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		MeshDebugMaterialsBuffer = GlobalHeap.PushBuffer(Window.Gfx, Materials.data(), Materials.size() * sizeof(mesh::material), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		DebugVertexBuffer = GlobalHeap.PushBuffer(Window.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		DebugIndexBuffer = GlobalHeap.PushBuffer(Window.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32), false, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshDrawDebugCommandBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(mesh_draw_command) * StaticMeshInstances.size(), false, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		WorldUpdateBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(global_world_data), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		MeshCommonCullingInputBuffer = GlobalHeap.PushBuffer(Window.Gfx, sizeof(mesh_comp_culling_common_input), false, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		DebugRootSignature.PushUniformBuffer()-> // Global World Update 
						   PushStorageBuffer()-> // Vertex Data
						   PushStorageBuffer()-> // Mesh Draw Commands
						   PushStorageBuffer()-> // Mesh Materials
						   Build(Window.Gfx, 0, true)->
						   BuildAll(Window.Gfx);

		DebugComputeRootSignature.PushUniformBuffer()->		// Mesh Common Culling Input
								  PushStorageBuffer()->		// Mesh Offsets
								  PushStorageBuffer()->		// Draw Command Input
								  PushStorageBuffer()->		// Draw Command Visibility
								  PushStorageBuffer()->		// Indirect Draw Indexed Command
								  PushStorageBuffer()->		// Indirect Draw Indexed Command Counter
								  PushStorageBuffer()->		// Draw Commands
								  Build(Window.Gfx, 0, true)->
								  BuildAll(Window.Gfx);

		render_context::input_data RendererInputData = {};
		RendererInputData.UseColor	  = true;
		RendererInputData.UseDepth	  = true;
		RendererInputData.UseBackFace = true;
		RendererInputData.UseOutline  = true;
		DebugContext = render_context(Window.Gfx, DebugRootSignature, {"..\\build\\shaders\\mesh.dbg.vert.spv", "..\\build\\shaders\\mesh.dbg.frag.spv"}, {ColorTargetFormat}, RendererInputData);
		DebugComputeContext = compute_context(Window.Gfx, DebugComputeRootSignature, "..\\build\\shaders\\mesh.dbg.comp.spv");
	}

	void Render(window& Window, global_pipeline_context& PipelineContext, 
				texture& GfxColorTarget, texture& GfxDepthTarget, 
				global_world_data& WorldData, mesh_comp_culling_common_input& MeshCommonCullingInput,
				alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility)
	{
		{
			std::vector<std::tuple<buffer&, VkAccessFlags, VkAccessFlags>> SetupBufferBarrier = 
			{
				{WorldUpdateBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
				{MeshCommonCullingInputBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT},
			};
			PipelineContext.SetBufferBarriers(SetupBufferBarrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			WorldUpdateBuffer.UpdateSize(Window.Gfx, &WorldData, sizeof(global_world_data), PipelineContext);
			MeshCommonCullingInputBuffer.UpdateSize(Window.Gfx, &MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), PipelineContext);
		}
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
			DebugComputeContext.Begin(PipelineContext);

			DebugComputeContext.SetUniformBufferView(MeshCommonCullingInputBuffer);
			DebugComputeContext.SetStorageBufferView(GeometryDebugOffsets);
			DebugComputeContext.SetStorageBufferView(DebugMeshDrawCommandDataBuffer);
			DebugComputeContext.SetStorageBufferView(DebugMeshDrawVisibilityDataBuffer);
			DebugComputeContext.SetStorageBufferView(DebugIndirectDrawIndexedCommands);
			DebugComputeContext.SetStorageBufferView(MeshDrawDebugCommandBuffer);

			DebugComputeContext.Execute(StaticMeshInstances.size());

			DebugComputeContext.End();
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
				CreateImageBarrier(GfxColorTarget.Handle, ColorSrcAccessMask, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, ColorOldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
				CreateImageBarrier(GfxDepthTarget.Handle, DepthSrcAccessMask, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, DepthOldLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT)
			};
			ImageBarrier(*PipelineContext.CommandList, SrcStageMask, DstStageMask, ImageBeginRenderBarriers);
			PipelineContext.SetBufferBarrier({MeshDebugMaterialsBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
#if 0
			PipelineContext.SetBufferBarriers({{DebugIndexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDEX_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
			PipelineContext.SetBufferBarriers({{DebugVertexBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{MeshDrawDebugCommandBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
			PipelineContext.SetBufferBarriers({{DebugIndirectDrawIndexedCommands, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT}}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
#endif

			DebugContext.SetColorTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx->Width, Window.Gfx->Height, {GfxColorTarget}, {0, 0, 0, 1});
			DebugContext.SetDepthTarget(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, Window.Gfx->Width, Window.Gfx->Height, GfxDepthTarget, {1, 0});
			DebugContext.Begin(Window.Gfx, PipelineContext, Window.Gfx->Width, Window.Gfx->Height);

			DebugContext.SetUniformBufferView(WorldUpdateBuffer);
			DebugContext.SetStorageBufferView(DebugVertexBuffer);
			DebugContext.SetStorageBufferView(MeshDrawDebugCommandBuffer);
			DebugContext.SetStorageBufferView(MeshDebugMaterialsBuffer);
			DebugContext.DrawIndirect<indirect_draw_indexed_command>(Geometries.MeshCount, DebugIndexBuffer, DebugIndirectDrawIndexedCommands);

			DebugContext.End();
		}
	}
};
