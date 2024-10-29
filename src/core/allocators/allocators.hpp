#pragma once

class global_memory_allocator;

#include "allocator_base.hpp"
#include "linear_allocator.hpp"


template<typename T, class alloc_type>
class allocator_adapter
{
public:
	using value_type = T;

	allocator_adapter() = delete;
	allocator_adapter(alloc_type& NewAllocator) noexcept : Allocator(NewAllocator) {}

	template<typename U>
	allocator_adapter(const allocator_adapter<U, alloc_type>& Other) noexcept : Allocator(Other.Allocator) {}
	template<typename U>
	allocator_adapter& operator=(const allocator_adapter<U, alloc_type>& Other) noexcept
	{
		if(this != &Other)
		{
			Allocator = Other.Allocator;
		}
		return *this;
	}

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
// TODO: Least Frequently Used Cache
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

	[[nodiscard]] void* Allocate(const size_t& Size, const std::uintptr_t& Alignment = sizeof(std::uintptr_t)) noexcept
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
					free((*it)->Start);
					(*it)->Start = nullptr;
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


// TODO: use another default allocator
template<typename type, typename allocator_type = linear_allocator>
class alloc_vector
{
	using base = std::vector<type, allocator_adapter<type, allocator_type>>;

public:
    explicit alloc_vector(size_t Count)
        : Allocator_(sizeof(type) * Count, PushArray(type, Count)),
          Vector_(Allocator_) {}

    alloc_vector(const alloc_vector& other)
        : Allocator_(other.Allocator_), Vector_(other.Vector_) {}

    alloc_vector(alloc_vector&& other) noexcept
        : Allocator_(std::move(other.Allocator_)), Vector_(std::move(other.Vector_)) {}

    alloc_vector& operator=(const alloc_vector& other) {
        if (this != &other) {
            Vector_ = other.Vector_;
            Allocator_ = other.Allocator_;
        }
        return *this;
    }

    alloc_vector& operator=(alloc_vector&& other) noexcept {
        if (this != &other) {
            Vector_ = std::move(other.Vector_);
            Allocator_ = std::move(other.Allocator_);
        }
        return *this;
    }

    // Element Access
    type& operator[](const size_t& idx) { return Vector_[idx]; }
    const type& operator[](const size_t& idx) const { return Vector_[idx]; }

    type& at(size_t idx) { return Vector_.at(idx); }
    const type& at(size_t idx) const { return Vector_.at(idx); }

    type& front() { return Vector_.front(); }
    const type& front() const { return Vector_.front(); }

    type& back() { return Vector_.back(); }
    const type& back() const { return Vector_.back(); }

    type* data() { return Vector_.data(); }
    const type* data() const { return Vector_.data(); }

    // Capacity
    bool empty() const { return Vector_.empty(); }
    size_t size() const { return Vector_.size(); }
    size_t max_size() const { return Vector_.max_size(); }
    void resize(size_t Count) { Vector_.resize(Count); }
    void resize(size_t Count, const type& Value) { Vector_.resize(Count, Value); }
    void reserve(size_t NewCapacity) { Vector_.reserve(NewCapacity); }
    void shrink_to_fit() { Vector_.shrink_to_fit(); }

    // Modifiers
    void clear() { Vector_.clear(); }
    void insert(typename base::iterator pos, const type& value) { Vector_.insert(pos, value); }
    void insert(typename base::iterator pos, size_t count, const type& value) { Vector_.insert(pos, count, value); }
    typename base::iterator erase(typename base::iterator pos) { return Vector_.erase(pos); }
    typename base::iterator erase(typename base::iterator first, typename base::iterator last) { return Vector_.erase(first, last); }
    void push_back(const type& Value) { Vector_.push_back(Value); }
    void pop_back() { Vector_.pop_back(); }
    void swap(alloc_vector& other) noexcept { Vector_.swap(other.Vector_); }

    // Iterators
    typename base::iterator begin() { return Vector_.begin(); }
    typename base::iterator end() { return Vector_.end(); }
    typename base::const_iterator begin() const { return Vector_.begin(); }
    typename base::const_iterator end() const { return Vector_.end(); }
    typename base::const_iterator cbegin() const { return Vector_.cbegin(); }
    typename base::const_iterator cend() const { return Vector_.cend(); }
    typename base::reverse_iterator rbegin() { return Vector_.rbegin(); }
    typename base::reverse_iterator rend() { return Vector_.rend(); }
    typename base::const_reverse_iterator rbegin() const { return Vector_.crbegin(); }
    typename base::const_reverse_iterator rend() const { return Vector_.crend(); }

    // Additional methods
    void assign(size_t count, const type& value) { Vector_.assign(count, value); }
    template <class InputIt>
    void assign(InputIt first, InputIt last) { Vector_.assign(first, last); }

    template <class... Args>
    typename base::iterator emplace(typename base::iterator pos, Args&&... args) {
        return Vector_.emplace(pos, std::forward<Args>(args)...);
    }

    template <class... Args>
    void emplace_back(Args&&... args) {
        Vector_.emplace_back(std::forward<Args>(args)...);
    }

private:
	allocator_type Allocator_;
	base Vector_;
};
