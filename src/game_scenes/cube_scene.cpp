
#include <intrinsics.h> 
#include <core/mesh_loader/mesh.cpp>
#include <core/entity_component_system/entity_systems.cpp>


// TODO: RequireSystem<system_type>() function so that I could add them later in other parts of the application
struct cube_scene : scene
{
	~cube_scene() override {}

	GameSceneStartFunc() override
	{
		IsInitialized = true;

		GlobalLightPos = vec3(-4, 4, 2);

		entity CubeObject = Registry.CreateEntity();
		CubeObject.AddComponent<mesh_component>("../assets/cube.obj", generate_aabb | generate_sphere);
		CubeObject.AddComponent<static_instances_component>();
		CubeObject.AddComponent<debug_component>();
		CubeObject.AddComponent<diffuse_component>("../assets/bricks4/brick-wall.diff.tga");
		CubeObject.AddComponent<normal_map_component>("../assets/bricks4/brick-wall.norm.tga");
		CubeObject.AddComponent<specular_map_component>("../assets/bricks4/brick-wall.spec.tga");
		CubeObject.AddComponent<height_map_component>("../assets/bricks4/brick-wall.disp.png");

		entity PlaneObject = Registry.CreateEntity();
		PlaneObject.AddComponent<mesh_component>("../assets/cube.obj", generate_aabb | generate_sphere);
		PlaneObject.AddComponent<static_instances_component>();

		entity EmmitObject = Registry.CreateEntity();
		EmmitObject.AddComponent<mesh_component>("../assets/cube.obj", generate_aabb | generate_sphere);
		EmmitObject.AddComponent<static_instances_component>();
		EmmitObject.AddComponent<emmit_component>(vec3(0.5, 0.1, 0.7), 17.0); // Should it become point light source?

		//entity VampireEntity = Registry.CreateEntity();
		//VampireEntity.AddComponent<mesh_component>("../assets/dancing vampire/dancing_vampire.dae");

		//entity BobEntity = Registry.CreateEntity();
		//BobEntity.AddComponent<mesh_component>("../assets/bob_lamp_update/bob_lamp_update.md5mesh");

		entity CameraObject = Registry.CreateEntity();
		CameraObject.AddComponent<camera_component>(45.0f, 0.01f, 100.0f, false);

		vec4 Scaling = vec4(vec3(1.0f / 2.0), 1.0);

#if 0
		entity LightComponent0 = Registry.CreateEntity();
		entity LightComponent1 = Registry.CreateEntity();
		LightComponent0.AddComponent<light_component>()->PointLight(vec3(-4,  4,  2), 10, vec3(1, 0, 1), 0.02);
		LightComponent1.AddComponent<light_component>()->PointLight(vec3( 4, -4, -3), 10, vec3(0, 1, 1), 0.02);

		u32  SceneRadius = 10;
		for(u32 DataIdx = 0;
			DataIdx < 512;
			DataIdx++)
		{
			vec4 Translation = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								    (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								    (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);

			if(DataIdx & 1)
				CubeObject.GetComponent<static_instances_component>()->AddInstance(Translation, Scaling, true);
			else
				PlaneObject.GetComponent<static_instances_component>()->AddInstance(Translation, Scaling, true);
		}
#else
		entity LightComponent0 = Registry.CreateEntity();
		LightComponent0.AddComponent<light_component>()->PointLight(vec3(0,  0,  0), 10, vec3(1, 1, 1), 0.02);

		CubeObject.GetComponent<static_instances_component>()->AddInstance(vec4(-2.5, 0.0, 0.0, 0.0), Scaling, true);
		PlaneObject.GetComponent<static_instances_component>()->AddInstance(vec4(2.5, 0.0, 0.0, 0.0), Scaling, true);
		EmmitObject.GetComponent<static_instances_component>()->AddInstance(vec4(0.0, 0.0, 2.5, 0.0), Scaling, true);
#endif
	}

	GameSceneUpdateFunc() override
	{
	}
};

extern "C" GameSceneCreateFunc(CubeSceneCreate)
{
	scene* Ptr = new cube_scene();
	return Ptr;
}

