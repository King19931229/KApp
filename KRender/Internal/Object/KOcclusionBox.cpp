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
	m_DepthBiasConstant(-3.25f),
	m_DepthBiasSlope(-3.25f),
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
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/occlusion.vert", m_VertexShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/occlusioninstance.vert", m_VertexInstanceShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "others/occlusion.frag", m_FragmentShader, false));

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

//#define DEBUG_OCCLUSION_BOX

	{
		IKPipelinePtr& pipeline = m_PipelineFrontFace;
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, false, true);
		pipeline->SetShader(ST_VERTEX, *m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, *m_FragmentShader);

		pipeline->SetDepthBiasEnable(true);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_ALWAYS, SO_KEEP, SO_INC, SO_KEEP);
#ifdef DEBUG_OCCLUSION_BOX
		pipeline->SetColorWrite(true, false, false, false);
#else
		pipeline->SetColorWrite(false, false, false, false);
#endif

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}

	{
		IKPipelinePtr& pipeline = m_PipelineInstanceFrontFace;
		pipeline->SetVertexBinding(ms_VertexInstanceFormats, ARRAY_SIZE(ms_VertexInstanceFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, false, true);
		pipeline->SetShader(ST_VERTEX, *m_VertexInstanceShader);
		pipeline->SetShader(ST_FRAGMENT, *m_FragmentShader);

		pipeline->SetDepthBiasEnable(true);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_ALWAYS, SO_KEEP, SO_INC, SO_KEEP);

#ifdef DEBUG_OCCLUSION_BOX
		pipeline->SetColorWrite(true, false, false, false);
#else
		pipeline->SetColorWrite(false, false, false, false);
#endif

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}

	{
		IKPipelinePtr& pipeline = m_PipelineBackFace;
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_FRONT);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_GREATER, false, true);
		pipeline->SetShader(ST_VERTEX, *m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, *m_FragmentShader);

		pipeline->SetDepthBiasEnable(true);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_EQUAL, SO_DEC, SO_KEEP, SO_KEEP);

#ifdef DEBUG_OCCLUSION_BOX
		pipeline->SetColorWrite(true, false, false, false);
#else
		pipeline->SetColorWrite(false, false, false, false);
#endif

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
	}

	{
		IKPipelinePtr& pipeline = m_PipelineInstanceBackFace;
		pipeline->SetVertexBinding(ms_VertexInstanceFormats, ARRAY_SIZE(ms_VertexInstanceFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_FRONT);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_GREATER, false, true);
		pipeline->SetShader(ST_VERTEX, *m_VertexInstanceShader);
		pipeline->SetShader(ST_FRAGMENT, *m_FragmentShader);

		pipeline->SetDepthBiasEnable(true);

		pipeline->SetStencilEnable(true);
		pipeline->SetStencilRef(0);
		pipeline->SetStencilFunc(CF_EQUAL, SO_DEC, SO_KEEP, SO_KEEP);

#ifdef DEBUG_OCCLUSION_BOX
		pipeline->SetColorWrite(true, false, false, false);
#else
		pipeline->SetColorWrite(false, false, false, false);
#endif

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

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

bool KOcclusionBox::Init(IKRenderDevice* renderDevice)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(UnInit());

	uint32_t frameInFlight = KRenderGlobal::NumFramesInFlight;

	m_Device = renderDevice;

	renderDevice->CreateVertexBuffer(m_VertexBuffer);
	renderDevice->CreateIndexBuffer(m_IndexBuffer);

	m_InstanceBuffers.resize(frameInFlight);

	KRenderGlobal::RenderDevice->CreatePipeline(m_PipelineFrontFace);
	KRenderGlobal::RenderDevice->CreatePipeline(m_PipelineBackFace);

	KRenderGlobal::RenderDevice->CreatePipeline(m_PipelineInstanceFrontFace);
	KRenderGlobal::RenderDevice->CreatePipeline(m_PipelineInstanceBackFace);

	LoadResource();
	PreparePipeline();
	InitRenderData();

	return true;
}

bool KOcclusionBox::UnInit()
{
	SAFE_UNINIT(m_PipelineFrontFace);
	SAFE_UNINIT(m_PipelineBackFace);
	SAFE_UNINIT(m_PipelineInstanceFrontFace);
	SAFE_UNINIT(m_PipelineInstanceBackFace);

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

	m_VertexShader.Release();
	m_VertexInstanceShader.Release();
	m_FragmentShader.Release();

	m_VertexData.Reset();
	m_IndexData.Reset();

	m_Device = nullptr;

	return true;
}

