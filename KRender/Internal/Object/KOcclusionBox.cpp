#include "KOcclusionBox.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

#include "KBase/Interface/IKLog.h"

const KVertexDefinition::POS_3F_NORM_3F_UV_2F KOcclusionBox::ms_Positions[] =
{
	{ glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 1.0f) },

	{ glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f) },
	{ glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1.0, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 1.0f) }
};

const uint16_t KOcclusionBox::ms_Indices[] =
{
	// up
	5, 1, 2, 2, 4, 5,
	// down
	6, 3, 0, 3, 6, 7,
	// left
	0, 1, 5, 0, 5, 6,
	// right
	7, 4, 2, 7, 2, 3,
	// front
	6, 5, 4, 6, 4, 7,
	// back
	0, 2, 1, 2, 0, 3,
};

const VertexFormat KOcclusionBox::ms_VertexFormats[] = { VF_POINT_NORMAL_UV };
const VertexFormat KOcclusionBox::ms_VertexInstanceFormats[] = { VF_POINT_NORMAL_UV, VF_INSTANCE };

KOcclusionBox::KOcclusionBox()
	: m_Device(nullptr),
	m_InstanceGroupSize(1000.0f),
	m_Enable(true)
{
}

KOcclusionBox::~KOcclusionBox()
{
	ASSERT_RESULT(m_Device == nullptr);
}

void KOcclusionBox::LoadResource()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("occlusion.vert", m_VertexShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("occlusioninstance.vert", m_VertexInstanceShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("occlusion.frag", m_FragmentShader, false));

	m_VertexBuffer->InitMemory(ARRAY_SIZE(ms_Positions), sizeof(ms_Positions[0]), ms_Positions);
	m_VertexBuffer->InitDevice(false);

	m_IndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_Indices), ms_Indices);
	m_IndexBuffer->InitDevice(false);
}

void KOcclusionBox::PreparePipeline()
{
	// https://kayru.org/articles/deferred-stencil/
	/*
	Each light volume (low poly sphere) is rendered it two passes.

	Pass 1:
	•Front (near) faces only
	•Colour write is disabled
	•Z-write is disabled
	•Z function is 'Less/Equal'
	•Z-Fail writes non-zero value to Stencil buffer (for example, 'Increment-Saturate')
	•Stencil test result does not modify Stencil buffer

	This pass creates a Stencil mask for the areas of the light volume that are not occluded by scene geometry.

	Pass 2:
	•Back (far) faces only
	•Colour write enabled
	•Z-write is disabled
	•Z function is 'Greater/Equal'
	•Stencil function is 'Equal' (Stencil ref = zero)
	•Always clears Stencil to zero

	This pass is where lighting actually happens. Every pixel that passes Z and Stencil tests is then added to light accumulation buffer.
	*/

	for (size_t i = 0; i < m_PipelinesFrontFace.size(); ++i)
	{
		IKPipelinePtr pipeline = m_PipelinesFrontFace[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, false, true);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_ALWAYS, SO_KEEP, SO_INC, SO_KEEP);

		pipeline->SetColorWrite(false, false, false, false);
		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}

	for (size_t i = 0; i < m_PipelinesInstanceFrontFace.size(); ++i)
	{
		IKPipelinePtr pipeline = m_PipelinesInstanceFrontFace[i];
		pipeline->SetVertexBinding(ms_VertexInstanceFormats, ARRAY_SIZE(ms_VertexInstanceFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, false, true);
		pipeline->SetShader(ST_VERTEX, m_VertexInstanceShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_ALWAYS, SO_KEEP, SO_INC, SO_KEEP);

		pipeline->SetColorWrite(false, false, false, false);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}

	for (size_t i = 0; i < m_PipelinesBackFace.size(); ++i)
	{
		IKPipelinePtr pipeline = m_PipelinesBackFace[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_FRONT);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_GREATER, false, true);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_EQUAL, SO_DEC, SO_KEEP, SO_KEEP);

		pipeline->SetColorWrite(false, false, false, false);
		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}

	for (size_t i = 0; i < m_PipelinesInstanceBackFace.size(); ++i)
	{
		IKPipelinePtr pipeline = m_PipelinesInstanceBackFace[i];
		pipeline->SetVertexBinding(ms_VertexInstanceFormats, ARRAY_SIZE(ms_VertexInstanceFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_FRONT);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_GREATER, false, true);
		pipeline->SetShader(ST_VERTEX, m_VertexInstanceShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_EQUAL, SO_DEC, SO_KEEP, SO_KEEP);

		pipeline->SetColorWrite(false, false, false, false);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(i, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}
}

