#pragma once

typedef std::bitset<32> signature;
typedef std::unordered_map<std::type_index, u32> type_map;

extern size_t GlobalNextID;

template<typename T>
inline size_t GetTypeID()
{
	static const size_t ID = GlobalNextID++;
	return ID;
}

struct registry;
struct entity
{
	u64 Handle;
	registry* Registry;

	entity(u64 NewHandle, registry* NewRegistry) : Handle(NewHandle), Registry(NewRegistry) {};
	entity(const entity& Oth) : Handle(Oth.Handle), Registry(Oth.Registry) {};

	void Kill();
	void AddTag(const std::string& Tag);
	bool HasTag(const std::string& Tag);
	void AddToGroup(const std::string& Group);
	bool BelongsToGroup(const std::string& Group);

	template<typename component_type, typename... args>
	component_type* AddComponent(args&&... Args);

	template<typename component_type>
	void RemoveComponent();

	template<typename component_type>
	bool HasComponent();

	template<typename component_type>
	component_type& GetComponent();

	bool operator==(const entity& Oth) const {return Handle == Oth.Handle;}
	bool operator!=(const entity& Oth) const {return Handle != Oth.Handle;}
	bool operator> (const entity& Oth) const {return Handle >  Oth.Handle;}
	bool operator< (const entity& Oth) const {return Handle <  Oth.Handle;}
};

namespace std
{
	template<>
    struct hash<entity>
    {
        size_t operator()(const entity& Entity) const
        {
            size_t Result = hash<uint64_t>{}(*(uint64_t*)&Entity.Handle);
            return Result;
        }
    };
};

struct entity_system
{
	signature Signature;
	std::vector<entity> Entities;

	entity_system() = default;
	~entity_system() = default;

	void AddEntity(entity& Entity);
	void RemoveEntity(entity& Entity);

	virtual void SubscribeOnEvents() {};
	virtual void Update(double dt) {};

	template<typename component_type>
	void RequireComponent();
};

struct base_pool
{
	virtual ~base_pool() = default;
	virtual void RemoveEntityFromPool(u64 EntityId) = 0;
};

template<typename T>
struct component_pool : public base_pool
{
	s32 Size = 0;
	std::vector<T> Data;

    std::unordered_map<u64, size_t> EntityIdToIndex;
    std::unordered_map<size_t, u64> IndexToEntityId;

	component_pool(size_t Capacity = 20)
	{
		Data.resize(Capacity);
	}

	~component_pool() override = default;

	void Clear()
	{
        Data.clear();
        EntityIdToIndex.clear();
        IndexToEntityId.clear();
        Size = 0;
	}

    void RemoveEntityFromPool(u64 EntityId) override
    {
        if(EntityIdToIndex.find(EntityId) != EntityIdToIndex.end())
        {
            Remove(EntityId);
        }
    }

	void Add(T& Object)
	{
		Data.push_back(Object);
	}

	void Resize(size_t NewSize)
	{
		Data.resize(NewSize);
	}

	void Set(u64 EntityId, T& Object)
	{
        if(EntityIdToIndex.find(EntityId) != EntityIdToIndex.end())
        {
            u64 Index = EntityIdToIndex[EntityId];
            Data[Index] = Object;
        }
        else
        {
            u64 Index = Size;
            EntityIdToIndex.emplace(EntityId, Index);
            IndexToEntityId.emplace(Index, EntityId);

            if(Index >= Data.capacity())
            {
                Data.resize(Size * 2);
            }

            Data[Index] = Object;
            Size++;
        }
	}

	void Remove(u64 EntityId)
	{
        u64 IndexOfRemoved = EntityIdToIndex[EntityId];
        u64 IndexOfLast = Size - 1;
        Data[IndexOfRemoved] = Data[IndexOfLast];

        u64 EntityIdOfLastElement = IndexToEntityId[IndexOfLast];
        EntityIdToIndex[EntityIdOfLastElement] = IndexOfRemoved;
        IndexToEntityId[IndexOfRemoved] = EntityIdOfLastElement;

        EntityIdToIndex.erase(EntityId);
        IndexToEntityId.erase(IndexOfLast);

        Size--;
	}

