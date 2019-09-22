#pragma once
#include "Vulkan/vulkan.h"
#include <assert.h>

#define VK_ASSERT_RESULT(exp)\
do\
{\
	VkResult result = (VkResult)(exp);\
	if(result != VK_SUCCESS)\
	{\
		assert(false && "Vulkan failure");\
	}\
}\
while(false);