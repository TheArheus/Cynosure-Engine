#pragma once

#include "intrinsics.h"

#include "core/entity_component_system/entity_systems.h"

#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"

#include "core/gfx/renderer.h"

#include "core/entity_component_system/components.h"

#include "game_module.hpp"

#include "core/platform/window.hpp"

class engine
{
	window Window;
	registry Registry;
	asset_store AssetStore;

	game_module_create NewGameModule;

	void OnButtonDown(key_down_event& Event);

	double MAX_FONT_SIZE = 96; // MAX_FONT_SIZE
	double FontSize = 18;
	double Scale = MAX_FONT_SIZE / FontSize;

	resource_descriptor TestFont[256];

public:
	engine(const std::vector<std::string>& args) : Window("3D Renderer") { Init(args); };

	void Init(const std::vector<std::string>& args);
	int Run();
};

