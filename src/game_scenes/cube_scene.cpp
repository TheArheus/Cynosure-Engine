
#include "..\intrinsics.h"
#include "..\mesh.cpp"

struct cube_scene : scene
{
	~cube_scene() override {}

	GameSceneStartFunc() override
	{
		IsInitialized = true;

		entity CubeObject = Registry.CreateEntity();
		Registry.AddComponent<mesh_component>(CubeObject, "..\\assets\\cube.obj", generate_aabb | generate_sphere);
		Registry.AddComponent<static_instances_component>(CubeObject);

		vec4 Scaling = vec4(vec3(1.0f / 2.0), 1.0);
		u32  SceneRadius = 10;
		for(u32 DataIdx = 0;
			DataIdx < 512;
			DataIdx++)
		{
			vec4 Translation = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								    (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
								    (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);

			Registry.GetComponent<static_instances_component>(CubeObject)->AddInstance(CubeObject, {vec4(1, 1, 1, 1), 0, 0, 0, 0}, Translation, Scaling, true);
		}
	}

	GameSceneUpdateFunc() override
	{
		AddPointLight(vec3(-4,  4,  2), 10, vec3(1, 0, 1), 0.2);
		AddPointLight(vec3( 4, -4, -3), 10, vec3(0, 1, 1), 0.2);
	}
};

extern "C" GameSceneCreateFunc(CubeSceneCreate)
{
	scene* Ptr = new cube_scene();
	return Ptr;
}

