#include "KRenderDispatcher.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"

KRenderDispatcher::KRenderDispatcher()
	: m_Device(nullptr),
	m_SwapChain(nullptr),
	m_UIOverlay(nullptr),
	m_MaxRenderThreadNum(std::thread::hardware_concurrency()),
	m_FrameInFlight(0),
	m_MultiThreadSumbit(true)
{
}

KRenderDispatcher::~KRenderDispatcher()
{
	ASSERT_RESULT(m_Device == nullptr);
	ASSERT_RESULT(m_SwapChain == nullptr);
	ASSERT_RESULT(m_UIOverlay == nullptr);
	ASSERT_RESULT(m_CommandBuffers.empty());
}

bool KRenderDispatcher::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_FrameInFlight);

	size_t numThread = m_ThreadPool.GetWorkerThreadNum();

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		m_Device->CreateCommandPool(m_CommandBuffers[i].commandPool);
		m_CommandBuffers[i].commandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].primaryCommandBuffer);
		m_CommandBuffers[i].primaryCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_PRIMARY);

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].skyBoxCommandBuffer);
		m_CommandBuffers[i].skyBoxCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].shadowMapCommandBuffer);
		m_CommandBuffers[i].shadowMapCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

		m_CommandBuffers[i].threadDatas.resize(numThread);

		// 创建线程命令缓冲与命令池
		for (size_t threadIdx = 0; threadIdx < numThread; ++threadIdx)
		{
			ThreadData& threadData = m_CommandBuffers[i].threadDatas[threadIdx];

			m_Device->CreateCommandPool(threadData.commandPool);
			threadData.commandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

			m_Device->CreateCommandBuffer(threadData.preZcommandBuffer);
			threadData.preZcommandBuffer->Init(threadData.commandPool, CBL_SECONDARY);

			m_Device->CreateCommandBuffer(threadData.commandBuffer);
			threadData.commandBuffer->Init(threadData.commandPool, CBL_SECONDARY);
		}
	}
	return true;
}

bool KRenderDispatcher::DestroyCommandBuffers()
{
	// clear command buffers
	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		for (ThreadData& thread : m_CommandBuffers[i].threadDatas)
		{
			thread.preZcommandBuffer->UnInit();
			thread.preZcommandBuffer = nullptr;

			thread.commandBuffer->UnInit();
			thread.commandBuffer = nullptr;

			thread.commandPool->UnInit();
			thread.commandPool = nullptr;
		}

		m_CommandBuffers[i].primaryCommandBuffer->UnInit();
		m_CommandBuffers[i].skyBoxCommandBuffer->UnInit();
		m_CommandBuffers[i].shadowMapCommandBuffer->UnInit();

		m_CommandBuffers[i].commandPool->UnInit();
		m_CommandBuffers[i].commandPool = nullptr;
	}

	m_CommandBuffers.clear();

	return true;
}

void KRenderDispatcher::ThreadRenderObject(uint32_t frameIndex, uint32_t threadIndex)
{
	IKPipelineHandlePtr pipelineHandle;

	ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];

	// https://devblogs.nvidia.com/vulkan-dos-donts/ ResetCommandPool释放内存
	threadData.commandPool->Reset();

	IKCommandBufferPtr commandBuffer = nullptr;
	// PreZ
	commandBuffer = threadData.preZcommandBuffer;

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	commandBuffer->BeginSecondary(offscreenTarget);
	commandBuffer->SetViewport(offscreenTarget);

	for (KRenderCommand& command : threadData.preZcommands)
	{
		commandBuffer->Render(command);
	}
	commandBuffer->End();

	// Regular
	commandBuffer = threadData.commandBuffer;

	commandBuffer->BeginSecondary(offscreenTarget);
	commandBuffer->SetViewport(offscreenTarget);

	for (KRenderCommand& command : threadData.commands)
	{
		commandBuffer->Render(command);
	}

	commandBuffer->End();
}

