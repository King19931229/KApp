#include "KRenderDispatcher.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/Gizmo/KCameraCube.h"

KRenderDispatcher::KRenderDispatcher()
	: m_Device(nullptr),
	m_SwapChain(nullptr),
	m_UIOverlay(nullptr),
	m_CameraCube(nullptr),
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
	ASSERT_RESULT(m_CameraCube == nullptr);
	ASSERT_RESULT(m_CommandBuffers.empty());
}

static const char* PRE_Z_STAGE = "PreZ";
static const char* DEFAULT_STAGE = "Default";
static const char* DEBUG_STAGE = "Debug";

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

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].gizmoCommandBuffer);
		m_CommandBuffers[i].gizmoCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].clearCommandBuffer);
		m_CommandBuffers[i].clearCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

		m_CommandBuffers[i].threadDatas.resize(numThread);

		// 创建线程命令缓冲与命令池
		for (size_t threadIdx = 0; threadIdx < numThread; ++threadIdx)
		{
			ThreadData& threadData = m_CommandBuffers[i].threadDatas[threadIdx];

			m_Device->CreateCommandPool(threadData.commandPool);
			threadData.commandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

			m_Device->CreateCommandBuffer(threadData.preZcommandBuffer);
			threadData.preZcommandBuffer->Init(threadData.commandPool, CBL_SECONDARY);

			m_Device->CreateCommandBuffer(threadData.defaultCommandBuffer);
			threadData.defaultCommandBuffer->Init(threadData.commandPool, CBL_SECONDARY);

			m_Device->CreateCommandBuffer(threadData.debugCommandBuffer);
			threadData.debugCommandBuffer->Init(threadData.commandPool, CBL_SECONDARY);
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

			thread.defaultCommandBuffer->UnInit();
			thread.defaultCommandBuffer = nullptr;

			thread.debugCommandBuffer->UnInit();
			thread.debugCommandBuffer = nullptr;

			thread.commandPool->UnInit();
			thread.commandPool = nullptr;
		}

		m_CommandBuffers[i].primaryCommandBuffer->UnInit();
		m_CommandBuffers[i].skyBoxCommandBuffer->UnInit();
		m_CommandBuffers[i].shadowMapCommandBuffer->UnInit();
		m_CommandBuffers[i].gizmoCommandBuffer->UnInit();
		m_CommandBuffers[i].clearCommandBuffer->UnInit();

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

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	auto beginSecondary = [offscreenTarget](IKCommandBufferPtr buffer, const std::vector<KRenderCommand>& commands)
	{
		buffer->BeginSecondary(offscreenTarget);
		buffer->SetViewport(offscreenTarget);

		for (const KRenderCommand& command : commands)
		{
			buffer->Render(command);
		}
		buffer->End();
	};

	beginSecondary(threadData.preZcommandBuffer, threadData.preZcommands);
	beginSecondary(threadData.defaultCommandBuffer, threadData.defaultCommands);
	beginSecondary(threadData.debugCommandBuffer, threadData.debugCommands);
}

void KRenderDispatcher::ClearDepthStencil(IKCommandBufferPtr buffer, IKRenderTargetPtr target, const KClearDepthStencil& value)
{
	KClearRect rect;

	size_t width = 0;
	size_t height = 0;
	target->GetSize(width, height);

	rect.width = static_cast<uint32_t>(width);
	rect.height = static_cast<uint32_t>(height);

	buffer->ClearDepthStencil(rect, value);
}

