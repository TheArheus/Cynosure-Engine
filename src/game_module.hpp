#pragma once

#define ModuleStart()    void Start(registry& Registry)
#define ModuleUpdate()   void Update(float dt)
#define ModuleShutdown() void Shutdown(registry& Registry)
class game_module
{
public:
    virtual ~game_module() = default;
    virtual ModuleStart() = 0;
    virtual ModuleUpdate() = 0;
    virtual ModuleShutdown() = 0;

	bool IsInitialized = false;
};

#define GameModuleCreateFunc(name) game_module* name()
typedef GameModuleCreateFunc(game_module_create);