bool KRenderDispatcher::SubmitCommandBufferSingleThread(KScene* scene, KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle = nullptr;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	IKCommandBufferPtr primaryBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;

	primaryBuffer->BeginPrimary();
	{
		primaryBuffer->BeginRenderPass(KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex), SUBPASS_CONTENTS_INLINE);
		{
			KRenderGlobal::ShadowMap.UpdateShadowMap(m_Device, primaryBuffer.get(), frameIndex);
		}
		primaryBuffer->EndRenderPass();

		primaryBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_INLINE);
		{
			primaryBuffer->SetViewport(offscreenTarget);
			// 开始渲染SkyBox
			{
				KRenderCommand command;
				if (KRenderGlobal::SkyBox.GetRenderCommand(frameIndex, command))
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, false);
					command.pipelineHandle = handle;
					primaryBuffer->Render(command);
				}
			}

			// 开始渲染物件
			{
				KRenderCommandList preZCommandList;
				KRenderCommandList commandList;

				std::vector<KRenderComponent*> cullRes;
				scene->GetRenderComponent(*camera, cullRes);

				for (KRenderComponent* component : cullRes)
				{
					KEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, (KComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());
							preZCommandList.push_back(std::move(command));
						});

						mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());
							commandList.push_back(std::move(command));
						});

						KDebugComponent* debug = nullptr;
						if (entity->GetComponent(CT_DEBUG, (KComponentBase**)&debug))
						{
							KConstantDefinition::DEBUG objectData;
							objectData.MODEL = transform->FinalTransform();
							objectData.COLOR = debug->Color();

							mesh->Visit(PIPELINE_STAGE_DEBUG_TRIANGLE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);
								commandList.push_back(std::move(command));
							});

							mesh->Visit(PIPELINE_STAGE_DEBUG_LINE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);
								commandList.push_back(std::move(command));
							});
						}
					}
				}

				for (KRenderCommand& command : preZCommandList)
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, true);
					command.pipelineHandle = handle;
					primaryBuffer->Render(command);
				}

				for (KRenderCommand& command : commandList)
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, true);
					command.pipelineHandle = handle;
					primaryBuffer->Render(command);
				}
			}
		}
		primaryBuffer->EndRenderPass();
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, frameIndex, m_SwapChain, m_UIOverlay, primaryBuffer);

	primaryBuffer->End();

	return true;
}

