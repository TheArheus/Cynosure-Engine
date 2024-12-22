
size_t GlobalNextID = 0;

void entity::
Kill()
{
	Registry->KillEntity(*this);
}
void entity::
AddTag(const std::string& Tag)
{
	Registry->AddTagToEntity(*this, Tag);
}
bool entity::
HasTag(const std::string& Tag)
{
	return Registry->EntityHasTag(*this, Tag);
}
void entity::
AddToGroup(const std::string& Group)
{
	Registry->GroupEntity(*this, Group);
}
bool entity::
BelongsToGroup(const std::string& Group)
{
	return Registry->EntityBelongsToGroup(*this, Group);
}

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
component_type& entity::GetComponent()
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
	Signature.set(GetTypeID<component_type>());
}

entity registry::
CreateEntity()
{
    u64 NewEntityID;

    if(FreeIDs.empty())
    {
        NewEntityID = EntitiesCount++;
        if(NewEntityID >= EntitiesComponentSignatures.size())
        {
            EntitiesComponentSignatures.resize(NewEntityID + 1);
        }
    }
    else
    {
        NewEntityID = FreeIDs.front();
        FreeIDs.pop_front();
    }

    entity NewEntity(NewEntityID, this);
    HotEntities.insert(NewEntity);

    return NewEntity;
}

void registry::
AddEntity(entity Entity)
{
	const signature& EntitySignature = EntitiesComponentSignatures[Entity.Handle];
	for(auto& System : Systems)
	{
		const signature& SystemSignature = System.second->Signature;
		if((EntitySignature & SystemSignature) == SystemSignature)
		{
			System.second->AddEntity(Entity);
		}
	}
}


void registry::
RemoveEntity(entity Entity)
{
    for(auto& System : Systems)
    {
        System.second->RemoveEntity(Entity);
    }
}

void registry::
KillEntity(entity Entity)
{
	ColdEntities.insert(Entity);
}

void registry::
UpdateSystems(double dt)
{
	for(entity Entity : HotEntities)
	{
		AddEntity(Entity);
	}

	HotEntities.clear();

    for(entity Entity : ColdEntities)
    {
        RemoveEntity(Entity);
        EntitiesComponentSignatures[Entity.Handle].reset();

        for(auto Pool : ComponentPools)
        {
            if(Pool) Pool->RemoveEntityFromPool(Entity.Handle);
        }

        FreeIDs.push_back(Entity.Handle);

        RemoveEntityTag(Entity);
        RemoveEntityFromGroup(Entity);
    }

    ColdEntities.clear();

    for (auto& [Type, System] : Systems)
	{
		System->SubscribeOnEvents();
        System->Update(dt);
    }
}

void registry::
ClearAllEntities()
{
    for (auto& [TypeIndex, SystemPtr] : Systems)
    {
        SystemPtr->Entities.clear();
    }

    for (auto& Pool : ComponentPools)
    {
        if (Pool)
        {
            Pool->Clear();
        }
    }

    HotEntities.clear();
    ColdEntities.clear();
    FreeIDs.clear();

    EntitiesComponentSignatures.clear();
    EntitiesCount = 0;

    TagToEntity.clear();
    EntityToTag.clear();
    EntitiesPerGroup.clear();
    GroupPerEntity.clear();
}

template<typename component_type, typename ...args>
component_type* registry::
AddComponent(entity& Object, args&&... Args)
{
	const size_t ComponentID = GetTypeID<component_type>();
	const size_t EntityID = Object.Handle;

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
	const size_t ComponentID = GetTypeID<component_type>();
	const size_t EntityID = Object.Handle;

	EntitiesComponentSignatures[EntityID].set(ComponentID, false);

	if (ComponentPools[ComponentID])
	{
		auto ComponentPool = std::static_pointer_cast<component_pool<component_type>>(ComponentPools[ComponentID]);
		ComponentPool->Remove(EntityID);
	}
}

template<typename component_type>
bool registry::
HasComponent(entity& Object)
{
	const size_t ComponentID = GetTypeID<component_type>();
	const size_t EntityID = Object.Handle;
	return EntitiesComponentSignatures[EntityID].test(ComponentID);
}

template<typename component_type>
component_type& registry::
GetComponent(entity& Object)
{
	const size_t ComponentID = GetTypeID<component_type>();
	const size_t EntityID = Object.Handle;
	std::shared_ptr<component_pool<component_type>> Component = std::static_pointer_cast<component_pool<component_type>>(ComponentPools[ComponentID]);
	return Component->Get(EntityID);
}

template<typename system_type, typename ...args>
std::shared_ptr<system_type> registry::
AddSystem(args&&... Args)
{
	std::shared_ptr<system_type> NewSystem(new system_type(std::forward<args>(Args)...));
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
