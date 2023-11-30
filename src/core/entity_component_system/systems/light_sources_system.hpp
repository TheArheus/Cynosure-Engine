#pragma once

struct light_sources_system : public entity_system
{
	system_constructor(light_sources_system)
	{
		RequireComponent<light_component>();
	}

	void Update(global_world_data& WorldUpdate, alloc_vector<light_source>& GlobalLightSources)
	{
		//WorldUpdate.LightSourceShadowsEnabled = false;
		WorldUpdate.DirectionalLightSourceCount = 0;
		WorldUpdate.PointLightSourceCount = 0;
		WorldUpdate.SpotLightSourceCount = 0;

		for(entity& Entity : Entities)
		{
			light_component* Light = Entity.GetComponent<light_component>();

			if(Light->Type == light_type_directional)
			{
				WorldUpdate.DirectionalLightSourceCount++;
			}
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
};
