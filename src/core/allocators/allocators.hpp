#pragma once

class global_memory_allocator;

#include "allocator_base.hpp"
#include "linear_allocator.hpp"

template<typename T, class alloc_type>
class allocator_adapter
{
public:
	typedef T value_type;

	allocator_adapter() = delete;
	allocator_adapter(alloc_type& NewAllocator) noexcept : Allocator(NewAllocator) {}

	template<typename U>
	allocator_adapter(const allocator_adapter<U, alloc_type> Other) noexcept : Allocator(Other.Allocator) {}

	[[nodiscard]] constexpr value_type* allocate(const std::size_t& AllocCount)
	{
		return (value_type*)Allocator.Allocate(AllocCount * sizeof(value_type), alignof(value_type));
	}

	constexpr void deallocate(value_type* Ptr, [[maybe_unused]] std::size_t DeallocCount) noexcept
	{
		Allocator.Free(Ptr);
	}

	std::size_t MaxAllocationSize() const noexcept
	{
		return Allocator.Size;
	}

	bool operator==(const allocator_adapter<T, alloc_type>& Other) const noexcept
	{
		if constexpr(std::is_base_of_v<alloc_type, linear_allocator>)
		{
			return Allocator.Start == Other.Allocator.Start;
		}
		// TODO: Other allocator types if needed
		else
		{
			return true;
		}
	}

	bool operator!=(const allocator_adapter<T, alloc_type>& Other) const noexcept
	{
		return !(*this == Other);
	}

	alloc_type& Allocator;
};


// TODO: implement data for memory footprint visualization
class global_memory_allocator
{

public:
	global_memory_allocator()
	{
		AllocateNewBlock();
	}

	~global_memory_allocator()
	{
		for(auto& Block : MemoryBlocks)
		{
			delete Block;
		}
	}

	void* Allocate(const size_t& Size, const std::uintptr_t& Alignment = sizeof(std::uintptr_t))
	{
		if(MemoryBlocks[CurrentBlockIndex]->Used + Size > MemoryBlocks[CurrentBlockIndex]->Size)
		{
			AllocateNewBlock(Size > BlockSize ? Size : BlockSize);
		}
		return MemoryBlocks[CurrentBlockIndex]->Allocate(Size, Alignment);
	}

	void UpdateAndReset()
	{
		for(auto it = MemoryBlocks.begin(); it != MemoryBlocks.end();)
		{
			if((*it)->Used == 0)
			{
				if((*it)->UnusedCycles > TotalUnusedCyclesCount)
				{
					delete *it;
					it = MemoryBlocks.erase(it);
					continue;
				}
				(*it)->UnusedCycles++;
			}
			else
			{
				(*it)->Clear();
			}
			++it;
		}

		if(MemoryBlocks.empty())
		{
			AllocateNewBlock();
		}	

		CurrentBlockIndex = MemoryBlocks.size() - 1;

		for(auto it = ContainerAllocators.begin(); it != ContainerAllocators.end();)
		{
			free(*it);
		}
		ContainerAllocators.clear();
	}

private:
	u32 CurrentBlockIndex = 0;
	u32 TotalUnusedCyclesCount = 5;
	size_t BlockSize = MiB(128);

	void AllocateNewBlock(const size_t& MemoryBlockSize = MiB(128))
	{
		void* NewBlock = calloc(1, MemoryBlockSize);
		MemoryBlocks.push_back(new linear_allocator(MemoryBlockSize, NewBlock));
		CurrentBlockIndex = MemoryBlocks.size() - 1;
	}

	std::vector<linear_allocator*> MemoryBlocks;
	std::vector<allocator*> ContainerAllocators;
};

static global_memory_allocator Allocator;

#define PushStructConstruct(Type, ...) new (Allocator.Allocate(sizeof(Type), alignof(Type))) Type(__VA_ARGS__)
#define PushArrayConstruct(Type, Count, ...) \
    { \
        Type* memory = (Type*)Allocator.Allocate(sizeof(Type) * Count, alignof(Type)); \
        for (size_t i = 0; i < Count; ++i) { \
            new (&memory[i]) Type(__VA_ARGS__); \
        } \
    }
#define PushStruct(Type) (Type*)Allocator.Allocate(sizeof(Type), alignof(Type))
#define PushArray(Type, Count) (Type*)Allocator.Allocate(sizeof(Type) * Count, alignof(Type))
#define PushSize(Size) Allocator.Allocate(Size)

#define PushAllocStructArg(Type) sizeof(Type), PushStruct(Type)
#define PushAllocArrayArg(Type, Count) sizeof(Type) * Count, PushArray(Type, Count)
#define PushAllocSizeArg(Size) Size, PushSize(Size)


// TODO: use another default allocator
// TODO: implement alloc_vector to use custom allocator inside for everything
template<typename type, typename allocator_type = linear_allocator>
using alloc_vector = std::vector<type, allocator_adapter<type, allocator_type>>;

