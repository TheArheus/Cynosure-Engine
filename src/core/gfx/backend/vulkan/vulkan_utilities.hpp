#pragma once

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_vulkan.cpp>

#define VK_CHECK(call, ...) \
	do { \
		VkResult CallResult = call; \
		if(!IsVulkanResultSuccess(CallResult)) { \
			printf("[DEBUG] %s\n", VulkanResultString(CallResult, ##__VA_ARGS__)); \
			assert(CallResult == VK_SUCCESS); \
		} \
	} while(0);


VkBool32 
DebugReportCallback(VkDebugReportFlagsEXT Flags, 
					VkDebugReportObjectTypeEXT ObjectType, 
					u64 Object, size_t Location, s32 MessageCode, 
					const char* pLayerPrefix, const char* pMessage, 
					void* pUserData)
{
	const char* ErrorType = (Flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "Error" :
							(Flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) ? "Warning" : 
							(Flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) ? "Performance Warning" :
							(Flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) ? "Debug" : "Unknown";

	char Message[4096];
	snprintf(Message, 4096, "[%s]: %s\n", ErrorType, pMessage);
	printf("%s", Message);

	return VK_FALSE;
}

u32 GetGraphicsQueueFamilyIndex(VkPhysicalDevice Device)
{
	u32 Result = VK_QUEUE_FAMILY_IGNORED;

	u32 QueueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyPropertiesCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilyProperties(QueueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyPropertiesCount, QueueFamilyProperties.data());

	for(u32 FamilyPropertyIndex = 0;
		FamilyPropertyIndex < QueueFamilyPropertiesCount;
		++FamilyPropertyIndex)
	{
		if(QueueFamilyProperties[FamilyPropertyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			Result = FamilyPropertyIndex;
			break;
		}
	}

	return Result;
}

VkPhysicalDevice
PickPhysicalDevice(const std::vector<VkPhysicalDevice>& PhysicalDevices)
{
	VkPhysicalDevice Discrete = 0;
	VkPhysicalDevice Fallback = 0;

	for(const VkPhysicalDevice& Device : PhysicalDevices)
	{
		VkPhysicalDeviceProperties Props;
		vkGetPhysicalDeviceProperties(Device, &Props);

		if(!Discrete && Props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			Discrete = Device;
		}

		if(!Fallback)
		{
			Fallback = Device;
		}
	}

	return Discrete ? Discrete : Fallback;
}


VkSurfaceFormatKHR 
GetSwapchainFormat(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
	u32 FormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, 0);
	std::vector<VkSurfaceFormatKHR> Formats(FormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, Formats.data());
#if 0
	for(u32 FormatIndex = 0;
		FormatIndex < FormatCount;
		++FormatIndex)
	{
		if(Formats[FormatIndex].format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 || Formats[FormatIndex].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
		{
			return Formats[FormatIndex];
		}
	}
#endif

	for(u32 FormatIndex = 0;
		FormatIndex < FormatCount;
		++FormatIndex)
	{
		if(Formats[FormatIndex].format == VK_FORMAT_R8G8B8A8_UNORM || Formats[FormatIndex].format == VK_FORMAT_B8G8R8A8_UNORM)
		{
			return Formats[FormatIndex];
		}
	}

	return Formats[0];
}

u32 SelectMemoryType(const VkPhysicalDeviceMemoryProperties& MemoryProperties, u32 MemoryTypeBits, VkMemoryPropertyFlags Flags)
{
	for(u32 PropertyIndex = 0;
		PropertyIndex < MemoryProperties.memoryTypeCount;
		++PropertyIndex)
	{
		if((MemoryTypeBits & (1 << PropertyIndex)) != 0 && (MemoryProperties.memoryTypes[PropertyIndex].propertyFlags & Flags) == Flags)
		{
			return PropertyIndex;
		}
	}

	return ~0u;
}

VkImageMemoryBarrier
CreateImageBarrier(VkImage Image, VkAccessFlags SrcAccess, VkAccessFlags DstAccess, 
				   VkImageLayout SrcImageLayout, VkImageLayout DstImageLayout, VkImageAspectFlags AspectMask = VK_IMAGE_ASPECT_COLOR_BIT, u32 BaseArrayLayer = 0, u32 ArrayLayersCount = VK_REMAINING_ARRAY_LAYERS, u32 BaseMip = 0, u32 MipLevelsCount = VK_REMAINING_MIP_LEVELS)
{
	VkImageMemoryBarrier ImageMemoryBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	ImageMemoryBarrier.srcAccessMask = SrcAccess;
	ImageMemoryBarrier.dstAccessMask = DstAccess;
	ImageMemoryBarrier.oldLayout = SrcImageLayout;
	ImageMemoryBarrier.newLayout = DstImageLayout;
	ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrier.image = Image;
	ImageMemoryBarrier.subresourceRange.aspectMask = AspectMask;
	ImageMemoryBarrier.subresourceRange.baseMipLevel   = BaseMip;
	ImageMemoryBarrier.subresourceRange.levelCount = MipLevelsCount;
	ImageMemoryBarrier.subresourceRange.baseArrayLayer = BaseArrayLayer;
	ImageMemoryBarrier.subresourceRange.layerCount = ArrayLayersCount;
	return ImageMemoryBarrier;
}

void ImageBarrier(VkCommandBuffer CommandBuffer, 
					 VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask,
					 const std::vector<VkImageMemoryBarrier>& ImageMemoryBarriers)
{
	if(ImageMemoryBarriers.size())
	{
		vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, 
							 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 
							 (u32)ImageMemoryBarriers.size(), ImageMemoryBarriers.data());
	}
}

const char* VulkanResultString(VkResult Result, bool GetExtended = false)
{    
    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    // Success Codes
    switch (Result) {
        default:
        case VK_SUCCESS:
            return !GetExtended ? "VK_SUCCESS" : "VK_SUCCESS Command successfully completed";
        case VK_NOT_READY:
            return !GetExtended ? "VK_NOT_READY" : "VK_NOT_READY A fence or query has not yet completed";
        case VK_TIMEOUT:
            return !GetExtended ? "VK_TIMEOUT" : "VK_TIMEOUT A wait operation has not completed in the specified time";
        case VK_EVENT_SET:
            return !GetExtended ? "VK_EVENT_SET" : "VK_EVENT_SET An event is signaled";
        case VK_EVENT_RESET:
            return !GetExtended ? "VK_EVENT_RESET" : "VK_EVENT_RESET An event is unsignaled";
        case VK_INCOMPLETE:
            return !GetExtended ? "VK_INCOMPLETE" : "VK_INCOMPLETE A return array was too small for the result";
        case VK_SUBOPTIMAL_KHR:
            return !GetExtended ? "VK_SUBOPTIMAL_KHR" : "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
        case VK_THREAD_IDLE_KHR:
            return !GetExtended ? "VK_THREAD_IDLE_KHR" : "VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
        case VK_THREAD_DONE_KHR:
            return !GetExtended ? "VK_THREAD_DONE_KHR" : "VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.";
        case VK_OPERATION_DEFERRED_KHR:
            return !GetExtended ? "VK_OPERATION_DEFERRED_KHR" : "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return !GetExtended ? "VK_OPERATION_NOT_DEFERRED_KHR" : "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return !GetExtended ? "VK_PIPELINE_COMPILE_REQUIRED_EXT" : "VK_PIPELINE_COMPILE_REQUIRED_EXT A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";

        // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return !GetExtended ? "VK_ERROR_OUT_OF_HOST_MEMORY" : "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return !GetExtended ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED:
            return !GetExtended ? "VK_ERROR_INITIALIZATION_FAILED" : "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could not be completed for implementation-specific reasons.";
        case VK_ERROR_DEVICE_LOST:
            return !GetExtended ? "VK_ERROR_DEVICE_LOST" : "VK_ERROR_DEVICE_LOST The logical or physical device has been lost. See Lost Device";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return !GetExtended ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return !GetExtended ? "VK_ERROR_LAYER_NOT_PRESENT" : "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return !GetExtended ? "VK_ERROR_EXTENSION_NOT_PRESENT" : "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return !GetExtended ? "VK_ERROR_FEATURE_NOT_PRESENT" : "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return !GetExtended ? "VK_ERROR_INCOMPATIBLE_DRIVER" : "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return !GetExtended ? "VK_ERROR_TOO_MANY_OBJECTS" : "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return !GetExtended ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported on this device.";
        case VK_ERROR_FRAGMENTED_POOL:
            return !GetExtended ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
        case VK_ERROR_SURFACE_LOST_KHR:
            return !GetExtended ? "VK_ERROR_SURFACE_LOST_KHR" : "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return !GetExtended ? "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" : "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return !GetExtended ? "VK_ERROR_OUT_OF_DATE_KHR" : "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return !GetExtended ? "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" : "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
        case VK_ERROR_INVALID_SHADER_NV:
            return !GetExtended ? "VK_ERROR_INVALID_SHADER_NV" : "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return !GetExtended ? "VK_ERROR_OUT_OF_POOL_MEMORY" : "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return !GetExtended ? "VK_ERROR_INVALID_EXTERNAL_HANDLE" : "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a valid handle of the specified type.";
        case VK_ERROR_FRAGMENTATION:
            return !GetExtended ? "VK_ERROR_FRAGMENTATION" : "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due to fragmentation.";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
            return !GetExtended ? "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" : "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed because the requested address is not available.";
        // NOTE: Same as above
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        //    return !GetExtended ? "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" :"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return !GetExtended ? "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" : "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
        case VK_ERROR_UNKNOWN:
            return !GetExtended ? "VK_ERROR_UNKNOWN" : "VK_ERROR_UNKNOWN An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    }
}

bool IsVulkanResultSuccess(VkResult Result)
{    
    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    switch (Result) {
            // Success Codes
        default:
        case VK_SUCCESS:
        case VK_NOT_READY:
        case VK_TIMEOUT:
        case VK_EVENT_SET:
        case VK_EVENT_RESET:
        case VK_INCOMPLETE:
        case VK_SUBOPTIMAL_KHR:
        case VK_THREAD_IDLE_KHR:
        case VK_THREAD_DONE_KHR:
        case VK_OPERATION_DEFERRED_KHR:
        case VK_OPERATION_NOT_DEFERRED_KHR:
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return true;

        // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_INITIALIZATION_FAILED:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_MEMORY_MAP_FAILED:
        case VK_ERROR_LAYER_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_INVALID_SHADER_NV:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        case VK_ERROR_FRAGMENTATION:
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
        // NOTE: Same as above
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        case VK_ERROR_UNKNOWN:
            return false;
    }
}

VkFormat GetVKFormat(image_format Format)
{
	switch (Format)
	{
	case image_format::UNDEFINED:
		return VK_FORMAT_UNDEFINED;
		// 8 bit
	case image_format::R8_SINT:
		return VK_FORMAT_R8_SINT;
	case image_format::R8_UINT:
		return VK_FORMAT_R8_UINT;
	case image_format::R8_UNORM:
		return VK_FORMAT_R8_UNORM;
	case image_format::R8_SNORM:
		return VK_FORMAT_R8_SNORM;

	case image_format::R8G8_SINT:
		return VK_FORMAT_R8G8_SINT;
	case image_format::R8G8_UINT:
		return VK_FORMAT_R8G8_UINT;
	case image_format::R8G8_UNORM:
		return VK_FORMAT_R8G8_UNORM;
	case image_format::R8G8_SNORM:
		return VK_FORMAT_R8G8_SNORM;

	case image_format::R8G8B8A8_SINT:
		return VK_FORMAT_R8G8B8A8_SINT;
	case image_format::R8G8B8A8_UINT:
		return VK_FORMAT_R8G8B8A8_UINT;
	case image_format::R8G8B8A8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case image_format::R8G8B8A8_SNORM:
		return VK_FORMAT_R8G8B8A8_SNORM;
	case image_format::R8G8B8A8_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;

	case image_format::B8G8R8A8_UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case image_format::B8G8R8A8_SRGB:
		return VK_FORMAT_B8G8R8A8_SRGB;

		// 16 bit
	case image_format::R16_SINT:
		return VK_FORMAT_R16_SINT;
	case image_format::R16_UINT:
		return VK_FORMAT_R16_UINT;
	case image_format::R16_UNORM:
		return VK_FORMAT_R16_UNORM;
	case image_format::R16_SNORM:
		return VK_FORMAT_R16_SNORM;
	case image_format::R16_SFLOAT:
		return VK_FORMAT_R16_SFLOAT;

	case image_format::R16G16_SINT:
		return VK_FORMAT_R16G16_SINT;
	case image_format::R16G16_UINT:
		return VK_FORMAT_R16G16_UINT;
	case image_format::R16G16_UNORM:
		return VK_FORMAT_R16G16_UNORM;
	case image_format::R16G16_SNORM:
		return VK_FORMAT_R16G16_SNORM;
	case image_format::R16G16_SFLOAT:
		return VK_FORMAT_R16G16_SFLOAT;

	case image_format::R16G16B16A16_SINT:
		return VK_FORMAT_R16G16B16A16_SINT;
	case image_format::R16G16B16A16_UINT:
		return VK_FORMAT_R16G16B16A16_UINT;
	case image_format::R16G16B16A16_UNORM:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case image_format::R16G16B16A16_SNORM:
		return VK_FORMAT_R16G16B16A16_SNORM;
	case image_format::R16G16B16A16_SFLOAT:
		return VK_FORMAT_R16G16B16A16_SFLOAT;

		// 32 bit
	case image_format::R32_SINT:
		return VK_FORMAT_R32_SINT;
	case image_format::R32_UINT:
		return VK_FORMAT_R32_UINT;
	case image_format::R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;

	case image_format::R32G32_SINT:
		return VK_FORMAT_R32G32_SINT;
	case image_format::R32G32_UINT:
		return VK_FORMAT_R32G32_UINT;
	case image_format::R32G32_SFLOAT:
		return VK_FORMAT_R32G32_SFLOAT;

	case image_format::R32G32B32_SFLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case image_format::R32G32B32_SINT:
		return VK_FORMAT_R32G32B32_SINT;
	case image_format::R32G32B32_UINT:
		return VK_FORMAT_R32G32B32_UINT;

	case image_format::R32G32B32A32_SINT:
		return VK_FORMAT_R32G32B32A32_SINT;
	case image_format::R32G32B32A32_UINT:
		return VK_FORMAT_R32G32B32A32_UINT;
	case image_format::R32G32B32A32_SFLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;

		// depth-stencil
	case image_format::D32_SFLOAT:
		return VK_FORMAT_D32_SFLOAT;
	case image_format::D24_UNORM_S8_UINT:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	case image_format::D16_UNORM:
		return VK_FORMAT_D16_UNORM;

		// misc
	case image_format::R11G11B10_SFLOAT:
		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case image_format::R10G0B10A2_INT:
		return VK_FORMAT_A2B10G10R10_SINT_PACK32;
	case image_format::BC3_BLOCK_SRGB:
		return VK_FORMAT_BC3_SRGB_BLOCK;
	case image_format::BC3_BLOCK_UNORM:
		return VK_FORMAT_BC3_UNORM_BLOCK;

	default:
		return VK_FORMAT_UNDEFINED;
	}
}

VkImageType GetVKImageType(image_type Type)
{
	switch(Type)
	{
	case image_type::Texture1D:
		return VK_IMAGE_TYPE_1D;
	case image_type::Texture2D:
		return VK_IMAGE_TYPE_2D;
	case image_type::Texture3D:
		return VK_IMAGE_TYPE_3D;
	default:
		return VK_IMAGE_TYPE_2D;
	}
}

VkIndexType GetVKIndexType(index_type Type)
{
	switch (Type)
	{
	case index_type::U16:
		return VK_INDEX_TYPE_UINT16;
	case index_type::U32:
		return VK_INDEX_TYPE_UINT32;
	default:
		return VK_INDEX_TYPE_UINT32;
	}
}

VkPolygonMode GetVKPolygonMode(polygon_mode Mode)
{
	switch (Mode)
	{
	case polygon_mode::fill:
		return VK_POLYGON_MODE_FILL;
	case polygon_mode::line:
		return VK_POLYGON_MODE_LINE;
	case polygon_mode::point:
		return VK_POLYGON_MODE_POINT;
	default:
		return VK_POLYGON_MODE_FILL;
	}
}

u32 GetVKCullMode(cull_mode Mode)
{
	switch (Mode)
	{
	case cull_mode::none:
		return VK_CULL_MODE_NONE;
	case cull_mode::front:
		return VK_CULL_MODE_FRONT_BIT;
	case cull_mode::back:
		return VK_CULL_MODE_BACK_BIT;
	default:
		return VK_CULL_MODE_NONE;
	}
}

VkFrontFace GetVKFrontFace(front_face Face)
{
	switch (Face)
	{
	case front_face::clock_wise:
		return VK_FRONT_FACE_CLOCKWISE;
	case front_face::counter_clock_wise:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	default:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

u32 GetVKShaderStage(shader_stage Stage)
{
	switch (Stage)
	{
	case shader_stage::vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case shader_stage::tessellation_control:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case shader_stage::tessellation_eval:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case shader_stage::geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case shader_stage::fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case shader_stage::compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	case shader_stage::all:
		return VK_SHADER_STAGE_ALL;
	default:
		return VK_SHADER_STAGE_ALL;
	}
}

VkCompareOp GetVKCompareOp(compare_op Op)
{
	switch (Op)
	{
	case compare_op::never:
		return VK_COMPARE_OP_NEVER;
	case compare_op::less:
		return VK_COMPARE_OP_LESS;
	case compare_op::equal:
		return VK_COMPARE_OP_EQUAL;
	case compare_op::less_equal:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case compare_op::greater:
		return VK_COMPARE_OP_GREATER;
	case compare_op::not_equal:
		return VK_COMPARE_OP_NOT_EQUAL;
	case compare_op::greater_equal:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case compare_op::always:
		return VK_COMPARE_OP_ALWAYS;
	default:
		return VK_COMPARE_OP_ALWAYS;
	}
}

VkStencilOp GetVKStencilOp(stencil_op Op)
{
	switch (Op)
	{
	case stencil_op::keep:
		return VkStencilOp::VK_STENCIL_OP_KEEP;
	case stencil_op::zero:
		return VkStencilOp::VK_STENCIL_OP_ZERO;
	case stencil_op::replace:
		return VkStencilOp::VK_STENCIL_OP_REPLACE;
	case stencil_op::increment_clamp:
		return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case stencil_op::decrement_clamp:
		return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case stencil_op::invert:
		return VkStencilOp::VK_STENCIL_OP_INVERT;
	case stencil_op::increment_wrap:
		return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case stencil_op::decrement_wrap:
		return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_WRAP;
	default:
		return VK_STENCIL_OP_KEEP;
	}
}

VkAttachmentLoadOp GetVKLoadOp(load_op Op)
{
	switch (Op)
	{
	case load_op::load:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	case load_op::clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case load_op::dont_care:
	case load_op::none:
	default:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}

VkAttachmentStoreOp GetVKStoreOp(store_op Op)
{
	switch (Op)
	{
	case store_op::store:
		return VK_ATTACHMENT_STORE_OP_STORE;
	case store_op::dont_care:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case store_op::none:
	default:
		return VK_ATTACHMENT_STORE_OP_NONE;
	}
}

VkPrimitiveTopology GetVKTopology(topology Topology)
{
	switch (Topology)
	{
	case topology::point_list:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case topology::line_list:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case topology::line_strip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case topology::triangle_list:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case topology::triangle_strip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case topology::triangle_fan:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case topology::triangle_list_adjacency:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
	case topology::triangle_strip_adjacency:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
	default:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

VkLogicOp GetVKLogicOp(logic_op Op)
{
	switch (Op)
	{
	case logic_op::clear:
		return VK_LOGIC_OP_CLEAR;
	case logic_op::AND:
		return VK_LOGIC_OP_AND;
	case logic_op::and_reverse:
		return VK_LOGIC_OP_AND_REVERSE;
	case logic_op::copy:
		return VK_LOGIC_OP_COPY;
	case logic_op::and_inverted:
		return VK_LOGIC_OP_AND_INVERTED;
	case logic_op::no_op:
		return VK_LOGIC_OP_NO_OP;
	case logic_op::XOR:
		return VK_LOGIC_OP_XOR;
	case logic_op::OR:
		return VK_LOGIC_OP_OR;
	case logic_op::NOR:
		return VK_LOGIC_OP_NOR;
	case logic_op::equivalent:
		return VK_LOGIC_OP_EQUIVALENT;
	default:
		return VK_LOGIC_OP_CLEAR;
	}
}

VkBlendFactor GetVKBlendFactor(blend_factor Factor)
{
	switch (Factor)
	{

	case blend_factor::zero:
		return VK_BLEND_FACTOR_ZERO;
	case blend_factor::one:
		return VK_BLEND_FACTOR_ONE;
	case blend_factor::src_color:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case blend_factor::one_minus_src_color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case blend_factor::dst_color:
		return VK_BLEND_FACTOR_DST_COLOR;
	case blend_factor::one_minus_dst_color:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case blend_factor::src_alpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case blend_factor::one_minus_src_alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case blend_factor::dst_alpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case blend_factor::one_minus_dst_alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	default:
		return VK_BLEND_FACTOR_ZERO;
	}
}

VkBlendOp GetVKBlendOp(blend_op Op)
{
	switch (Op)
	{
	case blend_op::add:
		return VK_BLEND_OP_ADD;
	case blend_op::subtract:
		return VK_BLEND_OP_SUBTRACT;
	case blend_op::reverse_subtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case blend_op::min:
		return VK_BLEND_OP_MIN;
	case blend_op::max:
		return VK_BLEND_OP_MAX;
	default:
		return VK_BLEND_OP_ADD;
	}
}

u32 GetVKColorComponentFlags(color_component_flags flags)
{
	switch (flags)
	{
	case color_component_flags::R:
		return VK_COLOR_COMPONENT_R_BIT;
	case color_component_flags::G:
		return VK_COLOR_COMPONENT_G_BIT;
	case color_component_flags::B:
		return VK_COLOR_COMPONENT_B_BIT;
	case color_component_flags::A:
		return VK_COLOR_COMPONENT_A_BIT;
	default:
		return VK_COLOR_COMPONENT_R_BIT;
	}
}

VkAttachmentLoadOp GetLoadOp(load_op Op)
{
	switch (Op)
	{
	case load_op::load:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	case load_op::clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case load_op::dont_care:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case load_op::none:
		return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
	default:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
}

VkAttachmentStoreOp GetStoreOp(store_op Op)
{
	switch (Op)
	{
	case store_op::store:
		return VK_ATTACHMENT_STORE_OP_STORE;
	case store_op::dont_care:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case store_op::none:
		return VK_ATTACHMENT_STORE_OP_NONE;
	default:
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
}

VkFilter GetVKFilter(filter Filter)
{
	switch (Filter)
	{
	case filter::linear:
		return VK_FILTER_LINEAR;
	case filter::nearest:
		return VK_FILTER_NEAREST;
	default:
		return VK_FILTER_LINEAR;
	}
}

VkSamplerAddressMode GetVKSamplerAddressMode(sampler_address_mode Mode)
{
	switch (Mode)
	{
	case sampler_address_mode::repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case sampler_address_mode::mirrored_repeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case sampler_address_mode::clamp_to_edge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case sampler_address_mode::clamp_to_border:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case sampler_address_mode::mirror_clamp_to_edge:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

VkSamplerReductionMode GetVKSamplerReductionMode(sampler_reduction_mode Mode)
{
	switch (Mode)
	{
	case sampler_reduction_mode::weighted_average:
		return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
	case sampler_reduction_mode::min:
		return VK_SAMPLER_REDUCTION_MODE_MIN;
	case sampler_reduction_mode::max:
		return VK_SAMPLER_REDUCTION_MODE_MAX;
	default:
		return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
	}
}

VkSamplerMipmapMode GetVKMipmapMode(mipmap_mode Mode)
{
	switch (Mode)
	{
	case mipmap_mode::nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case mipmap_mode::linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

VkBorderColor GetVKBorderColor(border_color Col)
{
	switch (Col)
	{
	case border_color::black_transparent:
		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	case border_color::black_opaque:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	case border_color::white_opaque:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	default:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	}
}

VkPresentModeKHR GetVKPresentMode(vk_sync VSync)
{
	switch (VSync)
	{
	case vk_sync::none:
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	case vk_sync::fifo:
		return VK_PRESENT_MODE_FIFO_KHR;
	case vk_sync::fifo_relaxed:
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	case vk_sync::mailbox:
		return VK_PRESENT_MODE_MAILBOX_KHR;
	default:
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
}

VkImageLayout GetVKLayout(barrier_state State)
{
    switch (State)
    {
	case barrier_state::general:
        return VK_IMAGE_LAYOUT_GENERAL;
    case barrier_state::color_attachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case barrier_state::depth_stencil_attachment:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case barrier_state::shader_read:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case barrier_state::depth_read:
        return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
    case barrier_state::stencil_read:
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    case barrier_state::depth_stencil_read:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case barrier_state::present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case barrier_state::transfer_src:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case barrier_state::transfer_dst:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case barrier_state::undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

bool IsVKAspectSupported(VkPipelineStageFlags StageMask, VkAccessFlags AccessFlag)
{
	switch (AccessFlag) 
	{
		case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		case VK_ACCESS_INDEX_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_UNIFORM_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_SHADER_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_SHADER_WRITE_BIT:
			return StageMask & (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
			return StageMask & (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
			return StageMask & (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		case VK_ACCESS_TRANSFER_READ_BIT:
			return StageMask & VK_PIPELINE_STAGE_TRANSFER_BIT;
		case VK_ACCESS_TRANSFER_WRITE_BIT:
			return StageMask & VK_PIPELINE_STAGE_TRANSFER_BIT;
		case VK_ACCESS_HOST_READ_BIT:
			return StageMask & VK_PIPELINE_STAGE_HOST_BIT;
		case VK_ACCESS_HOST_WRITE_BIT:
			return StageMask & VK_PIPELINE_STAGE_HOST_BIT;
		case VK_ACCESS_MEMORY_READ_BIT:
			return StageMask & (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		case VK_ACCESS_MEMORY_WRITE_BIT:
			return StageMask & (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		default:
			return false;
	}
}

VkAccessFlags GetVKAccessMask(u32 Layouts, u32 Stage)
{
    VkAccessFlags Result = 0;

    if (Layouts & AF_IndirectCommandRead && IsVKAspectSupported(Stage, VK_ACCESS_INDIRECT_COMMAND_READ_BIT)) 
	{
        Result |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }

    if (Layouts & AF_IndexRead && IsVKAspectSupported(Stage, VK_ACCESS_INDEX_READ_BIT)) 
	{
        Result |= VK_ACCESS_INDEX_READ_BIT;
    }

    if (Layouts & AF_VertexAttributeRead && IsVKAspectSupported(Stage, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) 
	{
        Result |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }

    if (Layouts & AF_UniformRead && IsVKAspectSupported(Stage, VK_ACCESS_UNIFORM_READ_BIT)) 
	{
        Result |= VK_ACCESS_UNIFORM_READ_BIT;
    }

    if (Layouts & AF_InputAttachmentRead && IsVKAspectSupported(Stage, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)) 
	{
        Result |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }

    if (Layouts & AF_ShaderRead && IsVKAspectSupported(Stage, VK_ACCESS_SHADER_READ_BIT)) 
	{
        Result |= VK_ACCESS_SHADER_READ_BIT;
    }

    if (Layouts & AF_ShaderWrite && IsVKAspectSupported(Stage, VK_ACCESS_SHADER_WRITE_BIT)) 
	{
        Result |= VK_ACCESS_SHADER_WRITE_BIT;
    }

    if (Layouts & AF_ColorAttachmentRead && IsVKAspectSupported(Stage, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT)) 
	{
        Result |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    }

    if (Layouts & AF_ColorAttachmentWrite && IsVKAspectSupported(Stage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) 
	{
        Result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (Layouts & AF_DepthStencilAttachmentRead && IsVKAspectSupported(Stage, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)) 
	{
        Result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }

    if (Layouts & AF_DepthStencilAttachmentWrite && IsVKAspectSupported(Stage, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) 
	{
        Result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (Layouts & AF_TransferRead && IsVKAspectSupported(Stage, VK_ACCESS_TRANSFER_READ_BIT)) 
	{
        Result |= VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (Layouts & AF_TransferWrite && IsVKAspectSupported(Stage, VK_ACCESS_TRANSFER_WRITE_BIT)) 
	{
        Result |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    if (Layouts & AF_HostRead && IsVKAspectSupported(Stage, VK_ACCESS_HOST_READ_BIT)) 
	{
        Result |= VK_ACCESS_HOST_READ_BIT;
    }

    if (Layouts & AF_HostWrite && IsVKAspectSupported(Stage, VK_ACCESS_HOST_WRITE_BIT)) 
	{
        Result |= VK_ACCESS_HOST_WRITE_BIT;
    }

    if (Layouts & AF_MemoryRead && IsVKAspectSupported(Stage, VK_ACCESS_MEMORY_READ_BIT)) 
	{
        Result |= VK_ACCESS_MEMORY_READ_BIT;
    }

    if (Layouts & AF_MemoryWrite && IsVKAspectSupported(Stage, VK_ACCESS_MEMORY_WRITE_BIT)) 
	{
        Result |= VK_ACCESS_MEMORY_WRITE_BIT;
    }

    return Result;
}

VkPipelineStageFlags GetVKPipelineStage(u32 Stages)
{
	VkPipelineStageFlags Result = {};

	if(Stages & PSF_TopOfPipe)
		Result |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	if(Stages & PSF_DrawIndirect)
		Result |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
	if(Stages & PSF_VertexInput)
		Result |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	if(Stages & PSF_VertexShader)
		Result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	if(Stages & PSF_FragmentShader)
		Result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	if(Stages & PSF_EarlyFragment)
		Result |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	if(Stages & PSF_LateFragment)
		Result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	if(Stages & PSF_ColorAttachment)
		Result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	if(Stages & PSF_Compute)
		Result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	if(Stages & PSF_Transfer)
		Result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
	if(Stages & PSF_BottomOfPipe)
		Result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	if(Stages & PSF_Host)
		Result |= VK_PIPELINE_STAGE_HOST_BIT;
	if(Stages & PSF_AllGraphics)
		Result |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	if(Stages & PSF_AllCommands)
		Result |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	return Result;
}
