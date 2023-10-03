#define _CRT_SECURE_NO_WARNINGS

#ifdef ENGINE_EXPORT_CODE
	#define ENGINE_EXPORT_CODE __declspec(dllexport)
#else
	#define ENGINE_EXPORT_CODE __declspec(dllimport)
#endif

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <iostream>
#include <string>
#include <fstream>
#include <string_view>
#include <vector>
#include <map>
#include <array>
#include <bitset>
#include <unordered_map>
#include <initializer_list>
#include <type_traits>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum button_symbol
{
	EC_LBUTTON        = 0x01,
	EC_RBUTTON        = 0x02,
	EC_MBUTTON        = 0x04,

	EC_BACK           = 0x08,
	EC_TAB            = 0x09,

	EC_CLEAR          = 0x0C,
	EC_RETURN         = 0x0D,

	EC_SHIFT          = 0x10,
	EC_CONTROL        = 0x11,
	EC_MENU           = 0x12,
	EC_PAUSE          = 0x13,
	EC_CAPITAL        = 0x14,

	EC_ESCAPE         = 0x1B,

	EC_SPACE          = 0x20,
	EC_PRIOR          = 0x21,
	EC_NEXT           = 0x22,
	EC_END            = 0x23,
	EC_HOME           = 0x24,
	EC_LEFT           = 0x25,
	EC_UP             = 0x26,
	EC_RIGHT          = 0x27,
	EC_DOWN           = 0x28,
	EC_SELECT         = 0x29,
	EC_PRINT          = 0x2A,
	EC_EXECUTE        = 0x2B,
	EC_SNAPSHOT       = 0x2C,
	EC_INSERT         = 0x2D,
	EC_DELETE         = 0x2E,
	EC_HELP           = 0x2F,

	EC_0              = 0x30,
	EC_1              = 0x31,
	EC_2              = 0x32,
	EC_3              = 0x33,
	EC_4              = 0x34,
	EC_5              = 0x35,
	EC_6              = 0x36,
	EC_7              = 0x37,
	EC_8              = 0x38,
	EC_9              = 0x39,

	EC_A              = 0x41,
	EC_B              = 0x42,
	EC_C              = 0x43,
	EC_D              = 0x44,
	EC_E              = 0x45,
	EC_F              = 0x46,
	EC_G              = 0x47,
	EC_H              = 0x48,
	EC_I              = 0x49,
	EC_J              = 0x4A,
	EC_K              = 0x4B,
	EC_L              = 0x4C,
	EC_M              = 0x4D,
	EC_N              = 0x4E,
	EC_O              = 0x4F,
	EC_P              = 0x50,
	EC_Q              = 0x51,
	EC_R              = 0x52,
	EC_S              = 0x53,
	EC_T              = 0x54,
	EC_U              = 0x55,
	EC_V              = 0x56,
	EC_W              = 0x57,
	EC_X              = 0x58,
	EC_Y              = 0x59,
	EC_Z              = 0x5A,

	EC_LWIN           = 0x5B,
	EC_RWIN           = 0x5C,

	EC_SLEEP          = 0x5F,

	EC_NUMPAD0        = 0x60,
	EC_NUMPAD1        = 0x61,
	EC_NUMPAD2        = 0x62,
	EC_NUMPAD3        = 0x63,
	EC_NUMPAD4        = 0x64,
	EC_NUMPAD5        = 0x65,
	EC_NUMPAD6        = 0x66,
	EC_NUMPAD7        = 0x67,
	EC_NUMPAD8        = 0x68,
	EC_NUMPAD9        = 0x69,
	EC_MULTIPLY       = 0x6A,
	EC_ADD            = 0x6B,
	EC_SEPARATOR      = 0x6C,
	EC_SUBTRACT       = 0x6D,
	EC_DECIMAL        = 0x6E,
	EC_DIVIDE         = 0x6F,
	EC_F1             = 0x70,
	EC_F2             = 0x71,
	EC_F3             = 0x72,
	EC_F4             = 0x73,
	EC_F5             = 0x74,
	EC_F6             = 0x75,
	EC_F7             = 0x76,
	EC_F8             = 0x77,
	EC_F9             = 0x78,
	EC_F10            = 0x79,
	EC_F11            = 0x7A,
	EC_F12            = 0x7B,
	EC_F13            = 0x7C,
	EC_F14            = 0x7D,
	EC_F15            = 0x7E,
	EC_F16            = 0x7F,
	EC_F17            = 0x80,
	EC_F18            = 0x81,
	EC_F19            = 0x82,
	EC_F20            = 0x83,
	EC_F21            = 0x84,
	EC_F22            = 0x85,
	EC_F23            = 0x86,
	EC_F24            = 0x87,

	EC_NUMLOCK        = 0x90,
	EC_SCROLL         = 0x91,

	EC_LSHIFT         = 0xA0,
	EC_RSHIFT         = 0xA1,
	EC_LCONTROL       = 0xA2,
	EC_RCONTROL       = 0xA3,
	EC_LMENU          = 0xA4,
	EC_RMENU          = 0xA5,

	EC_GAMEPAD_A                         = 0xC3,
	EC_GAMEPAD_B                         = 0xC4,
	EC_GAMEPAD_X                         = 0xC5,
	EC_GAMEPAD_Y                         = 0xC6,
	EC_GAMEPAD_RIGHT_SHOULDER            = 0xC7,
	EC_GAMEPAD_LEFT_SHOULDER             = 0xC8,
	EC_GAMEPAD_LEFT_TRIGGER              = 0xC9,
	EC_GAMEPAD_RIGHT_TRIGGER             = 0xCA,
	EC_GAMEPAD_DPAD_UP                   = 0xCB,
	EC_GAMEPAD_DPAD_DOWN                 = 0xCC,
	EC_GAMEPAD_DPAD_LEFT                 = 0xCD,
	EC_GAMEPAD_DPAD_RIGHT                = 0xCE,
	EC_GAMEPAD_MENU                      = 0xCF,
	EC_GAMEPAD_VIEW                      = 0xD0,
	EC_GAMEPAD_LEFT_THUMBSTICK_BUTTON    = 0xD1,
	EC_GAMEPAD_RIGHT_THUMBSTICK_BUTTON   = 0xD2,
	EC_GAMEPAD_LEFT_THUMBSTICK_UP        = 0xD3,
	EC_GAMEPAD_LEFT_THUMBSTICK_DOWN      = 0xD4,
	EC_GAMEPAD_LEFT_THUMBSTICK_RIGHT     = 0xD5,
	EC_GAMEPAD_LEFT_THUMBSTICK_LEFT      = 0xD6,
	EC_GAMEPAD_RIGHT_THUMBSTICK_UP       = 0xD7,
	EC_GAMEPAD_RIGHT_THUMBSTICK_DOWN     = 0xD8,
	EC_GAMEPAD_RIGHT_THUMBSTICK_RIGHT    = 0xD9,
	EC_GAMEPAD_RIGHT_THUMBSTICK_LEFT     = 0xDA,
};


