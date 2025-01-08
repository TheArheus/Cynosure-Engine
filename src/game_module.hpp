#pragma once

#define Construct(name)  name(window& NewWindow, event_bus& NewEventDispatcher, registry& NewRegistry, global_graphics_context& NewGfx) : game_module(NewWindow, NewEventDispatcher, NewRegistry, NewGfx) { }

#define ModuleStart()    Start()
#define ModuleUpdate()   Update(float dt)
#define ModuleShutdown() Shutdown()

class game_module
{
protected:
	window&    Window;
	event_bus& EventsDispatcher;
	registry&  Registry;
	global_graphics_context& Gfx;

public:
	game_module(window& NewWindow, event_bus& NewEventDispatcher, registry& NewRegistry, global_graphics_context& NewGfx) : Window(NewWindow), EventsDispatcher(NewEventDispatcher), Registry(NewRegistry), Gfx(NewGfx), IsInitialized(true) {}

    virtual ~game_module() = default;
    virtual void ModuleStart() = 0;
    virtual void ModuleUpdate() = 0;
    //virtual ModuleShutdown() = 0;

	bool IsInitialized = false;
};

#define GameModuleCreateFunc(name) game_module* name(window& NewWindow, event_bus& NewEventDispatcher, registry& NewRegistry, global_graphics_context& NewGfx, ce_gui_context* NewContext)
typedef GameModuleCreateFunc(game_module_create);
