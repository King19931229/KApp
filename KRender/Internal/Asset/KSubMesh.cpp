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

	m_IndexDraw = true;

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/prez.vert", m_PreZVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/prez.frag", m_PreZFSShader, true));

	// TODO fetch it from material?
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/diffuse.vert", m_SceneVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/diffuse.frag", m_SceneFSShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/shadow.vert", m_ShadowVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/shadow.frag", m_ShadowFSShader, true));

	for(size_t i = 0; i < PIPELINE_STAGE_DEBUG_LINE; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		pipelines.resize(m_FrameInFlight);
		for (size_t frameIndex = 0; frameIndex < m_FrameInFlight; ++frameIndex)
		{
			IKPipelinePtr& pipeline = pipelines[frameIndex];
			assert(!pipeline);
			CreatePipeline((PipelineStage)i, frameIndex, pipeline);
		}
	}

	return true;
}

bool KSubMesh::InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, size_t frameInFlight)
{
	UnInit();

	m_pVertexData = vertexData;
	m_FrameInFlight = frameInFlight;

	if (indexData != nullptr)
	{
		m_IndexDraw = true;
		m_IndexData = *indexData;
	}
	else
	{
		m_IndexDraw = false;
	}

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/debug.vert", m_DebugVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire("Shaders/debug.frag", m_DebugFSShader, true));

	PipelineStage debugStage = PIPELINE_STAGE_UNKNOWWN;
	switch (primtive)
	{
	case DEBUG_PRIMITIVE_LINE:
		debugStage = PIPELINE_STAGE_DEBUG_LINE;
		break;
	case DEBUG_PRIMITIVE_TRIANGLE:
		debugStage = PIPELINE_STAGE_DEBUG_TRIANGLE;
		break;
	default:
		assert(false && "impossible to reach");
		break;
	}

	FramePipelineList& pipelines = m_Pipelines[debugStage];
	pipelines.resize(m_FrameInFlight);
	for (size_t frameIndex = 0; frameIndex < m_FrameInFlight; ++frameIndex)
	{
		IKPipelinePtr& pipeline = pipelines[frameIndex];
		assert(pipeline == nullptr);
		CreatePipeline(debugStage, frameIndex, pipeline);
	}

	return true;
}

bool KSubMesh::UnInit()
{
	m_pVertexData = nullptr;
	m_IndexData.Destroy();
	m_FrameInFlight = 0;

#define SAFE_RELEASE_SHADER(shader)\
do\
{\
	if(shader)\
	{\
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(shader));\
		shader = nullptr;\
	}\
}while(0);\

	SAFE_RELEASE_SHADER(m_DebugVSShader);
	SAFE_RELEASE_SHADER(m_DebugFSShader);

	SAFE_RELEASE_SHADER(m_PreZVSShader);
	SAFE_RELEASE_SHADER(m_PreZFSShader);

	SAFE_RELEASE_SHADER(m_SceneVSShader);
	SAFE_RELEASE_SHADER(m_SceneFSShader);

	SAFE_RELEASE_SHADER(m_ShadowVSShader);
	SAFE_RELEASE_SHADER(m_ShadowFSShader);

#undef SAFE_RELEASE_SHADER

	for(size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		for (IKPipelinePtr& pipeline : pipelines)
		{
			if (pipeline)
			{
				KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
				pipeline = nullptr;
			}
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
/*
// 安卓上还是有用的
#ifndef __ANDROID__
#	define PRE_Z_DISABLE
#endif
*/

bool KSubMesh::CreatePipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline)
{
	assert(m_pVertexData);
	if(!m_pVertexData)
	{
		return false;
	}

	if(stage == PIPELINE_STAGE_PRE_Z)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);
		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, m_PreZVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_PreZFSShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetColorWrite(false, false, false, false);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init(true));
		return true;
	}
	else if(stage == PIPELINE_STAGE_OPAQUE)
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

		pipeline->SetColorWrite(true, true, true, true);
#ifdef PRE_Z_DISABLE
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);
#else
		pipeline->SetDepthFunc(CF_EQUAL, false, true);
#endif

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		IKUniformBufferPtr shaodwBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_SHADOW);
		pipeline->SetConstantBuffer(CBT_SHADOW, ST_VERTEX | ST_FRAGMENT, shaodwBuffer);

		bool diffuseReady = false;

		if(m_Material)
		{
			KMaterialTextureInfo diffuseMap = m_Material->GetTexture(MTS_DIFFUSE);
			KMaterialTextureInfo specularMap = m_Material->GetTexture(MTS_SPECULAR);
			KMaterialTextureInfo normalMap = m_Material->GetTexture(MTS_NORMAL);

			if(diffuseMap.texture && diffuseMap.sampler)
			{
				pipeline->SetSampler(CBT_COUNT, diffuseMap.texture, diffuseMap.sampler);
				diffuseReady = true;
			}
		}

		IKRenderTargetPtr shadowTarget = KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex);
		if(shadowTarget)
		{
			pipeline->SetSamplerDepthAttachment(CBT_COUNT + 3, shadowTarget, KRenderGlobal::ShadowMap.GetSampler());
		}

		if(!diffuseReady)
		{
			KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
			pipeline = nullptr;
			return false;
		}

		ASSERT_RESULT(pipeline->Init(true));
		return true;
	}
	else if(stage == PIPELINE_STAGE_SHADOW_GEN)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(false, false, false, false);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetDepthBiasEnable(true);

		pipeline->SetShader(ST_VERTEX, m_ShadowVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_ShadowFSShader);

		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_SHADOW);
		pipeline->SetConstantBuffer(CBT_SHADOW, ST_VERTEX, shadowBuffer);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		ASSERT_RESULT(pipeline->Init(true));
		return true;
	}
	else if (stage == PIPELINE_STAGE_DEBUG_LINE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_LINE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_LINE);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, true);

		pipeline->SetDepthBiasEnable(false);

		pipeline->SetShader(ST_VERTEX, m_DebugVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_DebugFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		ASSERT_RESULT(pipeline->Init(true));
		return true;
	}
	else if (stage == PIPELINE_STAGE_DEBUG_TRIANGLE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_ALWAYS, false, true);

		pipeline->SetDepthBiasEnable(false);

		pipeline->SetShader(ST_VERTEX, m_DebugVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_DebugFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX, cameraBuffer);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		ASSERT_RESULT(pipeline->Init(true));
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
#ifdef PRE_Z_DISABLE
	if(stage == PIPELINE_STAGE_PRE_Z)
	{
		return false;
	}
#endif

	FramePipelineList& pipelines = m_Pipelines[stage];
	if (frameIndex >= pipelines.size())
	{
		return false;
	}

	IKPipelinePtr& pipeline = pipelines[frameIndex];
	if(pipeline)
	{
		command.vertexData = m_pVertexData;
		command.indexData = &m_IndexData;
		command.pipeline = pipeline;
		command.indexDraw = m_IndexDraw;
		command.objectData = nullptr;

		return true;
	}
	else
	{
		return false;
	}
}

bool KSubMesh::Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand)> func)
{
	KRenderCommand command;
	if(GetRenderCommand(stage, frameIndex, command))
	{
		func(command);
		return true;
	}
	return false;
}