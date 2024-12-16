#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #if defined(_DLL)
    #define RENDERER_API __declspec(dllexport)
  #else
    #define RENDERER_API __declspec(dllimport)
  #endif
#elif defined(__GNUC__)
  #if defined(_DLL)
    #define RENDERER_API __attribute__((visibility("default")))
  #else
    #define RENDERER_API
  #endif
#else
  #define RENDERER_API
#endif

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>

#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#else
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#define CComPtr ComPtr
#endif
#define SUBRESOURCES_ALL ~0u
#define STRINGIFY(v) #v
#define BYTE(n) (1 << n)

constexpr size_t KB(size_t val) { return val * 1000; };
constexpr size_t MB(size_t val) { return KB(val) * 1000; };
constexpr size_t GB(size_t val) { return MB(val) * 1000; };

constexpr size_t KiB(size_t val) { return val * 1024; };
constexpr size_t MiB(size_t val) { return KiB(val) * 1024; };
constexpr size_t GiB(size_t val) { return MiB(val) * 1024; };

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
                 
typedef float    r32;
typedef double   r64;

typedef uint32_t b32;
typedef uint64_t b64;

typedef uintptr_t uptr;
typedef  intptr_t sptr;

typedef size_t memory_index;

struct texture_t
{
    u32* Memory;

    u32 Width;
    u32 Height;
};

struct glyph_t
{
    u32* Memory;

    u32 Width;
    u32 Height;
    s32 OffsetX;
    s32 OffsetY;
    s32 FontSize;
	float Advance;
};

#include "core/math.h"
#include "core/allocators/allocators.hpp"

