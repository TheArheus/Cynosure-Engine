#pragma once

#define USE_CE_GUI 0

// TODO: Make this modules as independent as possible
//       so that there would be only game dependent sources

#include "intrinsics.h"
#include "core/entity_component_system/entity_systems.h"
#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"
#include "core/mesh_loader/mesh.h"
#include "core/gfx/renderer.h"
#include "core/platform/window.hpp"
#include "core/ce_gui.h"
#include "game_module.hpp"


struct texture_data
{
	u32 Width  = 0;
	u32 Height = 0;
	u32 Depth  = 0;
	void* Data = 0;

	texture_data() = default;
	texture_data(const std::string& Path)
	{
		Load(Path);
	}
	texture_data(const char* Path)
	{
		Load(Path);
	}

	texture_data(const texture_data&) = delete;
	texture_data& operator=(const texture_data&) = delete;

	void Load(const char* Path)
	{
		Data = (void*)stbi_load(Path, (int*)&Width, (int*)&Height, (int*)&Depth, 4);
	}
	
	void Load(const std::string& Path)
	{
		Data = (void*)stbi_load(Path.c_str(), (int*)&Width, (int*)&Height, (int*)&Depth, 4);
	}

	void Delete()
	{
		stbi_image_free(Data);
	}
};

// TODO: Material light type: 
// NOTE: In Array of directional lights first one is always being set up by sun light
enum light_type : u32
{
	light_type_none        = 0,
	light_type_directional = 1,
	light_type_point       = 2,
	light_type_spot        = 3,
};

// NOTE: When light source is a point light Pos.w is a radius. Otherwise Pos.w is cutoff angle for spot light.
struct light_source
{
	vec4 Pos;
	vec4 Dir;
	vec4 Col;
	u32  Type;
	vec3 Pad;
};

struct indirect_command_generation_input
{
	u32 DrawCount;
	u32 MeshCount;
};

struct point_shadow_input
{
	vec4  LightPos;
	float FarZ;
	u32   LightIdx;
};

struct mesh_draw_command
{
	vec4 Translate;
	vec4 Scale;
	vec4 Rotate;
	u32  MeshIndex;
	vec3 Pad;
};

// TODO: Simplify and remove as much as possible
struct mesh_comp_culling_common_input
{
	mat4  Proj;
	mat4  View;
	plane CullingPlanes[6];
	u32   FrustrumCullingEnabled;
	u32   OcclusionCullingEnabled;
	float NearZ;
	u32   DrawCount;
	u32   MeshCount;
	u32   DebugDrawCount;
	u32   DebugMeshCount;
};

// TODO: Simplify and remove as much as possible
struct global_world_data 
{
	mat4  View;
	mat4  DebugView;
	mat4  Proj;
	mat4  LightView[DEPTH_CASCADES_COUNT];
	mat4  LightProj[DEPTH_CASCADES_COUNT];
	vec4  CameraPos;
	vec4  CameraDir;
	vec4  GlobalLightPos;
	vec4  SceneScale;
	vec4  SceneCenter;
	float GlobalLightSize;
	u32   PointLightSourceCount;
	u32   SpotLightSourceCount;
	float CascadeSplits[DEPTH_CASCADES_COUNT + 1];
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	u32   DebugColors;
	u32   LightSourceShadowsEnabled;
};

#include "components.h"

class renderer_3d : public game_module
{
	vec3  GlobalLightPos;
	bool  IsCameraLocked = false;
	float GScat = 0.7;

	global_world_data WorldUpdate = {};
	mesh_comp_culling_common_input MeshCompCullingCommonData = {};

public:
	Construct(renderer_3d);

    void ModuleStart() override;
    void ModuleUpdate() override;
};