bool KOcclusionBox::Reset(std::vector<KRenderComponent*>& cullRes, IKCommandBufferPtr primaryCommandBuffer)
{
	if (m_Enable)
	{
		for (KRenderComponent* render : cullRes)
		{
			IKQueryPtr ocQuery = render->GetOCQuery();
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

bool KOcclusionBox::SafeFillInstanceData(IKVertexBufferPtr buffer, std::vector<KRenderComponent*>& renderComponents)
{
	if (buffer && !renderComponents.empty())
	{
		size_t dataSize = sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F) * renderComponents.size();
		if (dataSize > buffer->GetBufferSize())
		{
			return false;
		}

		std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F> transforms;
		transforms.reserve(renderComponents.size());

		for (KRenderComponent* render : renderComponents)
		{
			IKEntity* entity = render->GetEntityHandle();
			KAABBBox bound;
			entity->GetBound(bound);
			glm::mat4 mat = transpose(glm::translate(glm::mat4(1.0f), bound.GetCenter()) * glm::scale(glm::mat4(1.0f), bound.GetExtend()));
			transforms.push_back({ mat[0], mat[1], mat[2], mat[0], mat[1], mat[2] });
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
			mergedBox = group.bound.Merge(bound);
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

bool KOcclusionBox::Render(IKCommandBufferPtr commandBuffer, IKRenderPassPtr renderPass, const KCamera* camera, std::vector<KRenderComponent*>& cullRes, std::vector<IKCommandBufferPtr>& buffers)
{
	if (camera)
	{
		if (m_Enable)
		{
			commandBuffer->SetViewport(renderPass->GetViewPort());
			// 解决Z-Fighting问题
			commandBuffer->SetDepthBias(m_DepthBiasConstant, 0.0f, m_DepthBiasSlope);

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

				IKQueryPtr ocInstanceQuery = render->GetOCInstacneQuery();

				// 注意一定要检查ocInstanceQuery的生命周期
				if (ocInstanceQuery && (ocInstanceQuery->GetStatus() == QS_QUERY_START || ocInstanceQuery->GetStatus() == QS_QUERYING))
				{
					AddIntoQueryComponents(ocInstanceQuery, render);
				}
				else
				{
					IKQueryPtr ocQuery = render->GetOCQuery();
					render->SetOCInstanceQuery(nullptr);
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
						render->SetOCInstanceQuery(nullptr);

						IKEntity* entity = render->GetEntityHandle();
						KAABBBox bound;
						entity->GetBound(bound);

#define DONT_CARE_CAMERA_INSIDE_OBJECT
						bound.InitFromHalfExtent(bound.GetCenter(), bound.GetExtend() * 0.5f);

						// 这里只能把物件先绘制上去 否则如果深度不写入
						// Occlusion背面Pass查询将会一直失败导致物件永远绘制不上去
						if (bound.Intersect(camera->GetPosition()))
						{
							render->SetOcclusionVisible(true);
						}
						// 这里干脆只把相机不在物件范围内的物件做Occlusion查询
						// 否则帧率会一直波动
#ifdef DONT_CARE_CAMERA_INSIDE_OBJECT
						else
#endif
						{
							MergeInstanceMap(render, bound, instanceMap);
						}
					}
				}
				else if (status == QS_QUERY_START || status == QS_QUERYING)
				{
					uint32_t samples = 0;
					bool success = query->GetResultSync(samples);
					if (samples)
					{
						for (KRenderComponent* render : componentList)
						{
							render->SetOcclusionVisible(true);
						}
						query->Abort();
					}
					else
					{
						if (success)
						{
							for (KRenderComponent* render : componentList)
							{
								render->SetOcclusionVisible(false);
							}
						}
						else
						{
							constexpr const float MAX_QUERY_TIME = 0.5f;
							float timeElapse = query->GetElapseTime();
							// KG_LOGD(LM_RENDER, "Query time elapse %.2fs", timeElapse);
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
						IKQueryPtr ocQuery = render->GetOCQuery();
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
							transform.PRVE_MODEL = transform.MODEL = glm::translate(glm::mat4(1.0f), bound.GetCenter()) * glm::scale(glm::mat4(1.0f), bound.GetExtend());

							command.objectUsage.binding = SHADER_BINDING_OBJECT;
							command.objectUsage.range = sizeof(transform);
							KRenderGlobal::DynamicConstantBufferManager.Alloc(&transform, command.objectUsage);

							// Front Face Pass
							{
								command.pipeline = m_PipelineFrontFace;
								command.pipeline->GetHandle(renderPass, command.pipelineHandle);
								commandBuffer->Render(command);
							}
							// Back Face Pass
							{
								command.pipeline = m_PipelineBackFace;
								command.pipeline->GetHandle(renderPass, command.pipelineHandle);
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

						{
							std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F> transforms;
							transforms.reserve(renderComponents.size());

							for (KRenderComponent* render : renderComponents)
							{
								IKEntity* entity = render->GetEntityHandle();
								KAABBBox bound;
								entity->GetBound(bound);

								glm::mat4 transform = glm::translate(glm::mat4(1.0f), bound.GetCenter()) * glm::scale(glm::mat4(1.0f), bound.GetExtend());
								transform = glm::transpose(transform);
								transforms.push_back({ transform[0], transform[1], transform[2],transform[0], transform[1], transform[2] });
							}

							std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
							ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(transforms[0]));
							ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(transforms.size(), transforms.data(), allocRes));

							command.instanceDraw = true;
							command.instanceUsages.resize(allocRes.size());
							for (size_t i = 0; i < allocRes.size(); ++i)
							{
								KInstanceBufferUsage& usage = command.instanceUsages[i];
								KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
								usage.buffer = allocResult.buffer;
								usage.start = allocResult.start;
								usage.count = allocResult.count;
								usage.offset = allocResult.offset;
							}
						}

						KRenderComponent* render = renderComponents[0];
						IKQueryPtr ocQuery = render->GetOCQuery();
						QueryStatus status = ocQuery->GetStatus();
						assert(status == QS_IDEL);

						for (KRenderComponent* render : renderComponents)
						{
							render->SetOCInstanceQuery(ocQuery);
						}

						commandBuffer->BeginQuery(ocQuery);

						// Front Face Pass
						{
							command.pipeline = m_PipelineInstanceFrontFace;
							command.pipeline->GetHandle(renderPass, command.pipelineHandle);
							commandBuffer->Render(command);
						}
						// Back Face Pass
						{
							command.pipeline = m_PipelineInstanceBackFace;
							command.pipeline->GetHandle(renderPass, command.pipelineHandle);
							commandBuffer->Render(command);
						}

						commandBuffer->EndQuery(ocQuery);

						++instanceIdx;
					}
				}
			}

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