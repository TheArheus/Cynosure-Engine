#include <windows.h>

#define  VK_NO_PROTOTYPES
#define  VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <Volk/volk.h>
#include <Volk/volk.c>
#include "core/vendor/vk_mem_alloc.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "core/vendor/imgui/imgui.h"
#include "core/vendor/imgui/imgui.cpp"
#include "core/vendor/imgui/imgui_demo.cpp"
#include "core/vendor/imgui/imgui_draw.cpp"
#include "core/vendor/imgui/imgui_tables.cpp"
#include "core/vendor/imgui/imgui_widgets.cpp"

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

#include "core/gfx/vulkan/vulkan_utilities.hpp"
#include "core/gfx/vulkan/command_queue.hpp"
#include "core/gfx/vulkan/resources.hpp"
#include "core/gfx/vulkan/shader_input_signature.hpp"
#include "core/gfx/vulkan/pipeline_context.hpp"
#include "core/gfx/vulkan/renderer_vulkan.h"
#include "core/platform/win32/win32_window.h"
#include "core/scene_manager/scene_manager.h"
