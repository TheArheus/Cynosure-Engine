#include "renderer_3d_game_module.h"

#include "core/asset_manager/asset_store.cpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "core/mesh_loader/mesh.cpp"

#include "systems.hpp"


// TODO: Sky
// TODO: Image Based Lighting
// TODO: Atmospheric scattering
// TODO: Better shadows and shadow maps
// TODO: Ambient Occlusion fix or a better one
// TODO: Volumetric clouds


void renderer_3d::
ModuleStart()
{
	GlobalLightPos = vec3(-4, 4, 2);

	entity CubeObject = Registry.CreateEntity();
	CubeObject.AddComponent<mesh_component>("../renderer_3d/assets/meshes/cube.obj", generate_aabb | generate_sphere);
	CubeObject.AddComponent<static_instances_component>();
	CubeObject.AddComponent<debug_component>();
	CubeObject.AddComponent<diffuse_component>("../renderer_3d/assets/textures/bricks4/brick-wall.diff.tga");
	CubeObject.AddComponent<normal_map_component>("../renderer_3d/assets/textures/bricks4/brick-wall.norm.tga");
	CubeObject.AddComponent<specular_map_component>("../renderer_3d/assets/textures/bricks4/brick-wall.spec.tga");
	CubeObject.AddComponent<height_map_component>("../renderer_3d/assets/textures/bricks4/brick-wall.disp.png");

	entity PlaneObject = Registry.CreateEntity();
	PlaneObject.AddComponent<mesh_component>("../renderer_3d/assets/meshes/cube.obj", generate_aabb | generate_sphere);
	PlaneObject.AddComponent<static_instances_component>();

	//entity VampireEntity = Registry.CreateEntity();
	//VampireEntity.AddComponent<mesh_component>("../renderer_3d/assets/meshes/dancing vampire/dancing_vampire.dae");

	//entity BobEntity = Registry.CreateEntity();
	//BobEntity.AddComponent<mesh_component>("../renderer_3d/assets/meshes/bob_lamp_update/bob_lamp_update.md5mesh");

	entity CameraObject = Registry.CreateEntity();
	CameraObject.AddComponent<camera_component>(45.0f, 0.01f, 100.0f, false);

	vec4 Scaling = vec4(vec3(1.0f / 2.0), 1.0);

#if 1
	entity LightComponent0 = Registry.CreateEntity();
	entity LightComponent1 = Registry.CreateEntity();
	LightComponent0.AddComponent<light_component>()->PointLight(vec3(-4,  4,  2), 10, vec3(1, 0, 1), 0.2);
	LightComponent1.AddComponent<light_component>()->PointLight(vec3( 4, -4, -3), 10, vec3(0, 1, 1), 0.2);

	u32  SceneRadius = 10;
	for(u32 DataIdx = 0;
		DataIdx < 512;
		DataIdx++)
	{
		vec4 Translation = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								(float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								(float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);

		if(DataIdx & 1)
			CubeObject.GetComponent<static_instances_component>().AddInstance(Translation, Scaling, true);
		else
			PlaneObject.GetComponent<static_instances_component>().AddInstance(Translation, Scaling, true);
	}
#else
	entity LightComponent0 = Registry.CreateEntity();
	LightComponent0.AddComponent<light_component>()->PointLight(vec3(0,  0,  0), 10, vec3(1, 1, 1), 1.0);

	CubeObject.GetComponent<static_instances_component>().AddInstance(vec4(-2.5, 0.0, 0.0, 0.0), Scaling, true);
	PlaneObject.GetComponent<static_instances_component>().AddInstance(vec4(2.5, 0.0, 0.0, 0.0), Scaling, true);
	EmmitObject.GetComponent<static_instances_component>().AddInstance(vec4(0.0, 0.0, 2.5, 0.0), Scaling, true);
#endif

	Registry.AddSystem<light_sources_system>();
	Registry.AddSystem<world_update_system>();
	Registry.AddSystem<deferred_raster_system>();
	//Registry.AddSystem<render_debug_system>();

	Registry.GetSystem<world_update_system>()->SubscribeToEvents(EventsDispatcher);

	Registry.UpdateSystems(0);

	Registry.GetSystem<deferred_raster_system>()->Setup(Window, WorldUpdate, MeshCompCullingCommonData);
	//Registry.GetSystem<render_debug_system>()->Setup(Window, MeshCompCullingCommonData);
}

void renderer_3d::
ModuleUpdate()
{
	alloc_vector<mesh_draw_command> GlobalMeshInstances(16384);
	alloc_vector<u32> GlobalMeshVisibility(16384);
	alloc_vector<mesh_draw_command> DebugMeshInstances(16384);
	alloc_vector<u32> DebugMeshVisibility(16384);
	alloc_vector<light_source> GlobalLightSources(LIGHT_SOURCES_MAX_COUNT);

	Registry.UpdateSystems(dt);
	Registry.GetSystem<world_update_system>()->Update(Window, WorldUpdate, MeshCompCullingCommonData, GlobalLightPos);
	Registry.GetSystem<light_sources_system>()->Update(WorldUpdate, GlobalLightSources);
	Registry.GetSystem<deferred_raster_system>()->Render(Window.Width, Window.Height, Window.Gfx, WorldUpdate, MeshCompCullingCommonData, GlobalLightSources/*, DynamicDebugInstances, DynamicDebugVisibility*/);
	//Registry.GetSystem<render_debug_system>()->Render(Window.Gfx, WorldUpdate, MeshCompCullingCommonData, DynamicDebugInstances, DynamicDebugVisibility);

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(250, 300));
	ImGui::Begin("Renderer settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::Checkbox("Enable light source shadows", (bool*)&WorldUpdate.LightSourceShadowsEnabled);
	ImGui::Checkbox("Enable frustrum culling", (bool*)&MeshCompCullingCommonData.FrustrumCullingEnabled);
	ImGui::Checkbox("Enable occlusion culling", (bool*)&MeshCompCullingCommonData.OcclusionCullingEnabled);
	ImGui::SliderFloat("Global Light Size", &WorldUpdate.GlobalLightSize, 0.0f, 1.0f);

	ImGui::End();
}

extern "C" GameModuleCreateFunc(GameModuleCreate)
{
	Allocator = NewAllocator;
	ImGui::SetCurrentContext(NewWindow.imguiContext.get());
	GlobalGuiContext = NewContext;
	game_module* Ptr = new renderer_3d(NewWindow, NewEventDispatcher, NewRegistry, NewGfx);
	return Ptr;
}
