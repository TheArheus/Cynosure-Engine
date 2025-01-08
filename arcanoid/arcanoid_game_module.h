#pragma once

#define USE_CE_GUI 0

// TODO: Make this modules as independent as possible
//       so that there would be only game dependent sources

#include "intrinsics.h"
#include "core/entity_component_system/entity_systems.h"
#include "core/events/events.hpp"
#include "core/asset_manager/asset_store.h"
#include "core/gfx/renderer.h"
#include "components.h"
#include "core/platform/window.hpp"
#include "core/ce_gui.h"
#include "game_module.hpp"


enum class game_state
{
    main_menu,
    playing,
    paused
};


class arcanoid : public game_module
{
	void DrawMainMenu();
	void DrawPauseMenu();
	void SetupGameEntities();

	void OnButtonUp(key_up_event& Event);
	void OnButtonDown(key_down_event& Event);
	void OnButtonHold(key_hold_event& Event);

	game_state CurrentGameState = game_state::main_menu;

public:
	Construct(arcanoid);

    void ModuleStart() override;
    void ModuleUpdate() override;
};
