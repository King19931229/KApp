#pragma once

#include "KVulkanConfig.h"
#include "KVulkanPipeline.h"
#include "KVulkanComputePipeline.h"

namespace KVulkanDebug
{
	struct RunningContext
	{
		struct GraphicsContext
		{
			KVulkanPipeline* pipeline = nullptr;
		};
		struct ComputeContext
		{
			KVulkanComputePipeline* pipeline = nullptr;
		};
		GraphicsContext graphics;
		ComputeContext compute;

		bool graphicsRunning = false;
		bool computeRunning = false;

		void DebugDump()
		{
			if (graphicsRunning && graphics.pipeline)
			{
				graphics.pipeline->DebugDump();
			}
			if (computeRunning && compute.pipeline)
			{
				compute.pipeline->DebugDump();
			}
		}
	};

	extern RunningContext Context;

	struct GraphicsScope
	{
		GraphicsScope(KVulkanPipeline* pipeline)
		{
			Context.graphics.pipeline = pipeline;
			Context.graphicsRunning = true;
		}
		~GraphicsScope()
		{
			Context.graphics.pipeline = nullptr;
			Context.graphicsRunning = false;
		}
	};
	struct ComputeScope
	{
		ComputeScope(KVulkanComputePipeline* pipeline)
		{
			Context.compute.pipeline = pipeline;
			Context.computeRunning = true;
		}
		~ComputeScope()
		{
			Context.compute.pipeline = nullptr;
			Context.computeRunning = false;
		}
	};
}

#define KVULKAN_GRAPHICS_SCOPE(pipeline) KVulkanDebug::GraphicsScope scope_##__FILE__##_##__LINE__(pipeline);
#define KVULKAN_COMPUTE_SCOPE(pipeline) KVulkanDebug::ComputeScope scope_##__FILE__##_##__LINE__(pipeline);