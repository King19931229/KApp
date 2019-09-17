#pragma once
#include "Vulkan/vulkan.h"
#include <exception>

#define VK_ASSERT_RESULT(result)\
do\
{\
	if(result != VK_SUCCESS)\
	{\
		std::exception("vulkan result fail");\
	}\
}\
while(false);