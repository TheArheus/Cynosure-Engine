
#include "..\intrinsics.h"
#include "..\mesh.cpp"

#include "game_objects/cube_object.hpp"

struct cube_scene : scene
{
	~cube_scene() override {}

	GameSceneStartFunc() override
	{
		IsInitialized = true;
		Objects.push_back(std::make_unique<cube_object>());
		for(std::unique_ptr<object_behavior>& Object : Objects)
		{
			Object->Start();
			GlobalMeshes.Load(Object->Mesh);
		}
	}

	GameSceneUpdateFunc() override
	{
		u32 MeshIdx = 1;
		LightSources.push_back({vec4(-4, 4,  2, 10), vec4(), vec4(1, 1, 1, 0.2), light_type_point});
		LightSources.push_back({vec4(4, -4, -3, 10), vec4(), vec4(1, 0, 0, 0.2), light_type_point});

		for(std::unique_ptr<object_behavior>& Object : Objects)
		{
			Object->MeshIdx = MeshIdx;
			Object->Update();

			GlobalInstances.insert(GlobalInstances.end(), Object->Instances.begin(), Object->Instances.end());
		}
	}
};

extern "C" GameSceneCreateFunc(CubeSceneCreate)
{
	scene* Ptr = new cube_scene();
	return Ptr;
}

