#define  VK_NO_PROTOTYPES
#define  VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>

#if _WIN32
	#define NOMINMAX
	#include <windows.h>
	#include <windowsx.h>
	#include <vulkan/vulkan_win32.h>
#else
	#include "core/vendor/glfw/glfw3native.h"
	#include "core/vendor/glfw/glfw3.h"
#endif

#include <core/vendor/Volk/volk.h>
#include <core/vendor/Volk/volk.c>

#define  AMD_EXTENSIONS
#define  NV_EXTENSIONS
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <spirv-headers/spirv.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "core/vendor/imgui/imgui.h"
#include "core/vendor/imgui/imgui.cpp"
#include "core/vendor/imgui/imgui_demo.cpp"
#include "core/vendor/imgui/imgui_draw.cpp"
#include "core/vendor/imgui/imgui_tables.cpp"
#include "core/vendor/imgui/imgui_widgets.cpp"

class window;
#include "core/gfx/common.hpp"
#include "core/gfx/renderer_utils.hpp"
#include "core/gfx/vulkan/vulkan_gfx.hpp"

#if _WIN32
	#include "core/gfx/dx12/directx12_gfx.hpp"
#endif

#include "core/gfx/renderer.h"
#include "core/platform/window.hpp"

u32 GetImageMipLevels(u32 Width, u32 Height)
{
	u32 Result = 1;

	while(Width > 1 || Height > 1)
	{
		Result++;
		Width  >>= 1;
		Height >>= 1;
	}

	return Result;
}

u32 PreviousPowerOfTwo(u32 x)
{
	x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

struct indirect_draw_indexed_command
{
	union
	{
		D3D12_DRAW_INDEXED_ARGUMENTS DxDrawArg;
		VkDrawIndexedIndirectCommand VkDrawArg; // 5
	};
};

struct point_shadow_input
{
	mat4  LightMat;
	vec4  LightPos;
	float FarZ;
};

struct mesh_comp_culling_common_input
{
	mat4  Proj;
	mat4  View;
	plane CullingPlanes[6];
	u32   FrustrumCullingEnabled;
	u32   OcclusionCullingEnabled;
	float NearZ;
	u32   DrawCount;
	u32   MeshCount;
	u32   DebugDrawCount;
	u32   DebugMeshCount;
};

#include "core/scene_manager/scene_manager.h"
