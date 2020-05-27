#include "KRenderDispatcher.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/Gizmo/KCameraCube.h"
#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KNumerical.h"

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
static const char* CSM_STAGE = "CascadedShadowMap";

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

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].preZcommandBuffer);
		m_CommandBuffers[i].preZcommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].defaultCommandBuffer);
		m_CommandBuffers[i].defaultCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

		m_Device->CreateCommandBuffer(m_CommandBuffers[i].debugCommandBuffer);
		m_CommandBuffers[i].debugCommandBuffer->Init(m_CommandBuffers[i].commandPool, CBL_SECONDARY);

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
		m_CommandBuffers[i].preZcommandBuffer->UnInit();
		m_CommandBuffers[i].defaultCommandBuffer->UnInit();
		m_CommandBuffers[i].debugCommandBuffer->UnInit();
		m_CommandBuffers[i].clearCommandBuffer->UnInit();

		m_CommandBuffers[i].commandPool->UnInit();
		m_CommandBuffers[i].commandPool = nullptr;
	}

	m_CommandBuffers.clear();

	return true;
}

void KRenderDispatcher::ThreadRenderObject(uint32_t frameIndex, uint32_t threadIndex)
{
	ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];

	// https://devblogs.nvidia.com/vulkan-dos-donts/ ResetCommandPool释放内存
	threadData.commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	RenderSecondary(threadData.preZcommandBuffer, offscreenTarget, threadData.preZcommands);
	RenderSecondary(threadData.defaultCommandBuffer, offscreenTarget, threadData.defaultCommands);
	RenderSecondary(threadData.debugCommandBuffer, offscreenTarget, threadData.debugCommands);
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

void KRenderDispatcher::RenderSecondary(IKCommandBufferPtr buffer, IKRenderTargetPtr offscreenTarget, const std::vector<KRenderCommand>& commands)
{
	buffer->BeginSecondary(offscreenTarget);
	buffer->SetViewport(offscreenTarget);
	for (const KRenderCommand& command : commands)
	{
		buffer->Render(command);
	}
	buffer->End();
}


bool KRenderDispatcher::AssignInstanceData(KVertexData* vertexData, InstanceBufferStage stage, const std::vector<KConstantDefinition::OBJECT>& objects)
{
	if (vertexData && !objects.empty())
	{
		IKVertexBufferPtr& instanceBuffer = vertexData->instanceBuffers[stage];
		if (!instanceBuffer)
		{
			m_Device->CreateVertexBuffer(instanceBuffer);
		}

		size_t dataSize = sizeof(KConstantDefinition::OBJECT) * objects.size();
		uint32_t dataHash = KHash::BKDR((const char*)objects.data(), dataSize);

		if (vertexData->instanceCount[stage] != objects.size())
		{
			vertexData->instanceCount[stage] = objects.size();
			// 数量不一样需要重新填充instance buffer
			vertexData->instanceDataHash[stage] = 0;
		}

		// instance buffer大小不够需要重新分配
		if (dataSize > instanceBuffer->GetBufferSize())
		{
			size_t vertexCount = KNumerical::Factor2GreaterEqual(objects.size());
			size_t vertexSize = sizeof(KConstantDefinition::OBJECT);

			ASSERT_RESULT(instanceBuffer->UnInit());
			ASSERT_RESULT(instanceBuffer->InitMemory(vertexCount, vertexSize, nullptr));
			ASSERT_RESULT(instanceBuffer->InitDevice(true));
			ASSERT_RESULT(instanceBuffer->Write(objects.data()));
		}
		else // 检查是否需要重新填充instance buffer
		{
			if (vertexData->instanceDataHash[stage] != dataHash)
			{
				// 数据大小小于原来一半 instance buffer大小缩小到原来一半
				if (dataSize < instanceBuffer->GetBufferSize() / 2)
				{
					size_t vertexCount = instanceBuffer->GetVertexCount() / 2;
					size_t vertexSize = instanceBuffer->GetVertexSize();
					ASSERT_RESULT(instanceBuffer->UnInit());
					ASSERT_RESULT(instanceBuffer->InitMemory(vertexCount, vertexSize, nullptr));
					ASSERT_RESULT(instanceBuffer->InitDevice(true));
				}
				ASSERT_RESULT(instanceBuffer->Write(objects.data()));
			}
		}

		vertexData->instanceDataHash[stage] = dataHash;

		return true;
	}
	return false;
}

