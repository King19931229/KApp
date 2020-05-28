#include "KOcclusionBox.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

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

KOcclusionBox::KOcclusionBox()
{
}

KOcclusionBox::~KOcclusionBox()
{
}

void KOcclusionBox::LoadResource()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("occlusion.vert", m_VertexShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("occlusion.frag", m_FragmentShader, false));

	m_VertexBuffer->InitMemory(ARRAY_SIZE(ms_Positions), sizeof(ms_Positions[0]), ms_Positions);
	m_VertexBuffer->InitDevice(false);

	m_IndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_Indices), ms_Indices);
	m_IndexBuffer->InitDevice(false);
}

void KOcclusionBox::PreparePipeline()
{
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

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateVertexBuffer(m_VertexBuffer);
	renderDevice->CreateIndexBuffer(m_IndexBuffer);

	m_PipelinesFrontFace.resize(frameInFlight);
	m_PipelinesBackFace.resize(frameInFlight);
	m_CommandBuffers.resize(frameInFlight);

	for (size_t i = 0; i < frameInFlight; ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_PipelinesFrontFace[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_PipelinesBackFace[i]);

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

	for (IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_CommandBuffers.clear();

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

	return true;
}

bool KOcclusionBox::Render(size_t frameIndex, IKRenderTargetPtr target, std::vector<KRenderComponent*>& cullRes, std::vector<IKCommandBufferPtr>& buffers)
{
	if (frameIndex < m_PipelinesFrontFace.size() && frameIndex < m_PipelinesBackFace.size())
	{
		IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];

		commandBuffer->BeginSecondary(target);
		commandBuffer->SetViewport(target);
		
		for (KRenderComponent* render : cullRes)
		{
			IKEntity* entity = render->GetEntityHandle();

			IKQueryPtr ocQuery = render->GetOCQuery(frameIndex);
			QueryStatus status = ocQuery->GetStatus();

			if (status != QS_QUERYING)
			{
				if (status == QS_FINISH)
				{
					commandBuffer->ResetQuery(ocQuery);
				}
				commandBuffer->BeginQuery(ocQuery);
				{
					KRenderCommand command;
					command.vertexData = &m_VertexData;
					command.indexData = &m_IndexData;
					command.indexDraw = true;

					KAABBBox box;
					ASSERT_RESULT(entity->GetBound(box));
					KConstantDefinition::OBJECT transform;
					transform.MODEL = glm::translate(glm::mat4(1.0f), box.GetCenter()) * glm::scale(glm::mat4(1.0f), box.GetExtend());
					command.SetObjectData(transform);

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
				render->SetOcclusionVisible(true);
			}
			else
			{
				uint32_t samples = 0;
				ocQuery->GetResultSync(samples);
				if(samples == 0)
				{
					render->SetOcclusionVisible(false);
				}
				else
				{
					render->SetOcclusionVisible(true);
				}
			}
		}

		commandBuffer->End();

		buffers.push_back(commandBuffer);
		return true;
	}
	return false;
}