// NOTE: unsigned 
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// NOTE: signed
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
                 
typedef float    r32;
typedef double   r64;

typedef uint32_t b32;
typedef uint64_t b64;

constexpr size_t KB(size_t val) { return val * 1000; };
constexpr size_t MB(size_t val) { return KB(val) * 1000; };
constexpr size_t GB(size_t val) { return MB(val) * 1000; };

constexpr size_t KiB(size_t val) { return val * 1024; };
constexpr size_t MiB(size_t val) { return KiB(val) * 1024; };
constexpr size_t GiB(size_t val) { return MiB(val) * 1024; };

template<typename T> struct type_name;

#include "math.h"
#include "mesh.h"

struct buttons
{
	bool IsDown;
	bool WasDown;
};

struct view_data
{
	vec3 CameraPos;
	vec3 ViewDir;
};

struct game_input
{
	buttons* Buttons;
	r32 DeltaTime;
};

struct alignas(16) mesh_draw_command_input
{
	mesh::material Mat;
	vec4 Translate;
	vec4 Scale;
	u32  MeshIndex;
	bool IsVisible;
};

// NOTE: When light is point(w is a radius), then pos. Otherwise it is dir(w is cutoff angle for spot light).
struct alignas(16) light_source
{
	vec4 Pos;
	vec4 Dir;
	vec4 Col;
	u32  LightType;
};

struct texture_data
{
	u32 Width;
	u32 Height;
	u32 Depth;
	void* Data;
};

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

// TODO: different object types: mesh/game object, particle object, maybe something else???
class mesh_object
{
public:
	mesh_object() = default;
	mesh_object(u32 NewMeshIdx) : MeshIndex(NewMeshIdx) {}; 

	void AddInstance(vec4 Translate, vec4 Scale, bool IsVisible)
	{
		mesh_draw_command_input Command = {};
		Command.Translate = Translate;
		Command.Scale = Scale;
		Command.IsVisible = IsVisible;
		Command.MeshIndex = MeshIndex;
		ObjectInstances.push_back(Command);
	}

	void AddInstance(mesh::material Mat, vec4 Translate, vec4 Scale, bool IsVisible)
	{
		mesh_draw_command_input Command = {};
		Command.Mat = Mat;
		Command.Translate = Translate;
		Command.Scale = Scale;
		Command.IsVisible = IsVisible;
		Command.MeshIndex = MeshIndex;
		ObjectInstances.push_back(Command);
	}

	void UpdateCommands(std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>>& DrawCommands)
	{
		DrawCommands.insert(DrawCommands.end(), ObjectInstances.begin(), ObjectInstances.end());
	}

	u32 MeshIndex;

private:
	std::vector<mesh_draw_command_input> ObjectInstances;
};

// TODO: Think how to implement for a multiple scenes at one time
// TODO: Choose a scene at runtime
class scene
{
public:
	virtual bool LoadScene() = 0;
	virtual void Update() = 0;

	bool IsLoaded = false;

	std::vector<texture_data> Textures;
	std::vector<light_source, allocator_adapter<light_source, linear_allocator>> LightSources;

	mesh Meshes;
	mesh_object ObjectCommands;
};

#define GameSetupFunc(name) void ENGINE_EXPORT_CODE name(u32& MemorySize)
typedef GameSetupFunc(game_setup);

#define GameUpdateAndRenderFunc(name) void ENGINE_EXPORT_CODE name(bool& SceneIsLoaded, std::vector<mesh_draw_command_input, allocator_adapter<mesh_draw_command_input, linear_allocator>>& MeshDrawCommands, mesh& Geometries, std::vector<light_source, allocator_adapter<light_source, linear_allocator>>& LightSources, const game_input& GameInput, view_data& ViewData)
typedef GameUpdateAndRenderFunc(game_update_and_render);