void KOcclusionBox::InitRenderData()
{
	m_VertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_VertexBuffer);
	m_VertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	m_VertexData.vertexCount = ARRAY_SIZE(ms_Positions);
	m_VertexData.vertexStart = 0;

	m_IndexData.indexBuffer = m_IndexBuffer;
	m_IndexData.indexCount = ARRAY_SIZE(ms_Indices);
	m_IndexData.indexStart = 0;
}

bool KOcclusionBox::Init(IKRenderDevice* renderDevice, size_t frameInFlight)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(frameInFlight > 0);

	ASSERT_RESULT(UnInit());

	m_Device = renderDevice;

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateVertexBuffer(m_VertexBuffer);
	renderDevice->CreateIndexBuffer(m_IndexBuffer);

	m_PipelinesFrontFace.resize(frameInFlight);
	m_PipelinesBackFace.resize(frameInFlight);
	m_PipelinesInstanceFrontFace.resize(frameInFlight);
	m_PipelinesInstanceBackFace.resize(frameInFlight);

	m_CommandBuffers.resize(frameInFlight);
	m_InstanceBuffers.resize(frameInFlight);

	for (size_t i = 0; i < frameInFlight; ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_PipelinesFrontFace[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_PipelinesBackFace[i]);

		KRenderGlobal::PipelineManager.CreatePipeline(m_PipelinesInstanceFrontFace[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_PipelinesInstanceBackFace[i]);

		IKCommandBufferPtr& buffer = m_CommandBuffers[i];
		ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
		ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
	}

	LoadResource();
	PreparePipeline();
	InitRenderData();

	return true;
}

bool KOcclusionBox::UnInit()
{
	for (IKPipelinePtr pipeline : m_PipelinesFrontFace)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_PipelinesFrontFace.clear();

	for (IKPipelinePtr pipeline : m_PipelinesBackFace)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_PipelinesBackFace.clear();

	for (IKPipelinePtr pipeline : m_PipelinesInstanceFrontFace)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_PipelinesInstanceFrontFace.clear();

	for (IKPipelinePtr pipeline : m_PipelinesInstanceBackFace)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_PipelinesInstanceBackFace.clear();

	for (IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_CommandBuffers.clear();

	for (FrameInstanceBufferList& instanceBuffers : m_InstanceBuffers)
	{
		for (IKVertexBufferPtr& instanceBuffer : instanceBuffers)
		{
			SAFE_UNINIT(instanceBuffer);
		}
		instanceBuffers.clear();
	}
	m_InstanceBuffers.clear();

	SAFE_UNINIT(m_VertexBuffer);
	SAFE_UNINIT(m_IndexBuffer);

	if (m_VertexShader)
	{
		KRenderGlobal::ShaderManager.Release(m_VertexShader);
	}
	if (m_FragmentShader)
	{
		KRenderGlobal::ShaderManager.Release(m_FragmentShader);
	}

	SAFE_UNINIT(m_CommandPool);

	m_VertexData.Clear();
	m_IndexData.Clear();

	m_Device = nullptr;

	return true;
}

bool KOcclusionBox::Reset(size_t frameIndex, std::vector<KRenderComponent*>& cullRes, IKCommandBufferPtr primaryCommandBuffer)
{
	if (m_Enable)
	{
		for (KRenderComponent* render : cullRes)
		{
			IKQueryPtr ocQuery = render->GetOCQuery(frameIndex);
			if (ocQuery)
			{
				QueryStatus status = ocQuery->GetStatus();
				assert(status != QS_INVAILD);
				if (status == QS_IDEL || status == QS_QUERY_END)
				{
					primaryCommandBuffer->ResetQuery(ocQuery);
				}
			}
		}
	}
	return true;
}

IKVertexBufferPtr KOcclusionBox::SafeGetInstanceBuffer(size_t frameIndex, size_t idx, size_t instanceCount)
{
	if (frameIndex < m_InstanceBuffers.size())
	{
		FrameInstanceBufferList& frameInstanceBuffer = m_InstanceBuffers[frameIndex];

		if (frameInstanceBuffer.size() <= idx)
		{
			frameInstanceBuffer.resize(idx + 1);
		}

		IKVertexBufferPtr& buffer = frameInstanceBuffer[idx];

		if (!buffer || buffer->GetVertexCount() < instanceCount)
		{
			if (!buffer)
			{
				m_Device->CreateVertexBuffer(buffer);
			}
			else
			{
				buffer->UnInit();
			}
			buffer->InitMemory(instanceCount, sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F), nullptr);
			buffer->InitDevice(true);
		}
		return buffer;
	}
	return nullptr;
}

