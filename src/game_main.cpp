#include "intrinsics.h"
#include "mesh.cpp"

class mesh_object
{
public:
	mesh_object(u32 NewMeshIdx) : MeshIndex(NewMeshIdx) {}; 

	void AddInstance(vec4 Translate, vec4 Scale, bool IsVisible)
	{
		mesh_draw_command_input Command = {};
		Command.Translate = Translate;
		Command.Scale = Scale;
		Command.IsVisible = IsVisible;
		Command.MeshIndex = MeshIndex;
		ObjectInstances.push_back(Command);
	}

	void AddInstance(mesh::material Mat, vec4 Translate, vec4 Scale, bool IsVisible)
	{
		mesh_draw_command_input Command = {};
		Command.Mat = Mat;
		Command.Translate = Translate;
		Command.Scale = Scale;
		Command.IsVisible = IsVisible;
		Command.MeshIndex = MeshIndex;
		ObjectInstances.push_back(Command);
	}

	void UpdateCommands(std::vector<mesh_draw_command_input>& DrawCommands)
	{
		DrawCommands.insert(DrawCommands.end(), ObjectInstances.begin(), ObjectInstances.end());
	}

	u32 MeshIndex;

private:
	std::vector<mesh_draw_command_input> ObjectInstances;
};

class game_object : mesh_object
{
public:
	virtual void Setup()  = 0;
	virtual void Update() = 0;

private:
	mesh_object Geometry;
};

class level
{
	std::vector<mesh> Geometries;
	std::vector<game_object> Objects;
};

// NOTE: There could be initial level setup or generation of something
GameSetupFunc(GameSetup)
{
	srand(128);
	u32 SceneRadius = 10;

	mesh_object CubeGeometry(Geometries.Load("..\\assets\\cube.obj", generate_aabb | generate_sphere));
	//mesh_object KittenGeometry(Geometries.Load("..\\assets\\kitten.obj", generate_aabb | generate_sphere));
	//mesh_object PlaneGeometry(Geometries.Load("..\\assets\\f22.obj", generate_aabb | generate_sphere));
	//mesh_object BunnyGeometry(Geometries.Load("..\\assets\\stanford-bunny.obj", generate_aabb | generate_sphere));

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

GameUpdateAndRenderFunc(GameUpdateAndRender)
{
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
