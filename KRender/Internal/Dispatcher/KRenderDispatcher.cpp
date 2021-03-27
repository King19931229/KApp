#include "KRenderDispatcher.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/Gizmo/KCameraCube.h"
#include "Internal/KConstantGlobal.h"
#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KNumerical.h"

KMainBasePass::KMainBasePass(KRenderDispatcher& master)
	: m_Master(master),
	KFrameGraphPass("MainPass")
{
}

KMainBasePass::~KMainBasePass()
{
}

bool KMainBasePass::Init()
{
	KRenderGlobal::FrameGraph.RegisterPass(this);
	return true;
}

bool KMainBasePass::UnInit()
{
	KRenderGlobal::FrameGraph.UnRegisterPass(this);
	return true;
}

bool KMainBasePass::Setup(KFrameGraphBuilder& builder)
{
	KCascadedShadowMapPassPtr shadowPass = KRenderGlobal::CascadedShadowMap.GetPass();
	const std::vector<KFrameGraphID>& shadowRts = shadowPass->GetAllTargetID();
	for (const KFrameGraphID& id : shadowRts)
	{
		builder.Read(id);
	}
	return true;
}

bool KMainBasePass::Execute(KFrameGraphExecutor& executor)
{
	IKCommandBufferPtr primaryBuffer = executor.GetPrimaryBuffer();
	uint32_t frameIndex = executor.GetFrameIndex();
	uint32_t chainIndex = executor.GetChainIndex();
	m_Master.UpdateBasePass(chainIndex, frameIndex);
	return true;
}

KRenderDispatcher::KRenderDispatcher()
	: m_Device(nullptr),
	m_SwapChain(nullptr),
	m_UIOverlay(nullptr),
	m_Scene(nullptr),
	m_Camera(nullptr),
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

	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget();
	IKRenderPassPtr renderPass = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderPass();

	RenderSecondary(threadData.preZcommandBuffer, renderPass, threadData.preZcommands);
	RenderSecondary(threadData.defaultCommandBuffer, renderPass, threadData.defaultCommands);
	RenderSecondary(threadData.debugCommandBuffer, renderPass, threadData.debugCommands);
}

void KRenderDispatcher::RenderSecondary(IKCommandBufferPtr buffer, IKRenderPassPtr renderPass, const std::vector<KRenderCommand>& commands)
{
	buffer->BeginSecondary(renderPass);
	buffer->SetViewport(renderPass->GetViewPort());
	for (const KRenderCommand& command : commands)
	{
		buffer->Render(command);
	}
	buffer->End();
}

bool KRenderDispatcher::AssignShadingParameter(KRenderCommand& command, IKMaterial* material, bool useMaterialTex)
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
		
		if (useMaterialTex)
		{
			const IKMaterialTextureBinding* textureBinding = material->GetDefaultMaterialTexture().get();
			uint8_t numSlot = textureBinding->GetNumSlot();
			for (uint8_t i = 0; i < numSlot; ++i)
			{
				IKTexturePtr texture = textureBinding->GetTexture(i);
				IKSamplerPtr sampler = textureBinding->GetSampler(i);
				if (texture && sampler)
				{
					command.pipeline->SetSampler(SHADER_BINDING_MATERIAL_BEGIN + i, texture, sampler, true);
				}
			}
		}
		return true;
	}
	return false;
}