bool KRenderDispatcher::SubmitCommandBufferSingleThread(KRenderScene* scene, KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex)
{
	KRenderStageStatistics preZStatistics;
	KRenderStageStatistics defaultStatistics;
	KRenderStageStatistics debugStatistics;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	IKCommandBufferPtr primaryBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;

	primaryBuffer->BeginPrimary();
	{
		KClearValue clearValue = { {0,0,0,0}, {1, 0} };

		// 更新阴影
		primaryBuffer->BeginRenderPass(KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex), SUBPASS_CONTENTS_INLINE, clearValue);
		{
			KRenderGlobal::ShadowMap.UpdateShadowMap(m_Device, primaryBuffer.get(), frameIndex);
		}
		primaryBuffer->EndRenderPass();

		primaryBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_INLINE, clearValue);
		{
			primaryBuffer->SetViewport(offscreenTarget);

			// 开始渲染SkyBox
			{
				KRenderCommand command;
				if (KRenderGlobal::SkyBox.GetRenderCommand(frameIndex, command))
				{
					IKPipelineHandlePtr handle = nullptr;
					if (command.pipeline->GetHandle(offscreenTarget, handle))
					{
						command.pipelineHandle = handle;
						primaryBuffer->Render(command);
					}
				}
			}

			// 开始渲染Camera Gizmo
			{
				if (m_CameraCube)
				{
					KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
					KRenderCommandList commandList;
					cameraCube->GetRenderCommand(frameIndex, commandList);
					for (KRenderCommand& command : commandList)
					{
						IKPipelineHandlePtr handle = nullptr;
						if (command.pipeline->GetHandle(offscreenTarget, handle))
						{
							command.pipelineHandle = handle;
							primaryBuffer->Render(command);
						}
					}
				}
			}

			// 开始渲染物件
			{
				KRenderCommandList preZCommandList;
				KRenderCommandList defaultCommandList;
				KRenderCommandList debugCommandList;

				std::vector<KRenderComponent*> cullRes;
				scene->GetRenderComponent(*camera, cullRes);

				for (KRenderComponent* component : cullRes)
				{
					IKEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, &transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());

							++preZStatistics.drawcalls;
							if (command.indexDraw)
							{
								preZStatistics.faces += command.indexData->indexCount / 3;
							}
							else
							{
								preZStatistics.faces += command.vertexData->vertexCount / 3;
							}

							preZCommandList.push_back(std::move(command));
						});

						mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());

							++defaultStatistics.drawcalls;
							if (command.indexDraw)
							{
								defaultStatistics.faces += command.indexData->indexCount / 3;
								defaultStatistics.primtives += command.indexData->indexCount;
							}
							else
							{
								defaultStatistics.faces += command.vertexData->vertexCount / 3;
								defaultStatistics.primtives += command.vertexData->vertexCount;
							}

							defaultCommandList.push_back(std::move(command));
						});

						KDebugComponent* debug = nullptr;
						if (entity->GetComponent(CT_DEBUG, (IKComponentBase**)&debug))
						{
							KConstantDefinition::DEBUG objectData;
							objectData.MODEL = transform->FinalTransform();
							objectData.COLOR = debug->Color();

							mesh->Visit(PIPELINE_STAGE_DEBUG_TRIANGLE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);

								++debugStatistics.drawcalls;
								if (command.indexDraw)
								{
									debugStatistics.faces += command.indexData->indexCount / 3;
									debugStatistics.primtives += command.indexData->indexCount;
								}
								else
								{
									debugStatistics.faces += command.vertexData->vertexCount / 3;
									debugStatistics.primtives += command.vertexData->vertexCount;
								}

								debugCommandList.push_back(std::move(command));
							});

							mesh->Visit(PIPELINE_STAGE_DEBUG_LINE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);

								++debugStatistics.drawcalls;
								if (command.indexDraw)
								{
									debugStatistics.faces += command.indexData->indexCount / 2;
									debugStatistics.primtives += command.indexData->indexCount;
								}
								else
								{
									debugStatistics.faces += command.vertexData->vertexCount / 2;
									debugStatistics.primtives += command.vertexData->vertexCount;
								}

								debugCommandList.push_back(std::move(command));
							});
						}
					}
				}

				for (KRenderCommand& command : preZCommandList)
				{
					IKPipelineHandlePtr handle = nullptr;
					if (command.pipeline->GetHandle(offscreenTarget, handle))
					{
						command.pipelineHandle = handle;
						primaryBuffer->Render(command);
					}
				}

				for (KRenderCommand& command : defaultCommandList)
				{
					IKPipelineHandlePtr handle = nullptr;
					if (command.pipeline->GetHandle(offscreenTarget, handle))
					{
						command.pipelineHandle = handle;
						primaryBuffer->Render(command);
					}
				}

				if (!debugCommandList.empty())
				{
					ClearDepthStencil(primaryBuffer, offscreenTarget, clearValue.depthStencil);

					for (KRenderCommand& command : debugCommandList)
					{
						IKPipelineHandlePtr handle = nullptr;
						if (command.pipeline->GetHandle(offscreenTarget, handle))
						{
							command.pipelineHandle = handle;
							primaryBuffer->Render(command);
						}
					}
				}
			}
		}
		primaryBuffer->EndRenderPass();
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, frameIndex, m_SwapChain, m_UIOverlay, primaryBuffer);

	primaryBuffer->End();

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(PRE_Z_STAGE, preZStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEFAULT_STAGE, defaultStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEBUG_STAGE, debugStatistics);

	return true;
}