bool KOcclusionBox::SafeFillInstanceData(IKVertexBufferPtr buffer, std::vector<KRenderComponent*>& renderComponents)
{
	if (buffer && !renderComponents.empty())
	{
		size_t dataSize = sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F) * renderComponents.size();
		if (dataSize > buffer->GetBufferSize())
		{
			return false;
		}

		static_assert(sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F) == sizeof(glm::mat4), "Size mush match");

		std::vector<glm::mat4> transforms;
		transforms.reserve(renderComponents.size());

		for (KRenderComponent* render : renderComponents)
		{
			IKEntity* entity = render->GetEntityHandle();
			KAABBBox bound;
			entity->GetBound(bound);
			transforms.push_back(glm::translate(glm::mat4(1.0f), bound.GetCenter()) * glm::scale(glm::mat4(1.0f), bound.GetExtend()));
		}

		void* pData = nullptr;
		buffer->Map(&pData);
		memcpy(pData, transforms.data(), dataSize);
		buffer->UnMap();
		return true;
	}
	return false;
}

bool KOcclusionBox::MergeInstanceGroup(KRenderComponent* renderComponent, const KAABBBox& bound, InstanceGroupList& groups)
{
	if (renderComponent)
	{
		for (InstanceGroup& group : groups)
		{
			KAABBBox mergedBox;
			group.bound.Merge(bound, mergedBox);
			glm::vec3 extend = mergedBox.GetExtend();

			if (extend.x <= m_InstanceGroupSize &&
				extend.y <= m_InstanceGroupSize &&
				extend.z <= m_InstanceGroupSize)
			{
				group.renderComponents.push_back(renderComponent);
				group.bound = mergedBox;
				return true;
			}
		}

		InstanceGroup newGroup;
		newGroup.renderComponents.push_back(renderComponent);
		newGroup.bound = bound;
		groups.push_back(std::move(newGroup));

		return true;
	}
	return false;
}

bool KOcclusionBox::MergeInstanceMap(KRenderComponent* renderComponent, const KAABBBox& bound, MeshInstanceMap& map)
{
	if (renderComponent)
	{
		KMeshPtr mesh = renderComponent->GetMesh();
		if (mesh)
		{
			auto it = map.find(mesh);
			if (it == map.end())
			{
				it = map.insert(MeshInstanceMap::value_type(mesh, InstanceGroupList())).first;
			}
			InstanceGroupList& groups = it->second;
			return MergeInstanceGroup(renderComponent, bound, groups);
		}
	}
	return false;
}

