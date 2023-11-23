
#include <windows.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <Volk/volk.h>
#include <Volk/volk.c>

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

#include "gfx/vulkan/vulkan_utilities.hpp"
#include "gfx/vulkan/command_queue.hpp"
#include "gfx/vulkan/resources.hpp"
#include "gfx/vulkan/shader_input_signature.hpp"
#include "gfx/vulkan/pipeline_context.hpp"
#include "gfx/vulkan/renderer_vulkan.h"
#include "platform/win32/win32_window.h"
