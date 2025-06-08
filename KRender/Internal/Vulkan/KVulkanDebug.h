#pragma once

#include "KVulkanConfig.h"
#include "KVulkanPipeline.h"
#include "KVulkanComputePipeline.h"

namespace KVulkanDebug
{
	struct GraphicsContext
	{
		KVulkanPipeline* pipeline;
	};
	struct ComputeContext
	{
		KVulkanComputePipeline* pipeline;
	};

	extern GraphicsContext RunningGraphics;
	extern ComputeContext RunningCompute;

	struct GraphicsScope
	{
		GraphicsScope(KVulkanPipeline* pipeline)
		{
			RunningGraphics.pipeline = pipeline;
		}
		~GraphicsScope()
		{
			RunningGraphics.pipeline = nullptr;
		}
	};
	struct ComputeScope
	{
		ComputeScope(KVulkanComputePipeline* pipeline)
		{
			RunningCompute.pipeline = pipeline;
		}
		~ComputeScope()
		{
			RunningCompute.pipeline = nullptr;
		}
	};
}

#define KVULKAN_GRAPHICS_SCOPE(pipeline) KVulkanDebug::GraphicsScope scope_##__FILE__##_##__LINE__(pipeline);
#define KVULKAN_COMPUTE_SCOPE(pipeline) KVulkanDebug::ComputeScope scope_##__FILE__##_##__LINE__(pipeline);