void KRenderDispatcher::PopulateRenderCommand(size_t frameIndex, IKRenderPassPtr renderPass,
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
	std::unordered_map<KMeshPtr, MaterialMapPtr> meshMaterialGroups;

	for (bool useMateiralTex : {true, false})
	{
		for (KRenderComponent* render : cullRes)
		{
			if (!render->IsOcclusionVisible())
			{
				continue;
			}

			if (render->GetUseMaterialTexture() != useMateiralTex)
			{
				continue;
			}

			IKEntity* entity = render->GetEntityHandle();

			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
			{
				MaterialMapPtr mateiralMap = nullptr;
				InstanceArrayPtr instanceArray = nullptr;

				KMeshPtr mesh = render->GetMesh();

				std::unordered_map<KMeshPtr, MaterialMapPtr>* meshMap = nullptr;
				if (useMateiralTex)
				{
					meshMap = &meshMaterialGroups;
				}
				else
				{
					meshMap = &meshGroups;
				}

				auto itMesh = meshMap->find(mesh);
				if (itMesh != meshMap->end())
				{
					mateiralMap = itMesh->second;
				}
				else
				{
					mateiralMap = std::make_shared<MaterialMap>();
					(*meshMap)[mesh] = mateiralMap;
				}
				ASSERT_RESULT(mateiralMap);

				IKMaterialPtr material = render->GetMaterial();

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

				instanceArray->render = render;
				instanceArray->instances.push_back({ transform->GetFinal() });

				KDebugComponent* debug = nullptr;
				if (entity->GetComponent(CT_DEBUG, (IKComponentBase**)&debug))
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

						command.pipeline->GetHandle(renderPass, command.pipelineHandle);

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

						command.pipeline->GetHandle(renderPass, command.pipelineHandle);

						if (command.Complete())
						{
							debugCommands.push_back(std::move(command));
						}
					});
				}
			}
		}
	}

	for (bool useMateiralTex : {true, false})
	{
		std::unordered_map<KMeshPtr, MaterialMapPtr>* meshMap = nullptr;
		if (useMateiralTex)
		{
			meshMap = &meshMaterialGroups;
		}
		else
		{
			meshMap = &meshGroups;
		}

		// 准备Instance数据
		for (auto& meshPair : *meshMap)
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

						command.pipeline->GetHandle(renderPass, command.pipelineHandle);

						if (command.Complete())
						{
							preZcommands.push_back(command);
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

							if (!AssignShadingParameter(command, material.get(), false))
							{
								continue;
							}

							command.pipeline->GetHandle(renderPass, command.pipelineHandle);

							if (command.Complete())
							{
								defaultCommands.push_back(command);
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

						if (AssignShadingParameter(command, material.get(), useMateiralTex))
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

							command.pipeline->GetHandle(renderPass, command.pipelineHandle);

							if (command.Complete())
							{
								defaultCommands.push_back(std::move(command));
							}
						}
					});
				}
			}
		}
	}

	KRenderGlobal::Statistics.UpdateRenderStageStatistics(PRE_Z_STAGE, preZStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEFAULT_STAGE, defaultStatistics);
	KRenderGlobal::Statistics.UpdateRenderStageStatistics(DEBUG_STAGE, debugStatistics);
}

void KRenderDispatcher::AssignRenderCommand(size_t frameIndex,
	const std::vector<KRenderCommand>& preZcommands, const std::vector<KRenderCommand>& defaultCommands, const std::vector<KRenderCommand>& debugCommands)
{
	if (m_MultiThreadSubmit)
	{
		size_t threadCount = m_ThreadPool.GetWorkerThreadNum();

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

#undef ASSIGN_RENDER_COMMAND
	}
	else
	{
		ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[0];
		threadData.preZcommands = preZcommands;
		threadData.defaultCommands = defaultCommands;
		threadData.debugCommands = debugCommands;
	}
}

void KRenderDispatcher::SumbitRenderCommand(size_t frameIndex, std::vector<IKCommandBufferPtr>& preZBuffers, std::vector<IKCommandBufferPtr>& defaultBuffers, std::vector<IKCommandBufferPtr>& debugBuffers)
{
	if (m_MultiThreadSubmit)
	{
		size_t threadCount = m_ThreadPool.GetWorkerThreadNum();

		for (size_t i = 0; i < threadCount; ++i)
		{
			m_ThreadPool.SubmitTask([=]()
			{
				ThreadRenderObject((uint32_t)frameIndex, (uint32_t)i);
			});
		}
		m_ThreadPool.WaitAllAsyncTaskDone();

		for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[threadIndex];
			if (!threadData.preZcommands.empty())
			{
				preZBuffers.push_back(threadData.preZcommandBuffer);
				threadData.preZcommands.clear();
			}

			if (!threadData.defaultCommands.empty())
			{
				defaultBuffers.push_back(threadData.defaultCommandBuffer);
				threadData.defaultCommands.clear();
			}

			if (!threadData.debugCommands.empty())
			{
				debugBuffers.push_back(threadData.debugCommandBuffer);
				threadData.debugCommands.clear();
			}
		}
	}
	else
	{
		ThreadRenderObject((uint32_t)frameIndex, 0);

		ThreadData& threadData = m_CommandBuffers[frameIndex].threadDatas[0];
		if (!threadData.preZcommands.empty())
		{
			preZBuffers.push_back(threadData.preZcommandBuffer);
			threadData.preZcommands.clear();
		}

		if (!threadData.defaultCommands.empty())
		{
			defaultBuffers.push_back(threadData.defaultCommandBuffer);
			threadData.defaultCommands.clear();
		}

		if (!threadData.debugCommands.empty())
		{
			debugBuffers.push_back(threadData.debugCommandBuffer);
			threadData.debugCommands.clear();
		}
	}
}

