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

bool KRenderDispatcher::AssignInstanceData(IKRenderDevice* device, uint32_t frameIndex, KVertexData* vertexData, InstanceBufferStage stage, const std::vector<KConstantDefinition::OBJECT>& objects)
{
	if (vertexData && !objects.empty())
	{
		assert(frameIndex < MAX_FRAME_IN_FLIGHT_INSTANCE);
		IKVertexBufferPtr& instanceBuffer = vertexData->instanceBuffers[frameIndex][stage];
		if (!instanceBuffer)
		{
			device->CreateVertexBuffer(instanceBuffer);
		}

		// CPU用Hash检查数据是否需要重新填充看起来是个负优化 起码在PC上是这样
		// TODO 安卓待验证
		bool ENABLE_CPU_HASH = false;

		size_t dataSize = sizeof(KConstantDefinition::OBJECT) * objects.size();
		uint32_t dataHash = 0;
		
		if (ENABLE_CPU_HASH)
		{
			dataHash = KHash::BKDR((const char*)objects.data(), dataSize);
		}

		if (vertexData->instanceCount[frameIndex][stage] != objects.size())
		{
			vertexData->instanceCount[frameIndex][stage] = objects.size();
			// 数量不一样需要重新填充instance buffer
			vertexData->instanceDataHash[frameIndex][stage] = 0;
		}

		// instance buffer大小不够需要重新分配
		if (dataSize > instanceBuffer->GetBufferSize())
		{
			size_t vertexCount = KNumerical::Factor2GreaterEqual(objects.size());
			size_t vertexSize = sizeof(KConstantDefinition::OBJECT);

			ASSERT_RESULT(instanceBuffer->UnInit());
			ASSERT_RESULT(instanceBuffer->InitMemory(vertexCount, vertexSize, nullptr));
			ASSERT_RESULT(instanceBuffer->InitDevice(true));

			void* bufferData = nullptr;
			ASSERT_RESULT(instanceBuffer->Map(&bufferData));
			memcpy(bufferData, objects.data(), dataSize);
			ASSERT_RESULT(instanceBuffer->UnMap());
		}
		else // 检查是否需要重新填充instance buffer
		{
			if (!ENABLE_CPU_HASH || vertexData->instanceDataHash[frameIndex][stage] != dataHash)
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

				void* bufferData = nullptr;
				ASSERT_RESULT(instanceBuffer->Map(&bufferData));
				memcpy(bufferData, objects.data(), dataSize);
				ASSERT_RESULT(instanceBuffer->UnMap());
			}
		}

		vertexData->instanceDataHash[frameIndex][stage] = dataHash;

		return true;
	}
	return false;
}

void KRenderDispatcher::PopulateRenderCommand(size_t frameIndex, IKRenderTargetPtr offscreenTarget,
	std::vector<KRenderComponent*>& cullRes, std::vector<KRenderCommand>& preZcommands, std::vector<KRenderCommand>& defaultCommands, std::vector<KRenderCommand>& debugCommands)
{
	KRenderStageStatistics preZStatistics;
	KRenderStageStatistics defaultStatistics;
	KRenderStageStatistics debugStatistics;

	std::unordered_map<KMeshPtr, std::vector<KConstantDefinition::OBJECT>> meshGroups;

	for (KRenderComponent* component : cullRes)
	{
		if (!component->IsOcclusionVisible())
		{
			continue;
		}

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
					command.objectUsage.binding = SB_OBJECT;
					command.objectUsage.range = sizeof(objectData);
					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

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

					if (command.Complete())
					{
						debugCommands.push_back(std::move(command));
					}
				});

				mesh->Visit(PIPELINE_STAGE_DEBUG_LINE, frameIndex, [&](KRenderCommand&& command)
				{
					command.objectUsage.binding = SB_OBJECT;
					command.objectUsage.range = sizeof(objectData);
					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

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

					if (command.Complete())
					{
						debugCommands.push_back(std::move(command));
					}
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

				command.objectUsage.binding = SB_OBJECT;
				command.objectUsage.range = sizeof(objects[idx]);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objects[idx], command.objectUsage);

				command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

				if (command.Complete())
				{
					preZcommands.push_back(command);
				}
			}
		});

		/*
		mesh->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, [&](KRenderCommand&& _command)
		{
			KRenderCommand command = std::move(_command);

			for (size_t idx = 0; idx < objects.size(); ++idx)
			{
				++defaultStatistics.drawcalls;
				if (command.indexDraw)
				{
					defaultStatistics.faces += command.indexData->indexCount / 3;
				}
				else
				{
					defaultStatistics.faces += command.vertexData->vertexCount / 3;
				}

				command.objectUsage.binding = SB_OBJECT;
				command.objectUsage.range = sizeof(objects[idx]);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&objects[idx], command.objectUsage);

				command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

				if (command.Complete())
				{
					defaultCommands.push_back(command);
				}
			}
		});
		*/
		mesh->Visit(PIPELINE_STAGE_OPAQUE_INSTANCE, frameIndex, [&](KRenderCommand&& _command)
		{
			KRenderCommand command = std::move(_command);

			++defaultStatistics.drawcalls;

			KVertexData* vertexData = const_cast<KVertexData*>(command.vertexData);
			AssignInstanceData(m_Device, (uint32_t)frameIndex, vertexData, IBS_OPAQUE, objects);

			command.instanceDraw = true;
			command.instanceBuffer = vertexData->instanceBuffers[frameIndex][IBS_OPAQUE];
			command.instanceCount = (uint32_t)vertexData->instanceCount[frameIndex][IBS_OPAQUE];

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

			if (command.Complete())
			{
				defaultCommands.push_back(command);
			}
		});
	}

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(PRE_Z_STAGE, preZStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEFAULT_STAGE, defaultStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEBUG_STAGE, debugStatistics);
}