	T& Get(size_t EntityId)
	{
		return Data[EntityIdToIndex[EntityId]];
	}

	T& operator[](size_t EntityId)
	{
		return Data[EntityIdToIndex[EntityId]];
	}
};

typedef std::unordered_map<std::type_index, std::shared_ptr<entity_system>> system_pool;
struct registry
{
	size_t EntitiesCount = 0;

	std::set<entity> HotEntities;
	std::set<entity> ColdEntities;
	std::deque<u64> FreeIDs;

	std::vector<signature> EntitiesComponentSignatures;
	std::vector<std::shared_ptr<base_pool>> ComponentPools;

	std::unordered_map<std::string, entity> TagToEntity;
	std::unordered_map<entity, std::string> EntityToTag;

	std::unordered_map<std::string, std::set<entity>> EntitiesPerGroup;
	std::unordered_map<entity, std::string> GroupPerEntity;

	system_pool Systems;

	entity CreateEntity();
	void AddEntity(entity NewEntity);
	void RemoveEntity(entity Entity);
	void KillEntity(entity Entity);

	void UpdateSystems(double dt);

	void AddTagToEntity(entity Handle, std::string Tag)
	{
		TagToEntity.insert({Tag, Handle});
		EntityToTag.insert({Handle, Tag});
	}

	entity GetEntityByTag(std::string Tag)
	{
		return TagToEntity.at(Tag);
	}

	bool EntityHasTag(entity Entity, const std::string& Tag)
	{
		auto EntityTagIt = EntityToTag.find(Entity);
		if (EntityTagIt == EntityToTag.end())
			return false;

		auto TagEntityIt = TagToEntity.find(Tag);
		if (TagEntityIt == TagToEntity.end())
			return false;

		return TagEntityIt->second == Entity;
	}

	void RemoveEntityTag(entity Entity)
	{
		auto TaggedEntity = EntityToTag.find(Entity);
		if(TaggedEntity != EntityToTag.end())
		{
			auto Tag = TaggedEntity->second;
			TagToEntity.erase(Tag);
			EntityToTag.erase(TaggedEntity);
		}
	}

	void GroupEntity(entity Entity, std::string Group)
	{
		EntitiesPerGroup[Group].insert(Entity);
		GroupPerEntity.emplace(Entity, Group);
	}

	std::set<entity> GetEntitiesByGroup(std::string Group)
	{
		return EntitiesPerGroup[Group];
	}

	bool EntityBelongsToGroup(entity Entity, const std::string& Group)
	{
		if(EntitiesPerGroup.find(Group) == EntitiesPerGroup.end()) return false;

		std::set<entity>& GroupEntities = EntitiesPerGroup.at(Group);
		return GroupEntities.find(Entity) != GroupEntities.end();
	}

	void RemoveEntityFromGroup(entity Entity)
	{
		auto FoundGroup = GroupPerEntity.find(Entity);
		if(FoundGroup != GroupPerEntity.end())
		{
			auto FoundGroupEntities = EntitiesPerGroup.find(FoundGroup->second);
			if(FoundGroupEntities != EntitiesPerGroup.end())
			{
				auto EntityInGroup = std::find(FoundGroupEntities->second.begin(), FoundGroupEntities->second.end(), Entity);
				if(EntityInGroup != FoundGroupEntities->second.end())
				{
					FoundGroupEntities->second.erase(EntityInGroup);
				}
			}
			GroupPerEntity.erase(FoundGroup);
		}
	}

	template<typename component_type, typename ...args>
	component_type* AddComponent(entity& Object, args&&... Args);

	template<typename component_type>
	void RemoveComponent(entity& Object);

	template<typename component_type>
	bool HasComponent(entity& Object);

	template<typename component_type>
	component_type& GetComponent(entity& Object);

	template<typename system_type, typename ...args>
	std::shared_ptr<system_type> AddSystem(args&&... Args);

	template<typename system_type>
	void RemoveSystem();

	template<typename system_type>
	std::shared_ptr<system_type> GetSystem();

	template<typename system_type>
	bool HasSystem();
};
