#pragma once


struct child_entities_component
{
	std::vector<entity> Data;

	child_entities_component() = default;

	void AddChild(entity& Entity)
	{
		Data.push_back(Entity);
	}
};

struct transform_component
{
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;

	transform_component(vec4 NewTranslate = vec4(0), vec4 NewScale = vec4(0), vec4 NewRotate = vec4(0)) : 
		Translate(NewTranslate), Scale(NewScale), Rotate(NewRotate)
	{}
};

struct color_component
{
	vec3 Data;

	color_component(vec3 NewColor = vec3(1, 1, 1)) : Data(NewColor) {}
};

struct camera_component
{
	union projection
	{
		struct
		{
			float FOV;
			float NearZ;
			float FarZ;
			float Pad[3];
		};
		struct
		{
			float Left;
			float Right;
			float Top;
			float Bottom;
			float Near;
			float Far;
		};
	};

	projection ProjectionData;
	u32   ProjectionType;
	bool  IsLocked;

	camera_component(float NewFOV = 45.0f, float NewNear = 0.01f, float NewFar = 100.0f, bool _IsLocked = false) : 
		IsLocked(_IsLocked)
	{
		ProjectionData.FOV   = NewFOV;
		ProjectionData.NearZ = NewNear;
		ProjectionData.FarZ  = NewFar;
	}
};

struct mesh_component
{
	mesh Data;

	mesh_component() = default;

	mesh_component(const std::string& Path, u32 BoundingGeneration = 0)
	{
		Data.Load(Path, BoundingGeneration);
	}
	
	void LoadMesh(const std::string& Path, u32 BoundingGeneration = 0)
	{
		Data.Load(Path, BoundingGeneration);
	}
};

struct debug_component
{
	debug_component() = default;
};

// TODO: Instance ID's
struct static_instances_component
{
	std::vector<u32> Visibility;
	std::vector<mesh_draw_command> Data;

	void AddInstance(vec4 Translate, vec4 Scale, bool IsVisible)
	{
		mesh_draw_command Command = {};
		Command.Translate = Translate;
		Command.Scale = Scale;
		Data.push_back(Command);
		Visibility.push_back(IsVisible);
	}
};

// NOTE: this should reset every frame
struct dynamic_instances_component
{
	std::vector<u32> Visibility;
	std::vector<mesh_draw_command> Data;

	// TODO: Have only this one
	void AddInstance(vec4 Translate, vec4 Scale, bool IsVisible)
	{
		mesh_draw_command Command = {};
		Command.Translate = Translate;
		Command.Scale = Scale;
		Data.push_back(Command);
		Visibility.push_back(IsVisible);
	}

	void Reset()
	{
		Data.clear();
	}
};

struct alignas(16) light_component
{
	vec4 Pos;
	vec4 Dir;
	vec4 Col;
	u32  Type;

	void PointLight(vec3 Position, float Radius, vec3 Color, float Intensity)
	{
		Pos  = vec4(Position, Radius);
		Col  = vec4(Color, Intensity);
		Type = light_type_point;
	}

	void SpotLight(vec3 Position, float Angle, vec3 Direction, vec3 Color, float Intensity)
	{
		Pos  = vec4(Position, Angle);
		Dir  = vec4(Direction);
		Col  = vec4(Color, Intensity);
		Type = light_type_spot;
	}

	void DirectionalLight(vec3 Position, vec3 Direction, vec3 Color, float Intensity)
	{
		Pos  = vec4(Position);
		Dir  = vec4(Direction);
		Col  = vec4(Color, Intensity);
		Type = light_type_directional;
	}
};

struct particle_component
{
	vec3  Velocity;
	vec3  Acceleration;
	float Time;
};

struct diffuse_component
{
	const char* Data;

	diffuse_component() = default;
	diffuse_component(const char* Path)
		: Data(Path)
	{}
};

struct normal_map_component
{
	const char* Data;

	normal_map_component() = default;
	normal_map_component(const char* Path)
		: Data(Path)
	{}
};

struct specular_map_component
{
	const char* Data;

	specular_map_component() = default;
	specular_map_component(const char* Path)
		: Data(Path)
	{}
};

struct height_map_component
{
	const char* Data;

	height_map_component() = default;
	height_map_component(const char* Path)
		: Data(Path)
	{}
};
