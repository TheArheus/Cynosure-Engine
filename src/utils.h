
#include <windows.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <Volk/volk.h>
#include <Volk/volk.c>

struct game_code
{
	HMODULE Library;
	game_setup*  Setup;
	game_start*  Start;
	game_update* Update;
};

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

#include "vulkan_utilities.hpp"
#include "command_queue.hpp"
#include "resources.hpp"
#include "shader_input_signature.hpp"
#include "pipeline_context.hpp"
#include "renderer_vulkan.h"
#include "win32_window.h"
