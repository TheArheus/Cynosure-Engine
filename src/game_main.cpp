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

	float CameraSpeed = 0.01f;
	if(GameInput.Buttons[EC_R].IsDown)
	{
		ViewData.CameraPos += vec3(0, 4.0f*CameraSpeed, 0);
	}
	if(GameInput.Buttons[EC_F].IsDown)
	{
		ViewData.CameraPos -= vec3(0, 4.0f*CameraSpeed, 0);
	}
	if(GameInput.Buttons[EC_W].IsDown)
	{
		ViewData.CameraPos += (ViewData.ViewDir * CameraSpeed);
	}
	if(GameInput.Buttons[EC_S].IsDown)
	{
		ViewData.CameraPos -= (ViewData.ViewDir * CameraSpeed);
	}
#if 0
    vec3 z = (ViewData.ViewDir - ViewData.CameraPos).Normalize();
    vec3 x = Cross(vec3(0, 1, 0), z).Normalize();
    vec3 y = Cross(z, x);
	vec3 Horizontal = x;
	if(GameInput.Buttons[EC_D].IsDown)
	{
		ViewData.CameraPos -= Horizontal * CameraSpeed;
	}
	if(GameInput.Buttons[EC_A].IsDown)
	{
		ViewData.CameraPos += Horizontal * CameraSpeed;
	}
#endif
	if(GameInput.Buttons[EC_LEFT].IsDown)
	{
		quat ViewDirQuat(ViewData.ViewDir, 0);
		quat RotQuat( CameraSpeed * 2, vec3(0, 1, 0));
		ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
	}
	if(GameInput.Buttons[EC_RIGHT].IsDown)
	{
		quat ViewDirQuat(ViewData.ViewDir, 0);
		quat RotQuat(-CameraSpeed * 2, vec3(0, 1, 0));
		ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
	}
	vec3 U = Cross(vec3(0, 1, 0), ViewData.ViewDir);
	if(GameInput.Buttons[EC_UP].IsDown)
	{
		quat ViewDirQuat(ViewData.ViewDir, 0);
		quat RotQuat( CameraSpeed / 2, U);
		ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
	}
	if(GameInput.Buttons[EC_DOWN].IsDown)
	{
		quat ViewDirQuat(ViewData.ViewDir, 0);
		quat RotQuat(-CameraSpeed / 2, U);
		ViewData.ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
	}
}
