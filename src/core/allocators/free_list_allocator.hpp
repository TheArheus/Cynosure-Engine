#pragma once
 
class free_list_allocator : public allocator
{
	friend global_memory_allocator;

public:

	free_list_allocator(const std::size_t& MemSize, void* const Start) : allocator(MemSize, Start), Current(Start) {}
	free_list_allocator(const std::size_t& MemSize) : allocator(MemSize), Current(Start) {}
};
