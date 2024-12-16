#include "engine.h"

#include "core/asset_manager/asset_store.cpp"
#include "core/entity_component_system/entity_systems.cpp"
#include "core/entity_component_system/systems.hpp"

#include "engine.cpp"

[[nodiscard]] int engine_main([[maybe_unused]] const std::vector<std::string>& args)
{
	engine Engine(args);
	return Engine.Run();
}
