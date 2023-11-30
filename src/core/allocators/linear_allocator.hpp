#pragma once

class linear_allocator : public allocator
{
public:
	linear_allocator(const std::size_t MemSize, void* const Start) : allocator(MemSize, Start), Current(Start) {}
	linear_allocator(linear_allocator&& Other) : allocator(std::move(Other)), Current(Other.Current) { Other.Current = nullptr; }
	~linear_allocator() noexcept
	{
		Clear();
	}

	linear_allocator& operator=(linear_allocator&& Other)
	{
		allocator::operator=(std::move(Other));

		Current = Other.Current;
		Other.Current = nullptr;

		return *this;
	}

	virtual void* Allocate(const std::size_t& AllocSize, const std::uintptr_t& Alignment = sizeof(std::uintptr_t)) override
	{
		assert(Size > 0 && Alignment > 0);

		uintptr_t Ptr = (std::uintptr_t)Current;
		uintptr_t Aligned = AlignUp(Ptr, Alignment);
		size_t Forward = Aligned - Ptr;

		if(Used + Forward + AllocSize > Size) return nullptr;

		void* AlignedAddr = (void*)((uintptr_t)Current + Forward);
		Current = (void*)((uintptr_t)AlignedAddr + AllocSize);
		Used = (uintptr_t)Current - (uintptr_t)Start;

		AllocCount++;
		return AlignedAddr;
	}

	virtual void Free(void* const Mem) noexcept override {}

	virtual void Clear() noexcept
	{
		Current = Start;
		AllocCount = 0;
		Used = 0;
	}

	virtual void Rewind(void* const Mark) noexcept
	{
		assert(Current >= Mark && Start <= Mark);
		Current = Mark;
		Used = (uintptr_t)Current - (uintptr_t)Start;
	}

protected:
	void* Current;
};
