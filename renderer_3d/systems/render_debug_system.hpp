#pragma once

// TODO: Recreate buffers on updates
// TODO: Don't forget about object destruction
struct render_debug_system : public entity_system
{
	mesh Geometries;
	std::vector<mesh::material> Materials;
	std::vector<mesh_draw_command> StaticMeshInstances;
	std::vector<u32> StaticMeshVisibility;

	resource_descriptor GeometryOffsets;
	resource_descriptor WorldUpdateBuffer;
	resource_descriptor MeshCommonCullingInputBuffer;

	resource_descriptor MeshDrawCommandDataBuffer;
	resource_descriptor MeshDrawVisibilityDataBuffer;

	resource_descriptor IndirectDrawIndexedCommands;

	resource_descriptor MeshMaterialsBuffer;

	resource_descriptor VertexBuffer;
	resource_descriptor IndexBuffer;

	resource_descriptor MeshDrawCommandBuffer;

	render_debug_system()
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
		u32 InstanceOffset = 0;
		for(entity& Entity : Entities)
		{
			u32 EntityIdx = *(u32*)&Entity.Handle;
			mesh NewMesh;
			mesh_component* MeshComponent = Entity.GetComponent<mesh_component>();
			static_instances_component* InstancesComponent = Entity.GetComponent<static_instances_component>();

			NewMesh.GenerateNTBDebug(MeshComponent->Data);
			Geometries.Load(NewMesh);

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

		GeometryOffsets = Window.Gfx.GpuMemoryHeap->CreateBuffer("GeometryOffsets", Geometries.Offsets.data(), sizeof(mesh::offset), Geometries.Offsets.size(), RF_StorageBuffer);

		MeshDrawCommandDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawCommandDataBuffer", StaticMeshInstances.data(), sizeof(mesh_draw_command), StaticMeshInstances.size(), RF_IndirectBuffer);
		MeshDrawVisibilityDataBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawVisibilityDataBuffer", StaticMeshVisibility.data(), sizeof(u32), StaticMeshVisibility.size(), RF_IndirectBuffer);

		IndirectDrawIndexedCommands = Window.Gfx.GpuMemoryHeap->CreateBuffer("IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), Geometries.MeshCount, RF_IndirectBuffer | RF_StorageBuffer | RF_WithCounter);

		MeshMaterialsBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshMaterialsBuffer", Materials.data(), sizeof(mesh::material), Materials.size(), RF_StorageBuffer);

		VertexBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("VertexBuffer", Geometries.Vertices.data(), sizeof(vertex), Geometries.Vertices.size(), RF_StorageBuffer);
		IndexBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("IndexBuffer", Geometries.VertexIndices.data(), sizeof(u32), Geometries.VertexIndices.size(), RF_IndexBuffer);
		MeshDrawCommandBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshDrawCommandBuffer", sizeof(mesh_draw_command) * 2, StaticMeshInstances.size(), RF_StorageBuffer);

		WorldUpdateBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("WorldUpdateBuffer", sizeof(global_world_data), 1, RF_StorageBuffer);
		MeshCommonCullingInputBuffer = Window.Gfx.GpuMemoryHeap->CreateBuffer("MeshCommonCullingInputBuffer", sizeof(mesh_comp_culling_common_input), 1, RF_StorageBuffer);
	}

	void Render(global_graphics_context& Gfx, command_list* PipelineContext, 
				global_world_data& WorldUpdate, mesh_comp_culling_common_input& MeshCommonCullingInput,
				alloc_vector<mesh_draw_command>& DynamicMeshInstances, alloc_vector<u32>& DynamicMeshVisibility)
	{
		if (!Geometries.MeshCount) return;

		Gfx.GpuMemoryHeap->UpdateBuffer(WorldUpdateBuffer, (void*)&WorldUpdate, sizeof(global_world_data));
		Gfx.GpuMemoryHeap->UpdateBuffer(MeshCommonCullingInputBuffer, (void*)&MeshCommonCullingInput, sizeof(mesh_comp_culling_common_input));

		{
			generate_all::parameters Parameters;
			Parameters.MeshCommonCullingInputBuffer = MeshCommonCullingInputBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;
			Parameters.MeshDrawCommandDataBuffer = MeshDrawCommandDataBuffer;
			Parameters.MeshDrawVisibilityDataBuffer = MeshDrawVisibilityDataBuffer;
			Parameters.IndirectDrawIndexedCommands = IndirectDrawIndexedCommands;
			Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;

			indirect_command_generation_input Input = {MeshCommonCullingInput.DebugDrawCount, MeshCommonCullingInput.DebugMeshCount};
			Gfx.AddComputePass<generate_all>("Command generation for debug meshes", Parameters,
			[Input, StaticMeshInstancesCount = StaticMeshInstances.size()](command_list* Cmd)
			{
				Cmd->SetConstant((void*)&Input, sizeof(indirect_command_generation_input));
				Cmd->Dispatch(StaticMeshInstancesCount);
			});
		}

		{
			debug_raster::parameters Parameters;
			Parameters.WorldUpdateBuffer = WorldUpdateBuffer;
			Parameters.VertexBuffer = VertexBuffer;
			Parameters.MeshDrawCommandBuffer = MeshDrawCommandBuffer;
			Parameters.MeshMaterialsBuffer = MeshMaterialsBuffer;
			Parameters.GeometryOffsets = GeometryOffsets;

			debug_raster::raster_parameters RasterParameters = {};

			Gfx.AddRasterPass<debug_raster>("Debug raster", Parameters, RasterParameters,
			[MeshCount = Geometries.MeshCount, IndexBuffer = Gfx.GpuMemoryHeap->GetBuffer(IndexBuffer), IndirectDrawIndexedCommands = Gfx.GpuMemoryHeap->GetBuffer(IndirectDrawIndexedCommands), ColorTarget = Gfx.GpuMemoryHeap->GetTexture(Gfx.ColorTarget[Gfx.BackBufferIndex]), DepthTarget = Gfx.GpuMemoryHeap->GetTexture(Gfx.DepthTarget)](command_list* Cmd)
			{
				Cmd->SetViewport(0, 0, ColorTarget->Width, ColorTarget->Height);
				Cmd->SetColorTarget({ColorTarget});
				Cmd->SetDepthTarget(DepthTarget);

				Cmd->SetIndexBuffer(IndexBuffer);
				Cmd->DrawIndirect(IndirectDrawIndexedCommands, MeshCount, sizeof(indirect_draw_indexed_command));
			});
		}
	}
};