bool KRenderDispatcher::UpdateBasePass(uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget();
	IKRenderPassPtr renderPass = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderPass();

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	IKCommandBufferPtr clearCommandBuffer = m_CommandBuffers[frameIndex].clearCommandBuffer;

	std::vector<KRenderComponent*> cullRes;
	((KRenderScene*)m_Scene)->GetRenderComponent(*m_Camera, cullRes);

	// 物件绘制RenderPass
	{
		KRenderGlobal::OcclusionBox.Reset(frameIndex, cullRes, primaryCommandBuffer);

		KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

		KRenderCommandList preZcommands;
		KRenderCommandList defaultCommands;
		KRenderCommandList debugCommands;

		std::vector<IKCommandBufferPtr> tempBuffers;
		std::vector<IKCommandBufferPtr> preZBuffers;
		std::vector<IKCommandBufferPtr> defaultBuffers;
		std::vector<IKCommandBufferPtr> debugBuffers;

		primaryCommandBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);

		// 绘制SkyBox
		KRenderGlobal::SkyBox.Render(frameIndex, renderPass, tempBuffers);
		if (!tempBuffers.empty())
		{
			primaryCommandBuffer->ExecuteAll(tempBuffers);
			tempBuffers.clear();
		}

		PopulateRenderCommand(frameIndex, renderPass, cullRes, preZcommands, defaultCommands, debugCommands);
		AssignRenderCommand(frameIndex, preZcommands, defaultCommands, debugCommands);
		SumbitRenderCommand(frameIndex, preZBuffers, defaultBuffers, debugBuffers);

		if (!preZBuffers.empty())
		{
			primaryCommandBuffer->ExecuteAll(preZBuffers);
		}
		if (!defaultBuffers.empty())
		{
			primaryCommandBuffer->ExecuteAll(defaultBuffers);
		}

		KRenderGlobal::OcclusionBox.Render(frameIndex, renderPass, m_Camera, cullRes, tempBuffers);
		if (!tempBuffers.empty())
		{
			primaryCommandBuffer->ExecuteAll(tempBuffers);
			tempBuffers.clear();
		}

		if (!debugBuffers.empty())
		{
			primaryCommandBuffer->ExecuteAll(debugBuffers);
		}

		// 绘制Camera Gizmo
		if (m_DisplayCameraCube)
		{
			if (m_CameraCube)
			{
				KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
				cameraCube->Render(frameIndex, renderPass, tempBuffers);

				if (!tempBuffers.empty())
				{
					// Clear the depth stencil
					{
						clearCommandBuffer->BeginSecondary(renderPass);
						clearCommandBuffer->SetViewport(renderPass->GetViewPort());
						clearCommandBuffer->ClearDepthStencil(renderPass->GetViewPort(), clearValue.depthStencil);
						clearCommandBuffer->End();
						primaryCommandBuffer->Execute(clearCommandBuffer);
					}

					primaryCommandBuffer->ExecuteAll(tempBuffers);
					tempBuffers.clear();
				}
			}
		}

		primaryCommandBuffer->EndRenderPass();
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, frameIndex, m_SwapChain, m_UIOverlay, primaryCommandBuffer);
	return true;
}

bool KRenderDispatcher::SubmitCommandBuffers(uint32_t chainImageIndex, uint32_t frameIndex)
{
	IKPipelineHandlePtr pipelineHandle;

	assert(frameIndex < m_CommandBuffers.size());

	m_CommandBuffers[frameIndex].commandPool->Reset();

	IKCommandBufferPtr primaryCommandBuffer = m_CommandBuffers[frameIndex].primaryCommandBuffer;
	// 开始渲染过程
	primaryCommandBuffer->BeginPrimary();
	{
		KRenderGlobal::CascadedShadowMap.UpdateShadowMap(m_Camera, frameIndex, primaryCommandBuffer);
		KRenderGlobal::FrameGraph.Compile();
		KRenderGlobal::FrameGraph.Execute(primaryCommandBuffer, frameIndex, chainImageIndex);
	}
	primaryCommandBuffer->End();

	return true;
}

