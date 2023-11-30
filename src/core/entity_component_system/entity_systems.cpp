
size_t base_component::NextID = 0;

template<typename component_type, typename... args>
component_type* entity::
AddComponent(args&&... Args)
{
	return Registry->AddComponent<component_type>(*this, std::forward<args>(Args)...);
}

template<typename component_type>
void entity::
RemoveComponent()
{
	Registry->RemoveComponent<component_type>(*this);
}

template<typename component_type>
bool entity::
HasComponent()
{
	return Registry->HasComponent<component_type>(*this);
}

template<typename component_type>
component_type* entity::GetComponent()
{
	return Registry->GetComponent<component_type>(*this);
}

void entity_system::
AddEntity(entity& Entity)
{
	Entities.push_back(Entity);
}
void entity_system::
RemoveEntity(entity& Entity)
{
	Entities.erase(std::remove_if(Entities.begin(), Entities.end(), [&Entity](entity& Other){return Entity == Other;}), Entities.end());
}
template<typename component_type>
void entity_system::
RequireComponent()
{
	size_t ComponentID = 0;
	auto FindIt = TypeMap.find(std::type_index(typeid(component_type)));

	if(FindIt != TypeMap.end())
		ComponentID = FindIt->second;
	else
	{
		ComponentID = component<component_type>::GetNextID();
		TypeMap[std::type_index(typeid(component_type))] = ComponentID;
	}

	Signature.set(ComponentID);
}

entity registry::
CreateEntity()
{
	EntitiesCount++;
	entity NewEntity(EntitiesCount, this);
	Entities.push_back(NewEntity);

	if((NewEntity.Handle - 1) >= EntitiesComponentSignatures.size())
	{
		EntitiesComponentSignatures.resize(Entities.size());
	}

	return NewEntity;
}

void registry::
AddEntity(entity NewEntity)
{
	EntitiesCount++;
	Entities.push_back(NewEntity);

	if(NewEntity.Handle >= EntitiesComponentSignatures.size())
	{
		EntitiesComponentSignatures.resize(Entities.size() + 1);
	}
}

void registry::
UpdateSystems()
{
	for(entity& Entity : Entities)
	{
		const signature& EntitySignature = EntitiesComponentSignatures[Entity.Handle - 1];
		for(auto& System : Systems)
		{
			const signature& SystemSignature = System.second->Signature;
			if((EntitySignature & SystemSignature) == SystemSignature)
			{
				System.second->AddEntity(Entity);
			}
		}
	}
}

template<typename component_type, typename ...args>
component_type* registry::
AddComponent(entity& Object, args&&... Args)
{
	const size_t EntityID = Object.Handle - 1;
	size_t ComponentID = 0;

	auto FindIt = TypeMap.find(std::type_index(typeid(component_type)));

	if(FindIt != TypeMap.end())
		ComponentID = FindIt->second;
	else
	{
		ComponentID = component<component_type>::GetNextID();
		TypeMap[std::type_index(typeid(component_type))] = ComponentID;
	}

	if(ComponentID >= ComponentPools.size())
	{
		ComponentPools.resize(ComponentID + 1, nullptr);
	}
	if(ComponentPools[ComponentID] == nullptr)
	{
		std::shared_ptr<component_pool<component_type>> NewPool(new component_pool<component_type>());
		ComponentPools[ComponentID] = NewPool;
	}

	std::shared_ptr<component_pool<component_type>> ComponentPool = std::static_pointer_cast<component_pool<component_type>>(ComponentPools[ComponentID]);
	if(EntityID >= ComponentPool->Data.size())
	{
		ComponentPool->Resize(EntitiesCount);
	}

	component_type NewComponent(std::forward<args>(Args)...);

	ComponentPool->Set(EntityID, NewComponent);
	EntitiesComponentSignatures[EntityID].set(ComponentID);

	return &std::static_pointer_cast<component_pool<component_type>>(ComponentPools[ComponentID])->Get(EntityID);
}

template<typename component_type>
void registry::
RemoveComponent(entity& Object)
{
	const size_t EntityID = Object.Handle - 1;
	const size_t ComponentID = TypeMap[std::type_index(typeid(component_type))];

	EntitiesComponentSignatures[EntityID].set(ComponentID, false);
}

template<typename component_type>
bool registry::
HasComponent(entity& Object)
{
	const size_t EntityID = Object.Handle - 1;
	size_t ComponentID = 0;

	auto FindIt = TypeMap.find(std::type_index(typeid(component_type)));

	if(FindIt != TypeMap.end())
		ComponentID = FindIt->second;
	else
		return false;

	return EntitiesComponentSignatures[EntityID].test(ComponentID);
}

template<typename component_type>
component_type* registry::
GetComponent(entity& Object)
{
	const size_t EntityID = Object.Handle - 1;

	auto it = TypeMap.find(std::type_index(typeid(component_type)));
	if(it == TypeMap.end()) 
	{
		return nullptr;
	}

	const size_t ComponentID = it->second;
	if(HasComponent<component_type>(Object))
	{
		component_type* Component = &std::static_pointer_cast<component_pool<component_type>>(ComponentPools[ComponentID])->Get(EntityID);
		return Component;
	}

	return nullptr;
}

template<typename system_type, typename ...args>
std::shared_ptr<system_type> registry::
AddSystem(args&&... Args)
{
	std::shared_ptr<system_type> NewSystem(new system_type(TypeMap, std::forward<args>(Args)...));
	Systems.insert(std::make_pair(std::type_index(typeid(system_type)), NewSystem));
	return NewSystem;
}

template<typename system_type>
void registry::
RemoveSystem()
{
	auto FoundSystem = Systems.find(std::type_index(typeid(system_type)));
	Systems.erase(FoundSystem);
}

template<typename system_type>
std::shared_ptr<system_type> registry::
GetSystem()
{
	auto it = Systems.find(std::type_index(typeid(system_type)));
	if(it == Systems.end()) 
	{
		return nullptr;
	}
	else
	{
		return std::static_pointer_cast<system_type>(it->second);
	}
}

template<typename system_type>
bool registry::
HasSystem()
{
	return Systems.find(std::type_index(typeid(system_type))) != Systems.end();
}