bool KRenderDispatcher::SubmitCommandBufferMuitiThread(KRenderScene* scene, KCamera* camera,
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
	IKCommandBufferPtr gizmoCommandBuffer = m_CommandBuffers[frameIndex].gizmoCommandBuffer;
	IKCommandBufferPtr clearCommandBuffer = m_CommandBuffers[frameIndex].clearCommandBuffer;

	KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

	KRenderStageStatistics preZStatistics;
	KRenderStageStatistics defaultStatistics;
	KRenderStageStatistics debugStatistics;

	// 开始渲染过程
	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			{
				primaryCommandBuffer->BeginRenderPass(shadowMapTarget, SUBPASS_CONTENTS_SECONDARY, clearValue);
				{
					shadowMapCommandBuffer->BeginSecondary(shadowMapTarget);
					// TODO 将Shadow Command拿出来操作
					KRenderGlobal::ShadowMap.UpdateShadowMap(m_Device, shadowMapCommandBuffer.get(), frameIndex);
					shadowMapCommandBuffer->End();
				}
				primaryCommandBuffer->Execute(shadowMapCommandBuffer);
				primaryCommandBuffer->EndRenderPass();
			}
		}
		// 物件绘制RenderPass
		{
			primaryCommandBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_SECONDARY, clearValue);

			auto commandBuffers = m_CommandBuffers[frameIndex].commandBuffersExec;
			commandBuffers.clear();

			// 绘制SkyBox
			{
				skyBoxCommandBuffer->BeginSecondary(offscreenTarget);
				skyBoxCommandBuffer->SetViewport(offscreenTarget);

				KRenderCommand command;
				if (KRenderGlobal::SkyBox.GetRenderCommand(frameIndex, command))
				{
					IKPipelineHandlePtr handle = nullptr;
					if (command.pipeline->GetHandle(offscreenTarget, handle))
					{
						command.pipelineHandle = handle;
						skyBoxCommandBuffer->Render(command);
					}
				}
				skyBoxCommandBuffer->End();

				commandBuffers.push_back(skyBoxCommandBuffer);
			}

			// 绘制Camera Gizmo
			{
				gizmoCommandBuffer->BeginSecondary(offscreenTarget);
				gizmoCommandBuffer->SetViewport(offscreenTarget);

				if (m_CameraCube)
				{
					KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
					KRenderCommandList commandList;
					cameraCube->GetRenderCommand(frameIndex, commandList);
					for (KRenderCommand& command : commandList)
					{
						IKPipelineHandlePtr handle = nullptr;
						if (command.pipeline->GetHandle(offscreenTarget, handle))
						{
							command.pipelineHandle = handle;
							gizmoCommandBuffer->Render(command);
						}
					}
				}
				gizmoCommandBuffer->End();

				commandBuffers.push_back(gizmoCommandBuffer);
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
				threadData.defaultCommands.clear();
				threadData.debugCommands.clear();

				auto addMesh = [&](size_t index)
				{
					KRenderComponent* component = cullRes[index];
					KMeshPtr mesh = component->GetMesh();

					IKEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());

							IKPipelineHandlePtr handle = nullptr;
							if (command.pipeline->GetHandle(offscreenTarget, handle))
							{
								command.pipelineHandle = handle;

								++preZStatistics.drawcalls;
								if (command.indexDraw)
								{
									preZStatistics.faces += command.indexData->indexCount / 3;
								}
								else
								{
									preZStatistics.faces += command.vertexData->vertexCount / 3;
								}

								threadData.preZcommands.push_back(std::move(command));
							}
						});

						mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, [&](KRenderCommand&& command)
						{
							command.SetObjectData(transform->FinalTransform());

							IKPipelineHandlePtr handle = nullptr;
							if (command.pipeline->GetHandle(offscreenTarget, handle))
							{
								command.pipelineHandle = handle;

								++defaultStatistics.drawcalls;
								if (command.indexDraw)
								{
									defaultStatistics.faces += command.indexData->indexCount / 3;
									defaultStatistics.primtives += command.indexData->indexCount;
								}
								else
								{
									defaultStatistics.faces += command.vertexData->vertexCount / 3;
									defaultStatistics.primtives += command.vertexData->vertexCount;
								}

								threadData.defaultCommands.push_back(std::move(command));
							}
						});

						KDebugComponent* debug = nullptr;
						if (entity->GetComponent(CT_DEBUG, (IKComponentBase**)&debug))
						{
							KConstantDefinition::DEBUG objectData;
							objectData.MODEL = transform->FinalTransform();
							objectData.COLOR = debug->Color();

							mesh->Visit(PIPELINE_STAGE_DEBUG_TRIANGLE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);

								IKPipelineHandlePtr handle = nullptr;
								if (command.pipeline->GetHandle(offscreenTarget, handle))
								{
									command.pipelineHandle = handle;

									++debugStatistics.drawcalls;
									if (command.indexDraw)
									{
										debugStatistics.faces += command.indexData->indexCount / 3;
										debugStatistics.primtives += command.indexData->indexCount;
									}
									else
									{
										debugStatistics.faces += command.vertexData->vertexCount / 3;
										debugStatistics.primtives += command.vertexData->vertexCount;
									}

									threadData.debugCommands.push_back(std::move(command));
								}
							});

							mesh->Visit(PIPELINE_STAGE_DEBUG_LINE, frameIndex, [&](KRenderCommand&& command)
							{
								command.SetObjectData(objectData);

								IKPipelineHandlePtr handle = nullptr;
								if (command.pipeline->GetHandle(offscreenTarget, handle))
								{
									command.pipelineHandle = handle;

									++debugStatistics.drawcalls;
									if (command.indexDraw)
									{
										debugStatistics.faces += command.indexData->indexCount / 2;
										debugStatistics.primtives += command.indexData->indexCount;
									}
									else
									{
										debugStatistics.faces += command.vertexData->vertexCount / 2;
										debugStatistics.primtives += command.vertexData->vertexCount;
									}

									threadData.debugCommands.push_back(std::move(command));
								}
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
				if (!threadData.defaultCommands.empty())
				{
					commandBuffers.push_back(threadData.defaultCommandBuffer);
					threadData.defaultCommands.clear();
				}
			}

			if (!commandBuffers.empty())
			{
				primaryCommandBuffer->ExecuteAll(commandBuffers);
				commandBuffers.clear();
			}

			for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
			{
				ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];
				if (!threadData.debugCommands.empty())
				{
					commandBuffers.push_back(threadData.debugCommandBuffer);
					threadData.debugCommands.clear();
				}
			}

			if (!commandBuffers.empty())
			{
				clearCommandBuffer->BeginSecondary(offscreenTarget);
				clearCommandBuffer->SetViewport(offscreenTarget);
				ClearDepthStencil(clearCommandBuffer, offscreenTarget, clearValue.depthStencil);
				clearCommandBuffer->End();

				primaryCommandBuffer->Execute(clearCommandBuffer);

				primaryCommandBuffer->ExecuteAll(commandBuffers);
				commandBuffers.clear();
			}

			primaryCommandBuffer->EndRenderPass();
		}
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, frameIndex, m_SwapChain, m_UIOverlay, primaryCommandBuffer);

	primaryCommandBuffer->End();

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(PRE_Z_STAGE, preZStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEFAULT_STAGE, defaultStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEBUG_STAGE, debugStatistics);

	return true;
}