bool KOcclusionBox::Render(size_t frameIndex, IKRenderTargetPtr target, std::vector<KRenderComponent*>& cullRes, std::vector<IKCommandBufferPtr>& buffers)
{
	if (frameIndex < m_CommandBuffers.size())
	{
		if (m_Enable)
		{
			IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];

			commandBuffer->BeginSecondary(target);
			commandBuffer->SetViewport(target);

			MeshInstanceMap instanceMap;

			std::unordered_map<IKQueryPtr, std::vector<KRenderComponent*>> queryToComponents;

			for (KRenderComponent* render : cullRes)
			{
				auto AddIntoQueryComponents = [&queryToComponents](IKQueryPtr query, KRenderComponent* component)
				{
					auto it = queryToComponents.find(query);
					if (it == queryToComponents.end())
					{
						it = queryToComponents.insert({ query , std::vector<KRenderComponent*>() }).first;
					}
					it->second.push_back(component);
				};

				IKQueryPtr ocQuery = render->GetOCQuery(frameIndex);
				IKQueryPtr ocInstanceQuery = render->GetOCInstacneQuery(frameIndex);

				if (ocInstanceQuery && (ocInstanceQuery->GetStatus() == QS_QUERY_START || ocInstanceQuery->GetStatus() == QS_QUERYING))
				{
					AddIntoQueryComponents(ocInstanceQuery, render);
				}
				else
				{
					render->SetOCInstanceQuery(frameIndex, nullptr);
					if (ocQuery)
					{
						AddIntoQueryComponents(ocQuery, render);
					}
					else
					{
						render->SetOcclusionVisible(true);
					}
				}
			}

			for (auto& pair : queryToComponents)
			{
				IKQueryPtr query = pair.first;
				auto& componentList = pair.second;
				QueryStatus status = query->GetStatus();
				assert(status != QS_INVAILD);
				if (status == QS_IDEL)
				{
					for (KRenderComponent* render : componentList)
					{
						render->SetOCInstanceQuery(frameIndex, nullptr);

						IKEntity* entity = render->GetEntityHandle();
						KAABBBox bound;
						entity->GetBound(bound);
						MergeInstanceMap(render, bound, instanceMap);
					}
				}
				else if (status == QS_QUERY_START || status == QS_QUERYING)
				{
					if (status == QS_QUERY_START)
					{
						for (KRenderComponent* render : componentList)
						{
							render->SetOcclusionVisible(false);
						}
					}

					constexpr const float MAX_QUERY_TIME = 0.5f;

					uint32_t samples = 0;
					query->GetResultAsync(samples);
					if (samples)
					{
						for (KRenderComponent* render : componentList)
						{
							render->SetOcclusionVisible(true);
						}
					}
					else
					{
						float timeElapse = query->GetElapseTime();
						//KG_LOGD(LM_RENDER, "Query time elapse %.2fs", timeElapse);
						if (timeElapse > MAX_QUERY_TIME)
						{
							for (KRenderComponent* render : componentList)
							{
								render->SetOcclusionVisible(true);
							}
							query->Abort();
						}
					}
				}
				else if (status == QS_QUERY_END)
				{
					KG_LOGE_ASSERT(LM_RENDER, "Should not reach");
				}
			}

			size_t instanceIdx = 0;
			for (auto& meshGroupPair : instanceMap)
			{
				InstanceGroupList& groups = meshGroupPair.second;
				for (InstanceGroup& group : groups)
				{
					std::vector<KRenderComponent*>& renderComponents = group.renderComponents;
					assert(!renderComponents.empty());
					if (renderComponents.size() == 1)
					{
						KRenderComponent* render = renderComponents[0];
						IKQueryPtr ocQuery = render->GetOCQuery(frameIndex);
						QueryStatus status = ocQuery->GetStatus();
						assert(status == QS_IDEL);

						commandBuffer->BeginQuery(ocQuery);
						{
							KRenderCommand command;
							command.vertexData = &m_VertexData;
							command.indexData = &m_IndexData;
							command.indexDraw = true;

							KConstantDefinition::OBJECT transform;
							KAABBBox &bound = group.bound;
							transform.MODEL = glm::translate(glm::mat4(1.0f), bound.GetCenter()) * glm::scale(glm::mat4(1.0f), bound.GetExtend());
							command.SetObjectData(transform);
							// Front Face Pass
							{
								command.pipeline = m_PipelinesFrontFace[frameIndex];
								command.pipeline->GetHandle(target, command.pipelineHandle);
								commandBuffer->Render(command);
							}
							// Back Face Pass
							{
								command.pipeline = m_PipelinesBackFace[frameIndex];
								command.pipeline->GetHandle(target, command.pipelineHandle);
								commandBuffer->Render(command);
							}
						}
						commandBuffer->EndQuery(ocQuery);
					}
					else if (renderComponents.size() > 1)
					{
						KVertexData instacneVertexData = m_VertexData;
						KRenderCommand command;
						command.vertexData = &instacneVertexData;
						command.indexData = &m_IndexData;
						command.indexDraw = true;

						command.instanceDraw = true;
						command.instanceCount = (uint32_t)renderComponents.size();
						command.instanceBuffer = SafeGetInstanceBuffer(frameIndex, instanceIdx, command.instanceCount);

						ASSERT_RESULT(SafeFillInstanceData(command.instanceBuffer, renderComponents));

						KRenderComponent* render = renderComponents[0];
						IKQueryPtr ocQuery = render->GetOCQuery(frameIndex);
						QueryStatus status = ocQuery->GetStatus();
						assert(status == QS_IDEL);

						for (KRenderComponent* render : renderComponents)
						{
							render->SetOCInstanceQuery(frameIndex, ocQuery);
						}

						commandBuffer->BeginQuery(ocQuery);

						// Front Face Pass
						{
							command.pipeline = m_PipelinesInstanceFrontFace[frameIndex];
							command.pipeline->GetHandle(target, command.pipelineHandle);
							commandBuffer->Render(command);
						}
						// Back Face Pass
						{
							command.pipeline = m_PipelinesInstanceBackFace[frameIndex];
							command.pipeline->GetHandle(target, command.pipelineHandle);
							commandBuffer->Render(command);
						}

						commandBuffer->EndQuery(ocQuery);

						++instanceIdx;
					}
				}
			}

			commandBuffer->End();

			buffers.push_back(commandBuffer);
		}
		else
		{
			for (KRenderComponent* render : cullRes)
			{
				render->SetOcclusionVisible(true);
			}
		}
		return true;
	}
	return false;
}