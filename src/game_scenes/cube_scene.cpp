
#include "..\intrinsics.h"
#include "..\mesh.cpp"

#include "game_objects/cube_object.hpp"
#include "game_objects/plane_object.hpp"

struct cube_scene : scene
{
	~cube_scene() override {}

	GameSceneStartFunc() override
	{
		IsInitialized = true;
		Objects.push_back(std::make_unique<cube_object>());
		Objects.push_back(std::make_unique<plane_object>());
		for(std::unique_ptr<object_behavior>& Object : Objects)
		{
			Object->Start();
		}
	}

	GameSceneUpdateFunc() override
	{
		u32 MeshIdx = 1;
		AddPointLight(vec3(-4,  4,  2), 10, vec3(1, 0, 1), 0.2);
		AddPointLight(vec3( 4, -4, -3), 10, vec3(0, 1, 1), 0.2);

		for(std::unique_ptr<object_behavior>& Object : Objects)
		{
			Object->MeshIdx = MeshIdx++;
			Object->Update();
		}
	}
};

extern "C" GameSceneCreateFunc(CubeSceneCreate)
{
	scene* Ptr = new cube_scene();
	return Ptr;
}

