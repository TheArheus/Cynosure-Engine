#pragma once

class allocator
{
public:
	allocator(const std::size_t MemSize, void* const NewStart) noexcept : Size(MemSize), Start(NewStart), Used(0), AllocCount(0) {assert(MemSize > 0);}
	allocator(const allocator&) = delete;
	allocator& operator=(allocator&) = delete;

	allocator(allocator&& Oth) noexcept : Size(Oth.Size), Used(Oth.Used), AllocCount(Oth.AllocCount), Start(Oth.Start)
	{
		Oth.Size = 0;
		Oth.Used = 0;
		Oth.Start = nullptr;
		Oth.AllocCount = 0;
	}
	allocator& operator=(allocator&& Oth) noexcept
	{
		Size = Oth.Size;
		Used = Oth.Used;
		Start = Oth.Start;
		AllocCount = Oth.AllocCount;

		Oth.Size = 0;
		Oth.Used = 0;
		Oth.Start = nullptr;
		Oth.AllocCount = 0;

		return *this;
	}

	virtual ~allocator() noexcept
	{
		assert(Used == 0 && AllocCount == 0);
	}

	virtual void* Allocate(const std::size_t& Size, const std::uintptr_t& Alignment = sizeof(std::uintptr_t)) = 0;
	virtual void  Free(void* const Mem) = 0;

	size_t Size;
	size_t Used;
	size_t AllocCount;

protected:
	void* Start;
};