bool KRenderDispatcher::SubmitCommandBufferSingleThread(KRenderScene* scene, KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex)
{
	KRenderStageStatistics preZStatistics;
	KRenderStageStatistics defaultStatistics;
	KRenderStageStatistics debugStatistics;
	KRenderStageStatistics shadowStatistics;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	IKCommandBufferPtr preZcommandBuffer = m_CommandBuffers[frameIndex].preZcommandBuffer;
	IKCommandBufferPtr defaultCommandBuffer = m_CommandBuffers[frameIndex].defaultCommandBuffer;
	IKCommandBufferPtr debugCommandBuffer = m_CommandBuffers[frameIndex].debugCommandBuffer;
	IKCommandBufferPtr clearCommandBuffer = m_CommandBuffers[frameIndex].clearCommandBuffer;

	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			//KRenderGlobal::ShadowMap.UpdateShadowMap(frameIndex, primaryCommandBuffer);
			KRenderGlobal::CascadedShadowMap.UpdateShadowMap(camera, frameIndex, primaryCommandBuffer, shadowStatistics);
		}

		// 物件绘制RenderPass
		{
			KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

			primaryCommandBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_SECONDARY, clearValue);

			KCommandBufferList commandBuffers;

			// 绘制SkyBox
			KRenderGlobal::SkyBox.Render(frameIndex, offscreenTarget, commandBuffers);

			// 开始渲染物件
			{
				KRenderCommandList preZcommands;
				KRenderCommandList defaultCommands;
				KRenderCommandList debugCommands;

				std::vector<KRenderComponent*> cullRes;
				scene->GetRenderComponent(*camera, cullRes);

				std::unordered_map<KMeshPtr, std::vector<KConstantDefinition::OBJECT>> meshGroups;

				for (KRenderComponent* component : cullRes)
				{
					IKEntity* entity = component->GetEntityHandle();

					KTransformComponent* transform = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, &transform))
					{
						KMeshPtr mesh = component->GetMesh();

						auto it = meshGroups.find(mesh);
						if (it != meshGroups.end())
						{
							it->second.push_back(transform->FinalTransform());
						}
						else
						{
							std::vector<KConstantDefinition::OBJECT> objects(1, transform->FinalTransform());
							meshGroups[mesh] = objects;
						}

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
								command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

								defaultCommands.push_back(std::move(command));
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
								command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

								debugCommands.push_back(std::move(command));
							});
						}
					}
				}

				// 准备Instance数据
				for (auto& pair : meshGroups)
				{
					KMeshPtr mesh = pair.first;
					std::vector<KConstantDefinition::OBJECT>& objects = pair.second;

					ASSERT_RESULT(!objects.empty());

					mesh->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand&& _command)
					{
						KRenderCommand command = std::move(_command);
						command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

						for (size_t idx = 0; idx < objects.size(); ++idx)
						{
							++preZStatistics.drawcalls;
							if (command.indexDraw)
							{
								preZStatistics.faces += command.indexData->indexCount / 3;
							}
							else
							{
								preZStatistics.faces += command.vertexData->vertexCount / 3;
							}
							command.SetObjectData(objects[idx]);
							preZcommands.push_back(command);
						}
					});

					mesh->Visit(PIPELINE_STAGE_OPAQUE_INSTANCE, frameIndex, [&](KRenderCommand&& _command)
					{
						KRenderCommand command = std::move(_command);

						++defaultStatistics.drawcalls;

						KVertexData* vertexData = const_cast<KVertexData*>(command.vertexData);
						AssignInstanceData(vertexData, IBS_OPAQUE, objects);

						command.instanceDraw = true;
						command.instanceBuffer = vertexData->instanceBuffers[IBS_OPAQUE];
						command.instanceCount = (uint32_t)vertexData->instanceCount[IBS_OPAQUE];

						for (size_t idx = 0; idx < objects.size(); ++idx)
						{
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
						}

						command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);
						defaultCommands.push_back(command);
					});
				}

				RenderSecondary(preZcommandBuffer, offscreenTarget, preZcommands);
				RenderSecondary(defaultCommandBuffer, offscreenTarget, defaultCommands);
				RenderSecondary(debugCommandBuffer, offscreenTarget, debugCommands);

				if (!preZcommands.empty())
				{
					commandBuffers.push_back(preZcommandBuffer);
				}

				if (!defaultCommands.empty())
				{
					commandBuffers.push_back(defaultCommandBuffer);
				}

				if (!commandBuffers.empty())
				{
					primaryCommandBuffer->ExecuteAll(commandBuffers);
					commandBuffers.clear();
				}

				// 绘制Debug
				if (!debugCommands.empty())
				{
					commandBuffers.push_back(debugCommandBuffer);
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

				// 绘制Camera Gizmo
				if (m_CameraCube)
				{
					KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
					cameraCube->Render(frameIndex, offscreenTarget, commandBuffers);

					if (!commandBuffers.empty())
					{
						primaryCommandBuffer->ExecuteAll(commandBuffers);
						commandBuffers.clear();
					}
				}

				// KRenderGlobal::CascadedShadowMap.DebugRender(frameIndex, offscreenTarget, commandBuffers);
				if (!commandBuffers.empty())
				{
					primaryCommandBuffer->ExecuteAll(commandBuffers);
					commandBuffers.clear();
				}
			}
		}
		primaryCommandBuffer->EndRenderPass();
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, frameIndex, m_SwapChain, m_UIOverlay, primaryCommandBuffer);

	primaryCommandBuffer->End();

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(PRE_Z_STAGE, preZStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEFAULT_STAGE, defaultStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEBUG_STAGE, debugStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(CSM_STAGE, shadowStatistics);

	return true;
}

