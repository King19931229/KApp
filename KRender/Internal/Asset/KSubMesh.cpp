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
	m_IndexDraw(true)
{

}

KSubMesh::~KSubMesh()
{

}

bool KSubMesh::Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialPtr material, size_t frameInFlight)
{
	UnInit();

	m_pVertexData = vertexData;
	m_IndexData = indexData;
	m_FrameInFlight = frameInFlight;

	m_Material = material;

	m_ObjectConstant.shaderTypes = ST_VERTEX;
	m_ObjectConstant.size = (int)KConstantDefinition::GetConstantBufferDetail(CBT_OBJECT).bufferSize;

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		pipelines.resize(m_FrameInFlight);
		for(size_t frameIndex = 0; frameIndex < m_FrameInFlight; ++frameIndex)
		{
			ASSERT_RESULT(CreatePipeline((PipelineStage)i, frameIndex, pipelines[frameIndex]));
		}
	}

	return true;
}

bool KSubMesh::UnInit()
{
	m_pVertexData = nullptr;
	m_IndexData.Destroy();
	m_FrameInFlight = 0;

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		for(IKPipelinePtr& pipeline : pipelines)
		{
			KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
			pipeline = nullptr;
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

bool KSubMesh::CreatePipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline)
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

		// hard code for now
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/diffuse.vert", m_SceneVSShader));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/diffuse.frag", m_SceneFSShader));

		pipeline->SetShader(ST_VERTEX, m_SceneVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_SceneFSShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(0, ST_VERTEX, cameraBuffer);

		IKUniformBufferPtr objectBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_OBJECT);
		pipeline->SetConstantBuffer(1, ST_VERTEX, objectBuffer);

		if(m_Material)
		{
			// hard code for now
			// 0:diffuse 1:specular 2:normal
			KMaterialTextureInfo diffuseMap = m_Material->GetTexture(0);
			KMaterialTextureInfo specularMap = m_Material->GetTexture(1);
			KMaterialTextureInfo normalMap = m_Material->GetTexture(2);

			if(diffuseMap.texture && diffuseMap.sampler)
			{
				pipeline->SetSampler(2, diffuseMap.texture->GetImageView(), diffuseMap.sampler);
			}
		}

		//pipeline->PushConstantBlock(m_ObjectConstant, m_ObjectConstantLoc);

		pipeline->Init();
		return true;
	}
	else
	{
		assert(false && "not impl");
		return false;
	}
}

bool KSubMesh::GetRenderCommand(PipelineStage stage, size_t frameIndex, KRenderCommand& command)
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

	IKPipelinePtr& pipeline = pipelines[frameIndex];
	assert(pipeline);

	command.vertexData = m_pVertexData;
	command.indexData = &m_IndexData;
	command.pipeline = pipeline.get();
	command.indexDraw = true;

	return true;
}

bool KSubMesh::AppendRenderList(PipelineStage stage, size_t frameIndex,	KRenderCommandList& list)
{
	KRenderCommand command;
	if(GetRenderCommand(stage, frameIndex, command))
	{
		list.push_back(command);
		return true;
	}

	return false;
}