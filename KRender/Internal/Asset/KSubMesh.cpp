#include "KSubMesh.h"
#include "KMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"

#include "Internal/KRenderGlobal.h"

KSubMesh::KSubMesh(KMesh* parent)
	: m_pParent(parent),
	m_pVertexData(nullptr),
	m_Material(nullptr),
	m_FrameInFlight(0),
	m_RenderThreadNum(0),
	m_IndexDraw(true)
{

}

KSubMesh::~KSubMesh()
{

}

bool KSubMesh::Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialPtr material, size_t frameInFlight, size_t renderThreadNum)
{
	UnInit();

	m_pVertexData = vertexData;
	m_IndexData = indexData;
	m_FrameInFlight = frameInFlight;
	m_RenderThreadNum = renderThreadNum;

	m_Material = material;

	// hard code for now
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/diffuse.vert", m_SceneVSShader));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/diffuse.frag", m_SceneFSShader));

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		pipelines.resize(m_FrameInFlight);
		for(size_t frameIndex = 0; frameIndex < m_FrameInFlight; ++frameIndex)
		{
			PipelineList& threadPipelines = pipelines[frameIndex];
			threadPipelines.resize(m_RenderThreadNum);

			for(size_t threadIndex = 0; threadIndex < m_RenderThreadNum; ++threadIndex)
			{
				PipelineInfo& info = threadPipelines[threadIndex];
				CreatePipeline((PipelineStage)i, frameIndex, threadIndex, info.pipeline, info.objectPushOffset);
			}
		}
	}

	return true;
}

bool KSubMesh::UnInit()
{
	m_pVertexData = nullptr;
	m_IndexData.Destroy();
	m_FrameInFlight = 0;

	if(m_SceneVSShader)
	{
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(m_SceneVSShader));
		m_SceneVSShader = nullptr;
	}
	if(m_SceneFSShader)
	{
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(m_SceneFSShader));
		m_SceneFSShader = nullptr;
	}

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];

		for(PipelineList& framePipelineList : pipelines)
		{
			for(PipelineInfo& info : framePipelineList)
			{
				if(info.pipeline)
				{
					KRenderGlobal::PipelineManager.DestroyPipeline(info.pipeline);
					info.pipeline = nullptr;
				}
			}
			framePipelineList.clear();
		}
		pipelines.clear();
	}

	if(m_Material)
	{
		m_Material->UnInit();
		m_Material = nullptr;
	}

	return true;
}

bool KSubMesh::CreatePipeline(PipelineStage stage, size_t frameIndex, size_t renderThreadIndex, IKPipelinePtr& pipeline, uint32_t& objectPushOffset)
{
	assert(m_pVertexData);
	if(!m_pVertexData)
	{
		return false;
	}

	if(stage == PIPELINE_STAGE_OPAQUE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, m_SceneVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_SceneFSShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->PushConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT), objectPushOffset);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, renderThreadIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		if(m_Material)
		{
			// hard code for now
			// 0:diffuse 1:specular 2:normal
			KMaterialTextureInfo diffuseMap = m_Material->GetTexture(0);
			KMaterialTextureInfo specularMap = m_Material->GetTexture(1);
			KMaterialTextureInfo normalMap = m_Material->GetTexture(2);

			if(diffuseMap.texture && diffuseMap.sampler)
			{
				pipeline->SetSampler(CBT_COUNT, diffuseMap.texture->GetImageView(), diffuseMap.sampler);
			}
			else
			{
				KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
				pipeline = nullptr;
				return false;
			}
		}

		pipeline->Init();
		return true;
	}
	else
	{
		assert(false && "not impl");
		return false;
	}
}

bool KSubMesh::GetRenderCommand(PipelineStage stage, size_t frameIndex, size_t renderThreadIndex, KRenderCommand& command)
{
	if(stage >= PIPELINE_STAGE_COUNT)
	{
		assert(false && "pipeline stage count out of bound");
		return false;
	}
	if(frameIndex >= m_FrameInFlight)
	{
		assert(false && "frame index out of bound");
		return false;
	}

	FramePipelineList& pipelines = m_Pipelines[stage];
	assert(frameIndex < pipelines.size());

	PipelineList& thredPipelines = pipelines[frameIndex];
	assert(renderThreadIndex < thredPipelines.size());

	PipelineInfo& info = thredPipelines[renderThreadIndex];
	if(info.pipeline)
	{
		command.vertexData = m_pVertexData;
		command.indexData = &m_IndexData;
		command.pipeline = info.pipeline.get();
		command.indexDraw = true;

		command.objectPushOffset = info.objectPushOffset;
		command.useObjectData = false;

		return true;
	}
	else
	{
		return false;
	}
}

bool KSubMesh::Visit(PipelineStage stage, size_t frameIndex, size_t threadIndex, std::function<void(KRenderCommand)> func)
{
	KRenderCommand command;
	if(GetRenderCommand(stage, frameIndex, threadIndex, command))
	{
		func(command);
		return true;
	}
	return false;
}