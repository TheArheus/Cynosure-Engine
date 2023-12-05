#pragma once

#define system_constructor(name, ...) name(type_map& NewTypeMap, size_t& NewComponentCount, ##__VA_ARGS__) : entity_system(NewTypeMap, NewComponentCount)

typedef u64 entity_handle;
typedef std::bitset<32> signature;
typedef std::unordered_map<std::type_index, u32> type_map;

struct registry;
struct entity
{
	entity_handle Handle;
	registry* Registry;

	entity(u64 NewHandle, registry* NewRegistry) : Handle(NewHandle), Registry(NewRegistry) {};
	entity(const entity& Oth) : Handle(Oth.Handle), Registry(Oth.Registry) {};

	template<typename component_type, typename... args>
	component_type* AddComponent(args&&... Args);

	template<typename component_type>
	void RemoveComponent();

	template<typename component_type>
	bool HasComponent();

	template<typename component_type>
	component_type* GetComponent();

	bool operator==(entity& Oth) const {return Handle == Oth.Handle;}
	bool operator!=(entity& Oth) const {return Handle != Oth.Handle;}
	bool operator> (entity& Oth) const {return Handle >  Oth.Handle;}
	bool operator< (entity& Oth) const {return Handle <  Oth.Handle;}
};

struct base_component
{
	static size_t NextID;
};

template<typename T>
struct component : public base_component
{
	static size_t GetNextID() 
	{
		size_t Result = NextID++;
		return Result;
	}
};

struct entity_system
{
	signature Signature;
	std::vector<entity> Entities;
	type_map& TypeMap;
	size_t& ComponentCount;

	entity_system(type_map& NewTypeMap, size_t& NewComponentCount) : TypeMap(NewTypeMap), ComponentCount(NewComponentCount) {};
	~entity_system() = default;

	void AddEntity(entity& Entity);
	void RemoveEntity(entity& Entity);

	template<typename component_type>
	void RequireComponent();
};

struct base_pool
{
	virtual ~base_pool() = default;
};

template<typename T>
struct component_pool : public base_pool
{
	std::vector<T> Data;

	component_pool(size_t Capacity = 20)
	{
		Data.resize(Capacity);
	}

	~component_pool() override = default;

	void Add(T& Object)
	{
		Data.push_back(Object);
	}

	void Resize(size_t NewSize)
	{
		Data.resize(NewSize);
	}

	void Set(size_t Idx, T& Object)
	{
		Data[Idx] = Object;
	}

	T& Get(size_t Index)
	{
		return Data[Index];
	}

	T& operator[](size_t Index)
	{
		return Data[Index];
	}
};

typedef std::unordered_map<std::type_index, std::shared_ptr<entity_system>> system_pool;
struct registry
{
	size_t EntitiesCount  = 0;
	size_t ComponentCount = 0;

	std::vector<entity> Entities;

	type_map TypeMap;
	std::vector<signature> EntitiesComponentSignatures;
	std::vector<std::shared_ptr<base_pool>> ComponentPools;

	system_pool Systems;

	entity CreateEntity();
	void AddEntity(entity NewEntity);

	void UpdateSystems();

	template<typename component_type, typename ...args>
	component_type* AddComponent(entity& Object, args&&... Args);

	template<typename component_type>
	void RemoveComponent(entity& Object);

	template<typename component_type>
	bool HasComponent(entity& Object);

	template<typename component_type>
	component_type* GetComponent(entity& Object);

	template<typename system_type, typename ...args>
	std::shared_ptr<system_type> AddSystem(args&&... Args);

	template<typename system_type>
	void RemoveSystem();

	template<typename system_type>
	std::shared_ptr<system_type> GetSystem();

	template<typename system_type>
	bool HasSystem();
};
