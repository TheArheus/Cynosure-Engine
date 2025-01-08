#pragma once

#include "intrinsics.h"

#include "core/entity_component_system/entity_systems.h"

#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"

#include "core/gfx/renderer.h"

#include "core/platform/window.hpp"

#include "core/ce_gui.h"


void CreateGuiContext(window* Window)
{
	if(GlobalGuiContext) return;
	GlobalGuiContext = new ce_gui_context();
	GlobalGuiContext->Window = Window;
}

void DestroyGuiContext()
{
	if(GlobalGuiContext) delete GlobalGuiContext;
	GlobalGuiContext = nullptr;
}

#include "game_module.hpp"

class engine
{
	struct scene_info
	{
		library_block Module;
		std::string Name;
		std::string Path;
		std::filesystem::file_time_type LastFileCreation;
		bool IsInitialized;
	};

	window Window;
	registry Registry;
	asset_store AssetStore;

	std::unique_ptr<game_module> Module;

	scene_info ModuleInfo = {};

	void LoadModule();
	void UpdateModule();

public:
	engine(const std::vector<std::string>& args) : Window("3D Renderer") { Init(args); };

	void Init(const std::vector<std::string>& args);
	int Run();
};

