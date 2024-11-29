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

	~render_debug_system()
	{
		delete GeometryDebugOffsets;
		delete WorldUpdateBuffer;
		delete MeshCommonCullingInputBuffer;

		delete DebugMeshDrawCommandDataBuffer;
		delete DebugMeshDrawVisibilityDataBuffer;

		delete DebugIndirectDrawIndexedCommands;

		delete MeshDebugMaterialsBuffer;

		delete DebugVertexBuffer;
		delete DebugIndexBuffer;

		delete MeshDrawDebugCommandBuffer;
	}

	void SubscribeToEvents(event_bus& Events)
	{
	}

	void Setup(window& Window, mesh_comp_culling_common_input& MeshCommonCullingInput)
	{
		u32 InstanceOffset = 0;
		for(entity& Entity : Entities)
		{
			u32 EntityIdx = *(u32*)&Entity.Handle;
			mesh NewDebugMesh;
			mesh_component* MeshComponent = Entity.GetComponent<mesh_component>();
			static_instances_component* InstancesComponent = Entity.GetComponent<static_instances_component>();

			NewDebugMesh.GenerateNTBDebug(MeshComponent->Data);
			Geometries.Load(NewDebugMesh);

			Geometries.Offsets[(EntityIdx - 1) * 3 + 0].InstanceOffset = InstanceOffset;
			InstanceOffset += InstancesComponent->Data.size();
			for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityIdx - 1) * 3 + 1 });
				StaticMeshVisibility.push_back(true);
			}

			Geometries.Offsets[(EntityIdx - 1) * 3 + 1].InstanceOffset = InstanceOffset;
			InstanceOffset += InstancesComponent->Data.size();
			for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityIdx - 1) * 3 + 2 });
				StaticMeshVisibility.push_back(true);
			}

			Geometries.Offsets[(EntityIdx - 1) * 3 + 2].InstanceOffset = InstanceOffset;
			InstanceOffset += InstancesComponent->Data.size();
			for (mesh_draw_command& EntityInstance : InstancesComponent->Data)
			{
				StaticMeshInstances.push_back({ EntityInstance.Translate, EntityInstance.Scale, vec4(0), (EntityIdx - 1) * 3 + 3 });
				StaticMeshVisibility.push_back(true);
			}

			Materials.push_back({ vec4(vec3(1, 0, 0), 1) });
			Materials.push_back({ vec4(vec3(0, 1, 0), 1) });
			Materials.push_back({ vec4(vec3(0, 0, 1), 1) });
		}

		MeshCommonCullingInput.DebugDrawCount = StaticMeshInstances.size();
		MeshCommonCullingInput.DebugMeshCount = Geometries.MeshCount;

		if (!Geometries.MeshCount) return;

		GeometryDebugOffsets = Window.Gfx.PushBuffer("GeometryDebugOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), RF_StorageBuffer | RF_CopyDst);

		DebugMeshDrawCommandDataBuffer = Window.Gfx.PushBuffer("DebugMeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_CopyDst);
		DebugMeshDrawVisibilityDataBuffer = Window.Gfx.PushBuffer("DebugMeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer | RF_CopySrc | RF_CopyDst);

		DebugIndirectDrawIndexedCommands = Window.Gfx.PushBuffer("DebugIndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), StaticMeshInstances.size(), RF_IndirectBuffer | RF_StorageBuffer | RF_CopySrc | RF_CopyDst | RF_WithCounter);

		MeshDebugMaterialsBuffer = Window.Gfx.PushBuffer("MeshDebugMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), RF_StorageBuffer | RF_CopyDst);

		DebugVertexBuffer = Window.Gfx.PushBuffer("DebugVertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), RF_StorageBuffer | RF_CopyDst);
		DebugIndexBuffer = Window.Gfx.PushBuffer("DebugIndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), RF_IndexBuffer | RF_CopyDst);
		MeshDrawDebugCommandBuffer = Window.Gfx.PushBuffer("MeshDrawDebugCommandBuffer", sizeof(mesh_draw_command) * 2, StaticMeshInstances.size(), RF_StorageBuffer | RF_CopyDst);

		WorldUpdateBuffer = Window.Gfx.PushBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, RF_StorageBuffer | RF_CopyDst);
		MeshCommonCullingInputBuffer = Window.Gfx.PushBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, RF_StorageBuffer | RF_CopyDst);
	}

	void Render(global_graphics_context& Gfx, command_list* PipelineContext, 
				global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput,
				alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility)
	{
		if (!Geometries.MeshCount) return;

		{
			Gfx.AddTransferPass("Data upload",
			[this, &WorldUpdate, &MeshCommonCullingInput](command_list* Cmd)
			{
				WorldUpdateBuffer->UpdateSize(&WorldUpdate, sizeof(global_world_data), Cmd);
				MeshCommonCullingInputBuffer->UpdateSize(&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input), Cmd);
			});
		}

		{
			shader_parameter<generate_all> Parameters;
			Parameters.Param.MeshCommonCullingInputBuffer = Gfx.UseBuffer(MeshCommonCullingInputBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryDebugOffsets);
			Parameters.Param.MeshDrawCommandDataBuffer = Gfx.UseBuffer(DebugMeshDrawCommandDataBuffer);
			Parameters.Param.MeshDrawVisibilityDataBuffer = Gfx.UseBuffer(DebugMeshDrawVisibilityDataBuffer);
			Parameters.Param.IndirectDrawIndexedCommands = Gfx.UseBuffer(DebugIndirectDrawIndexedCommands);
			Parameters.Param.MeshDrawCommandBuffer = Gfx.UseBuffer(MeshDrawDebugCommandBuffer);

			indirect_command_generation_input Input = {MeshCommonCullingInput.DebugDrawCount, MeshCommonCullingInput.DebugMeshCount};
			Gfx.AddPass<generate_all>("Command generation for debug meshes", Parameters, pass_type::compute,
			[this, Input](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstances.size());
			});
		}
		{
			shader_parameter<debug_raster> Parameters;
			Parameters.Param.WorldUpdateBuffer = Gfx.UseBuffer(WorldUpdateBuffer);
			Parameters.Param.VertexBuffer = Gfx.UseBuffer(DebugVertexBuffer);
			Parameters.Param.MeshDrawCommandBuffer = Gfx.UseBuffer(MeshDrawDebugCommandBuffer);
			Parameters.Param.MeshMaterialsBuffer = Gfx.UseBuffer(MeshDebugMaterialsBuffer);
			Parameters.Param.GeometryOffsets = Gfx.UseBuffer(GeometryDebugOffsets);

			Gfx.AddPass<debug_raster>("Debug raster", Parameters, pass_type::graphics, 
			[this, BackBufferIndex = Gfx.BackBufferIndex, GfxColorTarget = Gfx.GfxColorTarget, GfxDepthTarget = Gfx.GfxDepthTarget](command_list* Cmd)
			{
				Cmd->SetViewport(0, 0, GfxColorTarget[BackBufferIndex]->Width, GfxColorTarget[BackBufferIndex]->Height);
				Cmd->SetColorTarget({GfxColorTarget[BackBufferIndex]});
				Cmd->SetDepthTarget(GfxDepthTarget);

				Cmd->SetIndexBuffer(DebugIndexBuffer);
				Cmd->DrawIndirect(DebugIndirectDrawIndexedCommands, Geometries.MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}
	}
};
