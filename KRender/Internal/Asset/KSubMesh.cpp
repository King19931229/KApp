#include "KSubMesh.h"
#include "KMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"

#include "Internal/KRenderGlobal.h"

KSubMesh::KSubMesh(KMesh* parent, size_t mtlIndex)
	: m_pParent(parent),
	m_MaterialIndex(mtlIndex),
	m_FrameInFlight(0),
	m_IndexDraw(true)
{

}

KSubMesh::~KSubMesh()
{

}

bool KSubMesh::Init(const KVertexData& vertexData, const KIndexData& indexData, size_t frameInFlight)
{
	UnInit();

	m_VertexData = vertexData;
	m_IndexData = indexData;
	m_FrameInFlight = frameInFlight;

	m_ObjectConstant.shaderTypes = ST_VERTEX;
	m_ObjectConstant.size = (int)KConstantDefinition::GetConstantBufferDetail(CBT_OBJECT).bufferSize;

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		pipelines.resize(m_FrameInFlight);
		for(IKPipelinePtr& pipeline : pipelines)
		{
			pipeline = nullptr;
		}
	}

	return true;
}

bool KSubMesh::UnInit()
{
	m_VertexData.Destroy();
	m_IndexData.Destroy();
	m_FrameInFlight = 0;

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		for(IKPipelinePtr& pipeline : pipelines)
		{
			pipeline->UnInit();
			pipeline = nullptr;
		}
		pipelines.clear();
	}

	return true;
}

bool KSubMesh::CreatePipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline)
{
	if(stage == PIPELINE_STAGE_OPAQUE)
	{
		std::vector<VertexInputDetail> bindingDetails;
		bindingDetails.resize(m_VertexData.vertexFormats.size());

		// 每个format占用一个buffer
		for(size_t i = 0; i < bindingDetails.size(); ++i)
		{
			VertexInputDetail& inputDetail = bindingDetails[i];
			inputDetail.formats = (m_VertexData.vertexFormats.data() + i);
		}
		pipeline->SetVertexBinding(bindingDetails.data(), bindingDetails.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		/*
		pipeline->SetShader(ST_VERTEX, m_SceneVertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_SceneFragmentShader);
		*/

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(0, ST_VERTEX, cameraBuffer);

		/*
		pipeline->SetSampler(1, m_Texture->GetImageView(), m_Sampler);
		pipeline->SetSampler(2, m_SkyBox.GetCubeTexture()->GetImageView(), m_SkyBox.GetSampler());
		*/

		pipeline->PushConstantBlock(m_ObjectConstant, m_ObjectConstantLoc);

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

	if(pipeline == nullptr)
	{
		return CreatePipeline(stage, frameIndex, pipeline);
	}
	return false;
}

bool KSubMesh::AppendRenderList(PipelineStage stage, size_t frameIndex,	KRenderCommandList& list)
{
	KRenderCommand command;
	if(GetRenderCommand(stage, frameIndex, command))
	{
		list.push_back(std::move(command));
		return true;
	}

	return false;
}