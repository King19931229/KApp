#define NOMINMAX
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Internal/KRenderGlobal.h"
#include "KRender/Internal/Vulkan/KVulkanGlobal.h"
#include "KRender/Internal/Vulkan/KVulkanInitializer.h"
#include "KRender/Internal/Vulkan/KVulkanCommandBuffer.h"
#include "KRender/Internal/Vulkan/KVulkanFrameBuffer.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <windows.h>
#undef CreateSemaphore

static void Check(bool result, const char* operation)
{
	if (!result)
	{
		throw std::runtime_error(operation);
	}
}

// Exercises the real resource factories, sampled-image descriptors, command
// buffers and queues with two frame slots and one shared image. No per-frame
// device wait is used: the final graphics fence joins the compute branch.
static void RunCanary(uint32_t frameCount, bool negativeControl)
{
	IKRenderDevice* device = KRenderGlobal::RenderDevice;
	Check(KRenderGlobal::NumComputeQueue > 0, "Dedicated compute family required");
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	std::vector<uint32_t> families = KVulkanGlobal::resourceSharingFamilyIndices;
	KVulkanGlobal::resourceSharingFamilyIndices = { families.front() };
	KVulkanInitializer::SetupResourceSharing(bufferInfo);
	Check(bufferInfo.sharingMode == VK_SHARING_MODE_EXCLUSIVE && bufferInfo.queueFamilyIndexCount == 0, "Single-family fallback");
	KVulkanGlobal::resourceSharingFamilyIndices = families;
	KVulkanInitializer::SetupResourceSharing(bufferInfo);
	KVulkanInitializer::SetupResourceSharing(imageInfo);
	Check(bufferInfo.sharingMode == VK_SHARING_MODE_CONCURRENT && imageInfo.sharingMode == VK_SHARING_MODE_CONCURRENT, "Concurrent resource creation policy");
	Check(imageInfo.queueFamilyIndexCount == families.size(), "All selected queue families are included");

	IKRenderTargetPtr input;
	device->CreateRenderTarget(input);
	Check(input->InitFromStorage(1, 1, 1, EF_R32_FLOAT), "Create shared frame image");
	IKSamplerPtr sampler;
	device->CreateSampler(sampler);
	sampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	Check(sampler->Init(0, 0), "Create nearest sampler");
	IKStorageBufferPtr outputs[2];
	IKComputePipelinePtr pipelines[2];
	std::vector<uint32_t> initial(frameCount * 2, 0);
	for (uint32_t branch = 0; branch < 2; ++branch)
	{
		device->CreateStorageBuffer(outputs[branch]);
		outputs[branch]->InitMemory(initial.size() * sizeof(uint32_t), initial.data());
		Check(outputs[branch]->InitDevice(false, true), "Create branch readback buffer");
		device->CreateComputePipeline(pipelines[branch]);
		pipelines[branch]->BindSampler(0, input->GetFrameBuffer(), sampler, true);
		pipelines[branch]->BindStorageBuffer(1, outputs[branch], COMPUTE_RESOURCE_OUT, true);
		pipelines[branch]->BindDynamicUniformBuffer(2);
		Check(pipelines[branch]->Init("tests/async_compute_canary.comp", KShaderCompileEnvironment(), false), "Compile frame canary shader");
	}

	IKQueuePtr graphics, compute, finalGraphics;
	device->CreateQueue(graphics);
	device->CreateQueue(compute);
	Check(graphics->Init(QUEUE_GRAPHICS, 0), "Graphics queue");
	Check(compute->Init(QUEUE_COMPUTE, 0), "Compute queue");
	device->CreateQueue(finalGraphics);
	// The negative control restores the original split graphics/present queues,
	// allowing a later Pre submission to race the preceding compute consumer.
	Check(finalGraphics->Init(negativeControl ? QUEUE_PRESENT : QUEUE_GRAPHICS, 0), "Final graphics queue");
	IKCommandPoolPtr graphicsPool, computePool;
	device->CreateCommandPool(graphicsPool);
	device->CreateCommandPool(computePool);
	Check(graphicsPool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_POOL), "Graphics command pool");
	Check(computePool->Init(QUEUE_COMPUTE, 0, CBR_RESET_POOL), "Compute command pool");
	IKSemaphorePtr ready, computed;
	device->CreateSemaphore(ready);
	device->CreateSemaphore(computed);
	ready->Init();
	computed->Init();
	IKFencePtr fence;
	device->CreateFence(fence);
	fence->Init(true);

	uint32_t savedFrame = KRenderGlobal::CurrentFrameNum;
	uint32_t savedIndex = KRenderGlobal::CurrentInFlightFrameIndex;
	VkQueryPool timestamps = VK_NULL_HANDLE;
	if (KVulkanGlobal::deviceProperties.limits.timestampComputeAndGraphics)
	{
		VkQueryPoolCreateInfo info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		info.queryCount = frameCount * 4;
		Check(vkCreateQueryPool(KVulkanGlobal::device, &info, nullptr, &timestamps) == VK_SUCCESS, "Create branch timestamp queries");
	}
	KRenderGlobal::ImmediateCommandList.BeginRecord();
	KRenderGlobal::ImmediateCommandList.Transition(input->GetFrameBuffer(), PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
	KRenderGlobal::ImmediateCommandList.EndRecord();
	std::printf("[AsyncComputeTest] canary begin: %u frames, graphics family %u, compute family %u\n", frameCount, KVulkanGlobal::graphicsFamilyIndices[0], KVulkanGlobal::computeFamilyIndices[0]);
	for (uint32_t frame = 0; frame < frameCount; ++frame)
	{
		KRenderGlobal::CurrentFrameNum = frame + 1;
		KRenderGlobal::CurrentInFlightFrameIndex = frame % KRenderGlobal::NumFramesInFlight;
		Check(fence->Wait(), "Wait for reusable frame slot");
		fence->Reset();
		graphicsPool->Reset();
		computePool->Reset();
		IKCommandBufferPtr pre = graphicsPool->Request(CBL_PRIMARY);
		pre->BeginPrimary();
		pre->PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_ALL_COMMANDS);
		if (timestamps)
		{
			vkCmdResetQueryPool(static_cast<KVulkanCommandBuffer*>(pre.get())->GetVkHandle(), timestamps, frame * 4, 4);
		}
		pre->Transition(input->GetFrameBuffer(), PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_TRANSFER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_TRANSFER_DST);
		VkClearColorValue clear = {};
		clear.float32[0] = (float)(frame + 1);
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage(static_cast<KVulkanCommandBuffer*>(pre.get())->GetVkHandle(), static_cast<KVulkanFrameBuffer*>(input->GetFrameBuffer().get())->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
		pre->Transition(input->GetFrameBuffer(), PIPELINE_STAGE_TRANSFER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_SHADER_READ_ONLY);
		pre->End();

		struct FrameInfo
		{
			uint32_t frameIndex, iterations, padding[2];
		} data = { frame, frame % 2 ? 50000u : 1000u, {} };
		KDynamicConstantBufferUsage usage;
		usage.binding = 2;
		usage.range = sizeof(data);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&data, usage);
		FrameInfo graphicsData = { frame, frame % 2 ? 1000u : 50000u, {} };
		KDynamicConstantBufferUsage graphicsUsage;
		graphicsUsage.binding = 2;
		graphicsUsage.range = sizeof(graphicsData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&graphicsData, graphicsUsage);
		IKCommandBufferPtr ao = computePool->Request(CBL_PRIMARY);
		ao->BeginPrimary();
		if (timestamps)
		{
			vkCmdWriteTimestamp(static_cast<KVulkanCommandBuffer*>(ao.get())->GetVkHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamps, frame * 4);
		}
		Check(pipelines[0]->Execute(ao, 1, 1, 1, &usage), "Record compute branch");
		if (timestamps)
		{
			vkCmdWriteTimestamp(static_cast<KVulkanCommandBuffer*>(ao.get())->GetVkHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamps, frame * 4 + 1);
		}
		ao->End();
		IKCommandBufferPtr parallel = graphicsPool->Request(CBL_PRIMARY);
		parallel->BeginPrimary();
		if (timestamps)
		{
			vkCmdWriteTimestamp(static_cast<KVulkanCommandBuffer*>(parallel.get())->GetVkHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamps, frame * 4 + 2);
		}
		Check(pipelines[1]->Execute(parallel, 1, 1, 1, &graphicsUsage), "Record independent graphics-queue branch");
		if (timestamps)
		{
			vkCmdWriteTimestamp(static_cast<KVulkanCommandBuffer*>(parallel.get())->GetVkHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamps, frame * 4 + 3);
		}
		parallel->End();
		IKCommandBufferPtr post = graphicsPool->Request(CBL_PRIMARY);
		post->BeginPrimary();
		post->PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_ALL_COMMANDS);
		post->End();
		graphics->Submit(pre, {}, { ready }, nullptr);
		compute->Submit(ao, { { ready, PIPELINE_STAGE_ALL_COMMANDS } }, { computed }, nullptr);
		graphics->Submit(parallel, {}, {}, nullptr);
		finalGraphics->Submit(post, { { computed, PIPELINE_STAGE_ALL_COMMANDS } }, {}, fence);
	}
	device->Wait();
	if (timestamps)
	{
		std::vector<uint64_t> times(frameCount * 4);
		Check(vkGetQueryPoolResults(KVulkanGlobal::device, timestamps, 0, frameCount * 4, times.size() * sizeof(uint64_t), times.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS, "Read branch timestamps");
		uint32_t overlapping = 0;
		for (uint32_t frame = 0; frame < frameCount; ++frame)
		{
			uint64_t* t = times.data() + frame * 4;
			overlapping += t[0] < t[3] && t[2] < t[1];
		}
		std::printf("[AsyncComputeTest] GPU timestamp intervals overlap in %u/%u frames\n", overlapping, frameCount);
		vkDestroyQueryPool(KVulkanGlobal::device, timestamps, nullptr);
	}
	bool valid = true;
	for (uint32_t branch = 0; branch < 2; ++branch)
	{
		std::vector<uint32_t> result(initial.size());
		Check(outputs[branch]->Read(result.data()), "Read branch results");
		for (uint32_t frame = 0; frame < frameCount; ++frame)
		{
			uint32_t hash = frame;
			uint32_t iterations = ((frame + branch) % 2) ? 50000u : 1000u;
			for (uint32_t i = 0; i < iterations; ++i)
			{
				hash = hash * 1664525u + 1013904223u;
			}
			if (result[2 * frame] != frame + 1 || result[2 * frame + 1] != hash)
			{
				if (valid)
				{
					std::printf("[AsyncComputeTest] mismatch: branch=%u frame=%u expected=%u actual=%u\n", branch, frame, frame + 1, result[2 * frame]);
				}
				valid = false;
			}
		}
		pipelines[branch]->UnInit();
		outputs[branch]->UnInit();
	}
	fence->UnInit();
	ready->UnInit();
	computed->UnInit();
	computePool->UnInit();
	graphicsPool->UnInit();
	compute->UnInit();
	graphics->UnInit();
	finalGraphics->UnInit();
	sampler->UnInit();
	input->UnInit();
	KRenderGlobal::CurrentFrameNum = savedFrame;
	KRenderGlobal::CurrentInFlightFrameIndex = savedIndex;
	Check(valid, "Frame canary mismatch: shared image overwritten or UBO reused early");
	std::printf("[AsyncComputeTest] canary PASS: %u frame values and UBO checksums per branch\n", frameCount);
}

