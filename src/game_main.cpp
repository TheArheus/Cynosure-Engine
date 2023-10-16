#include "intrinsics.h"
#include "mesh.cpp"


// NOTE: Initial engine setup
GameSetupFunc(GameSetup)
{
	srand(128);
	MemorySize = MiB(128);
}

GameStartFunc(GameStart)
{
	mesh_object CubeGeometry(Geometries.Load("..\\assets\\cube.obj", generate_aabb | generate_sphere));
	//mesh_object KittenGeometry(Geometries.Load("..\\assets\\kitten.obj", generate_aabb | generate_sphere));
	//mesh_object PlaneGeometry(Geometries.Load("..\\assets\\f22.obj", generate_aabb | generate_sphere));
	//mesh_object BunnyGeometry(Geometries.Load("..\\assets\\stanford-bunny.obj", generate_aabb | generate_sphere));
	
	SceneIsLoaded = true;
}

GameUpdateFunc(GameUpdate)
{
	u32 SceneRadius = 10;
	LightSources.push_back({vec4(-4, 4, 2, 2), vec4(), vec4(1, 1, 0, 1), light_type_point});
	LightSources.push_back({vec4(-4, 4, 3, 2), vec4(), vec4(1, 0, 0, 1), light_type_point});

	mesh_object CubeGeometry(Geometries.Load());
	//mesh_object KittenGeometry(Geometries.Load());
	//mesh_object PlaneGeometry(Geometries.Load());
	//mesh_object BunnyGeometry(Geometries.Load());

	for(u32 DataIdx = 0;
		DataIdx < 512;
		DataIdx++)
	{
		vec4 Translate = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
							  (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
							  (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);

		//if((rand() % Geometries.MeshCount) == 0)
		{
			vec4 Scale = vec4(vec3(1.0f / 2.0), 1.0);
			CubeGeometry.AddInstance({vec4(1, 1, 1, 1), 0, 0, 0, 0}, Translate, Scale, true);
		}
#if 0
		else if((rand() % Geometries.MeshCount) == 1)
		{
			vec4 Scale = vec4(vec3(2.0f), 1.0);
			KittenGeometry.AddInstance({vec4(1, 1, 1, 1), 0, 0, 0, 0}, Translate, Scale, true);
		}
		else if((rand() % Geometries.MeshCount) == 2)
		{
			vec4 Scale = vec4(vec3(1.0f / 3.0f), 1.0);
			PlaneGeometry.AddInstance({vec4(1, 1, 1, 1), 0, 0, 0, 0}, Translate, Scale, true);
		}
		else
		{
			vec4 Scale = vec4(vec3(5.0f), 1.0);
			BunnyGeometry.AddInstance({vec4(1, 1, 1, 1), 0, 0, 0, 0}, Translate, Scale, true);
		}
#endif
	}
	CubeGeometry.UpdateCommands(MeshDrawCommands);
	//KittenGeometry.UpdateCommands(MeshDrawCommands);
	//PlaneGeometry.UpdateCommands(MeshDrawCommands);
	//BunnyGeometry.UpdateCommands(MeshDrawCommands);

}