bool KRenderDispatcher::SubmitCommandBufferMuitiThread(KRenderScene* scene, KCamera* camera,
	uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	IKCommandBufferPtr clearCommandBuffer = m_CommandBuffers[frameIndex].clearCommandBuffer;

	KRenderStageStatistics preZStatistics;
	KRenderStageStatistics defaultStatistics;
	KRenderStageStatistics debugStatistics;
	KRenderStageStatistics shadowStatistics;

	// 开始渲染过程
	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			//KRenderGlobal::ShadowMap.UpdateShadowMap(frameIndex, primaryCommandBuffer);
			KRenderGlobal::CascadedShadowMap.UpdateShadowMap(camera, frameIndex, primaryCommandBuffer, shadowStatistics);
		}
		// 物件绘制RenderPass
		{
			KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

			primaryCommandBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_SECONDARY, clearValue);

			KCommandBufferList commandBuffers;

			// 绘制SkyBox
			KRenderGlobal::SkyBox.Render(frameIndex, offscreenTarget, commandBuffers);

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

			// 绘制Debug
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

			// 绘制Camera Gizmo
			if (m_CameraCube)
			{
				KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
				cameraCube->Render(frameIndex, offscreenTarget, commandBuffers);

				if (!commandBuffers.empty())
				{
					primaryCommandBuffer->ExecuteAll(commandBuffers);
					commandBuffers.clear();
				}
			}

			// KRenderGlobal::CascadedShadowMap.DebugRender(frameIndex, offscreenTarget, commandBuffers);
			if (!commandBuffers.empty())
			{
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
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(CSM_STAGE, shadowStatistics);

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
	KRenderGlobal::Statistics.RegisterRenderStage(CSM_STAGE);

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
	KRenderGlobal::Statistics.UnRegisterRenderStage(CSM_STAGE);

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