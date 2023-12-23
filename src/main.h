#define  VK_NO_PROTOTYPES
#define  VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>

#if _WIN32
	#include <windows.h>
	#include <windowsx.h>
	#include <vulkan/vulkan_win32.h>
#else
	#include "core/vendor/glfw/glfw3native.h"
	#include "core/vendor/glfw/glfw3.h"
#endif

#include <Volk/volk.h>
#include <Volk/volk.c>
#include "core/vendor/vk_mem_alloc.h"

#define  AMD_EXTENSIONS
#define  NV_EXTENSIONS
#include <core/vendor/glslang/Include/glslang_c_interface.h>
#include <core/vendor/glslang/Public/resource_limits_c.h>
#include <core/vendor/glslang/Public/ResourceLimits.h>
#include <core/vendor/glslang/SPIRV/GlslangToSpv.h>
#include <core/vendor/glslang/Public/ShaderLang.h>

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
	VkDrawIndexedIndirectCommand DrawArg; // 5
	u32 CommandIdx;
};

struct point_shadow_input
{
	mat4  LightMat;
	vec4  LightPos;
	float FarZ;
};

struct alignas(16) mesh_comp_culling_common_input
{
	plane CullingPlanes[6];
	u32   FrustrumCullingEnabled;
	u32   OcclusionCullingEnabled;
	float NearZ;
	u32   DrawCount;
	u32   MeshCount;
	u32   DebugDrawCount;
	u32   DebugMeshCount;
	mat4  Proj;
	mat4  View;
};

#include "core/scene_manager/scene_manager.h"