bool KRenderDispatcher::SubmitCommandBufferMuitiThread(KScene* scene, KCamera* camera,
	uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);
	IKRenderTargetPtr swapChainTarget = m_SwapChain->GetRenderTarget(chainImageIndex);
	IKRenderTargetPtr shadowMapTarget = KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex);

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	IKCommandBufferPtr skyBoxCommandBuffer = m_CommandBuffers[frameIndex].skyBoxCommandBuffer;
	IKCommandBufferPtr shadowMapCommandBuffer = m_CommandBuffers[frameIndex].shadowMapCommandBuffer;

	// 开始渲染过程
	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			{
				primaryCommandBuffer->BeginRenderPass(shadowMapTarget, SUBPASS_CONTENTS_SECONDARY);
				{
					shadowMapCommandBuffer->BeginSecondary(shadowMapTarget);
					KRenderGlobal::ShadowMap.UpdateShadowMap(m_Device, shadowMapCommandBuffer.get(), frameIndex);
					shadowMapCommandBuffer->End();
				}
				primaryCommandBuffer->Execute(shadowMapCommandBuffer);
				primaryCommandBuffer->EndRenderPass();
			}
		}
		// 物件绘制RenderPass
		{
			primaryCommandBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_SECONDARY);

			auto commandBuffers = m_CommandBuffers[frameIndex].commandBuffersExec;
			commandBuffers.clear();

			// 绘制SkyBox
			{
				skyBoxCommandBuffer->BeginSecondary(offscreenTarget);
				skyBoxCommandBuffer->SetViewport(offscreenTarget);

				KRenderCommand command;
				if (KRenderGlobal::SkyBox.GetRenderCommand(frameIndex, command))
				{
					IKPipelineHandlePtr handle;
					KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, false);
					command.pipelineHandle = handle;
					skyBoxCommandBuffer->Render(command);
				}
				skyBoxCommandBuffer->End();

				commandBuffers.push_back(skyBoxCommandBuffer);
			}

			size_t threadCount = m_ThreadPool.GetWorkerThreadNum();

			std::vector<KRenderComponent*> cullRes;
			scene->GetRenderComponent(*camera, cullRes);

			size_t drawEachThread = cullRes.size() / threadCount;
			size_t reaminCount = cullRes.size() % threadCount;

			for (size_t i = 0; i < threadCount; ++i)
			{
				ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[i];

				threadData.preZcommands.clear();
				threadData.commands.clear();

				auto addMesh = [&](size_t index)
				{
					KRenderComponent* component = cullRes[index];
					KMeshPtr mesh = component->GetMesh();

					KEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, (KComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());

							IKPipelineHandlePtr handle;
							KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, true);
							command.pipelineHandle = handle;
							threadData.preZcommands.push_back(std::move(command));
						});

						mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());

							IKPipelineHandlePtr handle;
							KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, true);
							command.pipelineHandle = handle;

							threadData.commands.push_back(std::move(command));
						});

						KDebugComponent* debug = nullptr;
						if (entity->GetComponent(CT_DEBUG, (KComponentBase**)&debug))
						{
							KConstantDefinition::DEBUG objectData;
							objectData.MODEL = transform->FinalTransform();
							objectData.COLOR = debug->Color();

							mesh->Visit(PIPELINE_STAGE_DEBUG_TRIANGLE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);

								IKPipelineHandlePtr handle;
								KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, true);
								command.pipelineHandle = handle;

								threadData.commands.push_back(std::move(command));
							});

							mesh->Visit(PIPELINE_STAGE_DEBUG_LINE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);

								IKPipelineHandlePtr handle;
								KRenderGlobal::PipelineManager.GetPipelineHandle(command.pipeline, offscreenTarget, handle, true);
								command.pipelineHandle = handle;

								threadData.commands.push_back(std::move(command));
							});
						}
					}
				};

				for (size_t st = i * drawEachThread, ed = st + drawEachThread; st < ed; ++st)
				{
					addMesh(st);
				}

				if (i == threadCount - 1)
				{
					for (size_t st = threadCount * drawEachThread, ed = st + reaminCount; st < ed; ++st)
					{
						addMesh(st);
					}
				}
			}

			for (size_t i = 0; i < threadCount; ++i)
			{
				m_ThreadPool.SubmitTask([=]()
				{
					ThreadRenderObject(frameIndex, (uint32_t)i);
				});
			}
			m_ThreadPool.WaitAllAsyncTaskDone();

			for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
			{
				ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];
				if (!threadData.preZcommands.empty())
				{
					commandBuffers.push_back(threadData.preZcommandBuffer);
					threadData.preZcommands.clear();
				}
			}

			for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
			{
				ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];
				if (!threadData.commands.empty())
				{
					commandBuffers.push_back(threadData.commandBuffer);
					threadData.commands.clear();
				}
			}

			if (!commandBuffers.empty())
			{
				primaryCommandBuffer->ExecuteAll(commandBuffers);
			}

			primaryCommandBuffer->EndRenderPass();
		}
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, frameIndex, m_SwapChain, m_UIOverlay, primaryCommandBuffer);

	primaryCommandBuffer->End();

	return true;
}

bool KRenderDispatcher::Init(IKRenderDevice* device, uint32_t frameInFlight, IKSwapChainPtr swapChain, IKUIOverlayPtr uiOverlay)
{
	m_Device = device;
	m_FrameInFlight = frameInFlight;
	m_SwapChain = swapChain;
	m_UIOverlay = uiOverlay;
	m_ThreadPool.Init(m_MaxRenderThreadNum);
	CreateCommandBuffers();
	return true;
}

bool KRenderDispatcher::UnInit()
{
	m_Device = nullptr;
	m_SwapChain = nullptr;
	m_UIOverlay = nullptr;
	m_ThreadPool.UnInit();
	DestroyCommandBuffers();
	return true;
}

bool KRenderDispatcher::ResetSwapChain(IKSwapChainPtr swapChain, IKUIOverlayPtr uiOverlay)
{
	m_SwapChain = swapChain;
	m_UIOverlay = uiOverlay;
	return true;
}

bool KRenderDispatcher::Execute(KScene* scene, KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex)
{
	if (m_MultiThreadSumbit)
	{
		SubmitCommandBufferMuitiThread(scene, camera, chainImageIndex, frameIndex);
	}
	else
	{
		SubmitCommandBufferSingleThread(scene, camera, chainImageIndex, frameIndex);
	}

	return true;
}

IKCommandBufferPtr KRenderDispatcher::GetPrimaryCommandBuffer(uint32_t frameIndex)
{
	if (frameIndex < m_FrameInFlight)
		return m_CommandBuffers[frameIndex].primaryCommandBuffer;
	else
		return nullptr;
}