bool KRenderDispatcher::SubmitCommandBufferSingleThread(KRenderScene* scene, const KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex)
{
	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget(frameIndex);

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	IKCommandBufferPtr preZcommandBuffer = m_CommandBuffers[frameIndex].preZcommandBuffer;
	IKCommandBufferPtr defaultCommandBuffer = m_CommandBuffers[frameIndex].defaultCommandBuffer;
	IKCommandBufferPtr debugCommandBuffer = m_CommandBuffers[frameIndex].debugCommandBuffer;
	IKCommandBufferPtr clearCommandBuffer = m_CommandBuffers[frameIndex].clearCommandBuffer;

	std::vector<KRenderComponent*> cullRes;
	scene->GetRenderComponent(*camera, cullRes);

	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			KRenderStageStatistics shadowStatistics;
			KRenderGlobal::CascadedShadowMap.UpdateShadowMap(camera, frameIndex, primaryCommandBuffer, shadowStatistics);
			KRenderGlobal::Statistics.UpdateRenderStageStatistics(CSM_STAGE, shadowStatistics);
		}
		{
			KRenderGlobal::OcclusionBox.Reset(frameIndex, cullRes, primaryCommandBuffer);
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

				PopulateRenderCommand(frameIndex, offscreenTarget, cullRes, preZcommands, defaultCommands, debugCommands);

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

				KRenderGlobal::OcclusionBox.Render(frameIndex, offscreenTarget, camera, cullRes, commandBuffers);
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
					clearCommandBuffer->ClearDepthStencilRTRect(offscreenTarget, clearValue.depthStencil);
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

				KRenderGlobal::CascadedShadowMap.DebugRender(frameIndex, offscreenTarget, commandBuffers);
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

	return true;
}

bool KRenderDispatcher::SubmitCommandBufferMuitiThread(KRenderScene* scene, const KCamera* camera,
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

	std::vector<KRenderComponent*> cullRes;
	scene->GetRenderComponent(*camera, cullRes);

	// 开始渲染过程
	primaryCommandBuffer->BeginPrimary();
	{
		// 阴影绘制RenderPass
		{
			KRenderGlobal::CascadedShadowMap.UpdateShadowMap(camera, frameIndex, primaryCommandBuffer, shadowStatistics);
		}
		{
			KRenderGlobal::OcclusionBox.Reset(frameIndex, cullRes, primaryCommandBuffer);
		}
		// 物件绘制RenderPass
		{
			KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

			primaryCommandBuffer->BeginRenderPass(offscreenTarget, SUBPASS_CONTENTS_SECONDARY, clearValue);

			KCommandBufferList commandBuffers;

			// 绘制SkyBox
			KRenderGlobal::SkyBox.Render(frameIndex, offscreenTarget, commandBuffers);

			size_t threadCount = m_ThreadPool.GetWorkerThreadNum();

			KRenderCommandList preZcommands;
			KRenderCommandList defaultCommands;
			KRenderCommandList debugCommands;

			PopulateRenderCommand(frameIndex, offscreenTarget, cullRes, preZcommands, defaultCommands, debugCommands);
			
#define ASSIGN_RENDER_COMMAND(command)\
			{\
				size_t drawEachThread = command.size() / threadCount;\
				size_t reaminCount = command.size() % threadCount;\
				for (size_t i = 0; i < threadCount; ++i)\
				{\
					ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[i];\
					threadData.command.clear();\
\
					for (size_t idx = i * drawEachThread, ed = i * drawEachThread + drawEachThread; idx < ed; ++idx)\
					{\
						threadData.command.push_back(std::move(command[idx]));\
					}\
\
					if (i == threadCount - 1)\
					{\
						for (size_t idx = threadCount * drawEachThread, ed = threadCount * drawEachThread + reaminCount; idx < ed; ++idx)\
						{\
							threadData.command.push_back(std::move(command[idx]));\
						}\
					}\
				}\
			}

			ASSIGN_RENDER_COMMAND(preZcommands);
			ASSIGN_RENDER_COMMAND(defaultCommands);
			ASSIGN_RENDER_COMMAND(debugCommands);

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

			KRenderGlobal::OcclusionBox.Render(frameIndex, offscreenTarget, camera, cullRes, commandBuffers);
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
				clearCommandBuffer->ClearDepthStencilRTRect(offscreenTarget, clearValue.depthStencil);
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

			KRenderGlobal::CascadedShadowMap.DebugRender(frameIndex, offscreenTarget, commandBuffers);
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