bool KRenderDispatcher::Init(IKRenderDevice* device, uint32_t frameInFlight, IKCameraCubePtr cameraCube)
{
	m_Device = device;
	m_FrameInFlight = frameInFlight;
	m_CameraCube = cameraCube;
	m_ThreadPool.Init("RenderThread", m_MaxRenderThreadNum);
	m_Pass = KMainBasePassPtr(KNEW KMainBasePass(*this));
	m_Pass->Init();
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
	SAFE_UNINIT(m_Pass);
	m_ThreadPool.UnInit();
	DestroyCommandBuffers();

	KRenderGlobal::Statistics.UnRegisterRenderStage(PRE_Z_STAGE);
	KRenderGlobal::Statistics.UnRegisterRenderStage(DEFAULT_STAGE);
	KRenderGlobal::Statistics.UnRegisterRenderStage(DEBUG_STAGE);
	KRenderGlobal::Statistics.UnRegisterRenderStage(CSM_STAGE);

	return true;
}

bool KRenderDispatcher::SetCameraCubeDisplay(bool display)
{
	m_DisplayCameraCube = display;
	return true;
}

bool KRenderDispatcher::SetSwapChain(IKSwapChain* swapChain, IKUIOverlay* uiOverlay)
{
	m_SwapChain = swapChain;
	m_UIOverlay = uiOverlay;
	return true;
}

bool KRenderDispatcher::SetSceneCamera(IKRenderScene* scene, const KCamera* camera)
{
	m_Scene = scene;
	m_Camera = camera;
	return true;
}

bool KRenderDispatcher::SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback)
{
	if (window && callback)
	{
		m_WindowRenderCB[window] = callback;
		return true;
	}
	return false;
}

bool KRenderDispatcher::RemoveCallback(IKRenderWindow* window)
{
	if (window)
	{
		if (m_WindowRenderCB.erase(window))
		{
			return true;
		}
	}
	return false;
}

bool KRenderDispatcher::UpdateCamera(size_t frameIndex)
{
	if (m_Camera)
	{
		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		if (cameraBuffer)
		{
			glm::mat4 view = m_Camera->GetViewMatrix();
			glm::mat4 proj = m_Camera->GetProjectiveMatrix();
			glm::mat4 viewInv = glm::inverse(view);

			void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CAMERA);
			const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA);
			for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
			{
				void* pWritePos = nullptr;
				if (detail.semantic == CS_VIEW)
				{
					assert(sizeof(view) == detail.size);
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					memcpy(pWritePos, &view, sizeof(view));
				}
				else if (detail.semantic == CS_PROJ)
				{
					assert(sizeof(proj) == detail.size);
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					memcpy(pWritePos, &proj, sizeof(proj));
				}
				else if (detail.semantic == CS_VIEW_INV)
				{
					assert(sizeof(viewInv) == detail.size);
					pWritePos = POINTER_OFFSET(pData, detail.offset);
					memcpy(pWritePos, &viewInv, sizeof(viewInv));
				}
			}
			cameraBuffer->Write(pData);
			return true;
		}
	}
	return false;
}

bool KRenderDispatcher::UpdateGlobal(size_t frameIndex)
{
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_GLOBAL);
	if (globalBuffer)
	{
		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_GLOBAL);
		const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_GLOBAL);
		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			void* pWritePos = nullptr;
			if (detail.semantic == CS_SUN_LIGHT_DIRECTION)
			{
				glm::vec4 sunLightDir = glm::vec4(KRenderGlobal::CascadedShadowMap.GetCamera().GetForward(), 0.0f);
				assert(sizeof(sunLightDir) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &sunLightDir, sizeof(sunLightDir));
			}
		}
		globalBuffer->Write(pData);
		return true;
	}
	return false;
}

bool KRenderDispatcher::Update(uint32_t frameIndex)
{
	UpdateGlobal(frameIndex);
	return true;
}

bool KRenderDispatcher::Execute(uint32_t chainImageIndex, uint32_t frameIndex)
{
	if (m_SwapChain)
	{
		IKRenderWindow* window = m_SwapChain->GetWindow();
		ASSERT_RESULT(window);

		size_t windowWidth = 0, windowHeight = 0;
		ASSERT_RESULT(window->GetSize(windowWidth, windowHeight));

		// 窗口最小化后就干脆不提交了
		if (windowWidth && windowHeight)
		{
			// 促发绘制回调
			{
				auto it = m_WindowRenderCB.find(window);
				if (it != m_WindowRenderCB.end())
				{
					(*(it->second))(this, chainImageIndex, frameIndex);
				}
			}

			if (m_Scene && m_Camera)
			{
				UpdateCamera(frameIndex);
				SubmitCommandBuffers(chainImageIndex, frameIndex);
			}
		}
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