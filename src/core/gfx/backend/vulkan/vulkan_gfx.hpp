#pragma once

#define  VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#if _WIN32
	#include <vulkan/vulkan_win32.h>
#elif __linux__
	#include <vulkan/vulkan_xlib.h>
#endif

#define  VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <Volk/volk.h>

#include "vulkan_utilities.hpp"

#include "vulkan_command_queue.h"
#include "vulkan_backend.h"

#include "vulkan_pipeline_context.h"
#include "vulkan_resources.hpp"
