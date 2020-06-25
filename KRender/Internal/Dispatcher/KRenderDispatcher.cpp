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
	m_MultiThreadSubmit(true),
	m_InstanceSubmit(true)
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

bool KRenderDispatcher::AssignShadingParameter(KRenderCommand& command, IKMaterial* material)
{
	if (material)
	{
		const IKMaterialParameterPtr vsParameter = material->GetVSParameter();
		const IKMaterialParameterPtr fsParameter = material->GetFSParameter();
		if (!(vsParameter && fsParameter))
		{
			return false;
		}

		const KShaderInformation::Constant* vsConstant = material->GetVSShadingInfo();
		const KShaderInformation::Constant* fsConstant = material->GetFSShadingInfo();
		if (!(vsConstant && fsConstant))
		{
			return false;
		}

		if (vsConstant->size)
		{
			static std::vector<char> vsShadingBuffer;

			command.vertexShadingUsage.binding = SHADER_BINDING_VERTEX_SHADING;
			command.vertexShadingUsage.range = vsConstant->size;

			if (vsConstant->size > vsShadingBuffer.size())
			{
				vsShadingBuffer.resize(vsConstant->size);
			}

			for (const KShaderInformation::Constant::ConstantMember& member : vsConstant->members)
			{
				IKMaterialValuePtr value = vsParameter->GetValue(member.name);
				ASSERT_RESULT(value);
				memcpy(POINTER_OFFSET(vsShadingBuffer.data(), member.offset), value->GetData(), member.size);
			}

			KRenderGlobal::DynamicConstantBufferManager.Alloc(vsShadingBuffer.data(), command.vertexShadingUsage);
		}

		if (fsConstant->size)
		{
			static std::vector<char> fsShadingBuffer;

			command.fragmentShadingUsage.binding = SHADER_BINDING_FRAGMENT_SHADING;
			command.fragmentShadingUsage.range = fsConstant->size;

			if (fsConstant->size > fsShadingBuffer.size())
			{
				fsShadingBuffer.resize(fsConstant->size);
			}

			for (const KShaderInformation::Constant::ConstantMember& member : fsConstant->members)
			{
				IKMaterialValuePtr value = fsParameter->GetValue(member.name);
				ASSERT_RESULT(value);
				memcpy(POINTER_OFFSET(fsShadingBuffer.data(), member.offset), value->GetData(), member.size);
			}

			KRenderGlobal::DynamicConstantBufferManager.Alloc(fsShadingBuffer.data(), command.fragmentShadingUsage);
		}
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

	struct InstanceArray
	{
		KRenderComponent* render;
		std::vector<KConstantDefinition::OBJECT> instances;
	};
	typedef std::shared_ptr<InstanceArray> InstanceArrayPtr;
	struct MaterialMap
	{
		std::unordered_map<IKMaterialPtr, InstanceArrayPtr> mapping;
	};
	typedef std::shared_ptr<MaterialMap> MaterialMapPtr;
	std::unordered_map<KMeshPtr, MaterialMapPtr> meshGroups;

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
			MaterialMapPtr mateiralMap = nullptr;
			InstanceArrayPtr instanceArray = nullptr;

			KMeshPtr mesh = component->GetMesh();

			auto itMesh = meshGroups.find(mesh);

			if (itMesh != meshGroups.end())
			{
				mateiralMap = itMesh->second;
			}
			else
			{
				mateiralMap = std::make_shared<MaterialMap>();
				meshGroups[mesh] = mateiralMap;
			}
			ASSERT_RESULT(mateiralMap);

			IKMaterialPtr material = component->GetMaterial();

			auto itMat = mateiralMap->mapping.find(material);
			if (itMat == mateiralMap->mapping.end())
			{
				instanceArray = std::make_shared<InstanceArray>();
				(mateiralMap->mapping)[material] = instanceArray;
			}
			else
			{
				instanceArray = itMat->second;
			}

			instanceArray->render = component;
			instanceArray->instances.push_back({ transform->GetFinal() });

			KDebugComponent* debug = nullptr;
			KRenderComponent* render = nullptr;
			if (entity->GetComponent(CT_DEBUG, (IKComponentBase**)&debug) && entity->GetComponent(CT_RENDER, (IKComponentBase**)&render))
			{
				KConstantDefinition::DEBUG objectData;
				objectData.MODEL = transform->FinalTransform();
				objectData.COLOR = debug->Color();

				render->Visit(PIPELINE_STAGE_DEBUG_TRIANGLE, frameIndex, [&](KRenderCommand& command)
				{
					command.objectUsage.binding = SHADER_BINDING_OBJECT;
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

				render->Visit(PIPELINE_STAGE_DEBUG_LINE, frameIndex, [&](KRenderCommand& command)
				{
					command.objectUsage.binding = SHADER_BINDING_OBJECT;
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
	for (auto& meshPair : meshGroups)
	{
		KMeshPtr mesh = meshPair.first;
		MaterialMapPtr materialMap = meshPair.second;

		for (auto& materialPair : materialMap->mapping)
		{
			IKMaterialPtr material = materialPair.first;
			InstanceArrayPtr instanceArray = materialPair.second;
			KRenderComponent* render = instanceArray->render;
			std::vector<KConstantDefinition::OBJECT>& instances = instanceArray->instances;

			ASSERT_RESULT(render);
			ASSERT_RESULT(instances.size() > 0);

			// TODO PIPELINE_STAGE_PRE_Z Instance
			render->Visit(PIPELINE_STAGE_PRE_Z, frameIndex, [&](KRenderCommand& _command)
			{
				KRenderCommand command = std::move(_command);

				for (size_t idx = 0; idx < instances.size(); ++idx)
				{
					++preZStatistics.drawcalls;
					if (command.indexDraw)
					{
						preZStatistics.faces += command.indexData->indexCount / 3;
						preZStatistics.primtives += command.indexData->indexCount;
					}
					else
					{
						preZStatistics.faces += command.vertexData->vertexCount / 3;
						preZStatistics.primtives += command.vertexData->vertexCount;
					}

					command.objectUsage.binding = SHADER_BINDING_OBJECT;
					command.objectUsage.range = sizeof(instances[idx]);
					KRenderGlobal::DynamicConstantBufferManager.Alloc(&instances[idx], command.objectUsage);

					command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

					if (command.Complete())
					{
						preZcommands.push_back(std::move(command));
					}
				}
			});

			if (!m_InstanceSubmit)
			{
				render->Visit(PIPELINE_STAGE_OPAQUE, frameIndex, [&](KRenderCommand& _command)
				{
					KRenderCommand command = std::move(_command);

					for (size_t idx = 0; idx < instances.size(); ++idx)
					{
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

						command.objectUsage.binding = SHADER_BINDING_OBJECT;
						command.objectUsage.range = sizeof(instances[idx]);
						KRenderGlobal::DynamicConstantBufferManager.Alloc(&instances[idx], command.objectUsage);

						if (!AssignShadingParameter(command, material.get()))
						{
							continue;
						}

						command.pipeline->GetHandle(offscreenTarget, command.pipelineHandle);

						if (command.Complete())
						{
							defaultCommands.push_back(std::move(command));
						}
					}
				});
			}
			else
			{
				render->Visit(PIPELINE_STAGE_OPAQUE_INSTANCE, frameIndex, [&](KRenderCommand& _command)
				{
					KRenderCommand command = std::move(_command);

					++defaultStatistics.drawcalls;

					KVertexData* vertexData = const_cast<KVertexData*>(command.vertexData);

					std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
					ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(instances[0]));
					ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(instances.size(), instances.data(), allocRes));

					command.instanceDraw = true;
					command.instanceUsages.resize(allocRes.size());
					for (size_t i = 0; i < allocRes.size(); ++i)
					{
						KInstanceBufferUsage& usage = command.instanceUsages[i];
						KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
						usage.buffer = allocResult.buffer;
						usage.start = allocResult.start;
						usage.count = allocResult.count;
					}

					if (AssignShadingParameter(command, material.get()))
					{
						for (size_t idx = 0; idx < instances.size(); ++idx)
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
							defaultCommands.push_back(std::move(command));
						}
					}
				});
			}
		}
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
	m_ThreadPool.Init("RenderThread", m_MaxRenderThreadNum);
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
	if (m_MultiThreadSubmit)
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