int main(int argc, char** argv)
{
	int result = 0;
	try
	{
		KRenderGlobal::EnableAsyncShaderCompile = argc < 3;
		KEngineGlobal::CreateEngine();
		IKEnginePtr engine = KEngineGlobal::Engine;
		KEngineOptions options;
		options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;
		options.window.width = 640;
		options.window.height = 360;
		options.window.resizable = true;
		IKRenderWindowPtr windowOwner = CreateRenderWindow(RENDER_WINDOW_GLFW);
		IKRenderWindow* window = windowOwner.get();
		Check(engine->Init(std::move(windowOwner), options), "Engine initialization");
		ShowWindow(static_cast<HWND>(window->GetHWND()), SW_HIDE);
		engine->Wait();
		bool negativeControl = argc > 1 && std::strcmp(argv[1], "--negative-control") == 0;
		RunCanary(1000, negativeControl);
		Check(!negativeControl, "Negative control did not reproduce the original cross-frame race");
		if (argc > 1)
		{
			Check(engine->GetScene()->Load(argv[1]), "Load smoke-test scene");
		}
		std::atomic<bool> done(false);
		std::atomic<uint32_t> resize(0);
		std::atomic<uint32_t> actions(0);
		IKRenderWindowPtr secondary;
		uint32_t frames = 0;
		uint32_t computeCount = KRenderGlobal::NumComputeQueue;
		glm::vec3 cameraPosition = engine->GetRenderCore()->GetCamera()->GetPosition();
		KRenderCoreUIRenderCallback tick = [&]()
		{
			uint32_t frame = frames++;
			if (frame % 128 == 0)
			{
				KRenderGlobal::EnableAsyncCompute = (frame / 128) % 2 != 0;
				std::printf("[AsyncComputeTest] scene frame %u, AsyncCompute=%d\n", frame, KRenderGlobal::EnableAsyncCompute);
			}
			KRenderGlobal::RTAO.GetEnable() = frame < 256 || frame >= 384;
			KRenderGlobal::ScreenSpaceReflection.GetEnable() = frame < 512 || frame >= 640;
			KRenderGlobal::EnableRHIImmediate = frame >= 640 && frame < 768;
			if (frame >= 100 && frame <= 200)
			{
				engine->GetRenderCore()->GetCamera()->SetPosition(cameraPosition + glm::vec3(0.01f * (frame - 100), 0, 0));
			}
			if (frame == 200) actions.fetch_or(1);
			if (frame == 480) actions.fetch_or(2);
			if (frame == 800)
			{
				KRenderGlobal::EnableAsyncCompute = true;
				actions.fetch_or(4);
			}
			if (frame == 864) actions.fetch_or(8);
			if (frame == 320 || frame == 720)
			{
				resize.store(frame);
			}
			if (frame == 896)
			{
				KRenderGlobal::EnableAsyncCompute = false;
			}
			if (frame == 897)
			{
				KRenderGlobal::NumComputeQueue = 0;
				KRenderGlobal::EnableAsyncCompute = true;
			}
			if (frame == 960)
			{
				KRenderGlobal::EnableAsyncCompute = false;
				KRenderGlobal::NumComputeQueue = computeCount;
			}
			if (frame == 961)
			{
				KRenderGlobal::EnableAsyncCompute = true;
			}
			if (frame >= 1024)
			{
				done.store(true);
			}
		};
		engine->GetRenderCore()->RegisterUIRenderCallback(&tick);
		auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(5);
		while (!done.load() && !engine->GetRenderCore()->TickShouldEnd())
		{
			Check(std::chrono::steady_clock::now() < deadline, "Scene smoke test timeout");
			GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
			uint32_t request = resize.exchange(0);
			uint32_t action = actions.exchange(0);
			if (action)
			{
				engine->Wait();
				if ((action & 1) && !engine->GetScene()->GetEntities().empty())
				{
					IKEntityPtr entity = *engine->GetScene()->GetEntities().begin();
					IKComponentBase* component = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, &component))
					{
						IKTransformComponent* transform = static_cast<IKTransformComponent*>(component);
						transform->SetPosition(transform->GetPosition() + glm::vec3(1, 0, 0));
						engine->GetScene()->Transform(entity);
					}
				}
				if (action & 2)
				{
					ShowWindow(static_cast<HWND>(window->GetHWND()), SW_MINIMIZE);
					for (uint32_t i = 0; i < 3; ++i)
					{
						GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
						engine->Tick();
						std::this_thread::sleep_for(std::chrono::milliseconds(16));
					}
					engine->Wait();
					ShowWindow(static_cast<HWND>(window->GetHWND()), SW_RESTORE);
					ShowWindow(static_cast<HWND>(window->GetHWND()), SW_HIDE);
				}
				if (action & 4)
				{
					secondary = CreateRenderWindow(RENDER_WINDOW_GLFW);
					secondary->SetRenderDevice(engine->GetRenderCore()->GetRenderDevice());
					Check(secondary->Init(0, 0, 640, 360, true, false), "Initialize secondary window");
					ShowWindow(static_cast<HWND>(secondary->GetHWND()), SW_HIDE);
					Check(engine->GetRenderCore()->RegisterSecordaryWindow(secondary), "Register secondary window");
				}
				if ((action & 8) && secondary)
				{
					Check(engine->GetRenderCore()->UnRegisterSecordaryWindow(secondary), "Unregister secondary window");
				}
			}
			if (request)
			{
				engine->Wait();
				window->SetSize(request == 320 ? 800 : 640, request == 320 ? 450 : 360);
			}
			engine->Tick();
		}
		engine->Wait();
		engine->GetRenderCore()->UnRegisterUIRenderCallback(&tick);
		Check(done.load(), "Window closed before smoke-test completion");
		if (secondary)
		{
			engine->GetRenderCore()->UnRegisterSecordaryWindow(secondary);
		}
		std::printf("[AsyncComputeTest] scene PASS: %u frames; toggles, RHI modes, camera/object motion, resize, minimize, secondary window and compute fallback\n", frames);
		engine->UnInit();
		engine = nullptr;
		window = nullptr;
		KEngineGlobal::DestroyEngine();
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "[AsyncComputeTest] FAIL: %s\n", error.what());
		if (KEngineGlobal::Engine)
		{
			KEngineGlobal::Engine->Wait();
			KEngineGlobal::Engine->UnInit();
			KEngineGlobal::DestroyEngine();
		}
		result = 1;
	}
	return result;
}