bool KRenderDispatcher::Init(IKRenderDevice* device, uint32_t frameInFlight, IKSwapChainPtr swapChain, IKUIOverlayPtr uiOverlay, IKCameraCubePtr cameraCube)
{
	m_Device = device;
	m_FrameInFlight = frameInFlight;
	m_SwapChain = swapChain;
	m_UIOverlay = uiOverlay;
	m_CameraCube = cameraCube;
	m_ThreadPool.Init(m_MaxRenderThreadNum);
	CreateCommandBuffers();

	KRenderGlobal::Statistics.RegisterRenderStage(PRE_Z_STAGE);
	KRenderGlobal::Statistics.RegisterRenderStage(DEFAULT_STAGE);
	KRenderGlobal::Statistics.RegisterRenderStage(DEBUG_STAGE);

	return true;
}

bool KRenderDispatcher::UnInit()
{
	if (m_Device)
	{
		m_Device->Wait();
	}
	m_Device = nullptr;
	m_SwapChain = nullptr;
	m_UIOverlay = nullptr;
	m_CameraCube = nullptr;
	m_ThreadPool.UnInit();
	DestroyCommandBuffers();

	KRenderGlobal::Statistics.UnRegisterRenderStage(PRE_Z_STAGE);
	KRenderGlobal::Statistics.UnRegisterRenderStage(DEFAULT_STAGE);
	KRenderGlobal::Statistics.UnRegisterRenderStage(DEBUG_STAGE);

	return true;
}

bool KRenderDispatcher::Execute(KRenderScene* scene, KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex)
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