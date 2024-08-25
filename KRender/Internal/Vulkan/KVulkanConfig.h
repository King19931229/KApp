#pragma once
#include "vulkan/vulkan.h"
#ifdef __ANDROID__
#include "vulkan/vk_platform.h"
#endif
#include <stdio.h>
#include <assert.h>


// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#define VK_NULL_HANDEL 0

static const char* VK_ERROR_STRING(VkResult errorCode)
{
	switch (errorCode)
	{
#define STR(r) case VK_ ##r: return #r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
default:
	return "UNKNOWN_ERROR";
	}
}

#define VK_ASSERT_RESULT(exp)\
do\
{\
	VkResult result##__LINE__ = (VkResult)(exp);\
	if(result##__LINE__ != VK_SUCCESS)\
	{\
		printf("Vulkan Failure:[%s] File:[%s] Line:[%d]\n", VK_ERROR_STRING(result##__LINE__), __FILE__, __LINE__);\
		assert(false && "Vulkan failure please check");\
	}\
}\
while(false);

#ifdef VK_EXT_DEBUG_UTILS_SPEC_VERSION
#define VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
#endif