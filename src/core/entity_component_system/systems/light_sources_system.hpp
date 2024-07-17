#pragma once

struct light_sources_system : public entity_system
{
	u32 PointLightSourceCount = 0;
	u32 SpotLightSourceCount  = 0;

	std::vector<light_source> StaticLightSources;

	system_constructor(light_sources_system)
	{
		RequireComponent<light_component>();
	}

	void SubscribeToEvents(event_bus& Events)
	{
	}

	void Update(global_world_data& WorldUpdate, alloc_vector<light_source>& GlobalLightSources)
	{
		WorldUpdate.PointLightSourceCount = PointLightSourceCount;
		WorldUpdate.SpotLightSourceCount  = SpotLightSourceCount;

		for(entity& Entity : Entities)
		{
			light_component* Light = Entity.GetComponent<light_component>();

			if(Light->Type == light_type_point)
			{
				WorldUpdate.PointLightSourceCount++;
			}
			if(Light->Type == light_type_spot)
			{
				WorldUpdate.SpotLightSourceCount++;
			}

			GlobalLightSources.push_back({Light->Pos, Light->Dir, Light->Col, Light->Type});
		}

		assert(GlobalLightSources.size() < LIGHT_SOURCES_MAX_COUNT);
	}

	void UpdateResources(window& Window, alloc_vector<light_source>& GlobalLightSources, global_world_data& WorldUpdate, u32 BackBufferIndex)
	{
	}
};
