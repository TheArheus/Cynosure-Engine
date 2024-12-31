#pragma once
#define USE_CE_GUI 1

#include "intrinsics.h"

#include "core/entity_component_system/entity_systems.h"

#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"

#include "core/gfx/renderer.h"

#include "core/entity_component_system/components.h"

#include "core/platform/window.hpp"

#include "core/ce_gui.h"

#include "game_module.hpp"

enum class game_state
{
    main_menu,
    playing,
    paused
};

class engine
{
	window Window;
	registry Registry;
	asset_store AssetStore;

	void DrawMainMenu();
	void DrawPauseMenu();
	void SetupGameEntities();

	void OnButtonUp(key_up_event& Event);
	void OnButtonDown(key_down_event& Event);
	void OnButtonHold(key_hold_event& Event);

	game_state CurrentGameState = game_state::main_menu;

public:
	engine(const std::vector<std::string>& args) : Window("3D Renderer") { Init(args); };

	void Init(const std::vector<std::string>& args);
	int Run();
};

