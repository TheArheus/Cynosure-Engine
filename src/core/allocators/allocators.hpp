#pragma once

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

template<typename type, typename allocator_type = linear_allocator>
using alloc_vector = std::vector<type, allocator_adapter<type, allocator_type>>;
