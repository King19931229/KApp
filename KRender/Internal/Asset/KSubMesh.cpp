#include "KSubMesh.h"
#include "KMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"

#include "Internal/KRenderGlobal.h"

KSubMesh::KSubMesh(KMesh* parent)
	: m_pParent(parent),
	m_pVertexData(nullptr),
	m_FrameInFlight(0),
	m_IndexDraw(true)
{
}

KSubMesh::~KSubMesh()
{
}

bool KSubMesh::Init(const KVertexData* vertexData, const KIndexData& indexData, KMeshTextureBinding&& binding, size_t frameInFlight)
{
	UnInit();

	m_pVertexData = vertexData;
	m_IndexData = indexData;
	m_FrameInFlight = frameInFlight;

	m_Texture = std::move(binding);

	m_IndexDraw = true;

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "prez.vert", m_PreZVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "prez.frag", m_PreZFSShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "diffuse.vert", m_SceneVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "diffuse.vert", { {"INSTANCE_INPUT", "1"} }, m_SceneVSInstanceShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "diffuse.frag", m_SceneFSShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow.vert", m_ShadowVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow.frag", m_ShadowFSShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "cascadedshadow.vert", m_CascadedShadowVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "cascadedshadowinstance.vert", m_CascadedShadowVSInstanceShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow.frag", m_CascadedShadowFSShader, true));

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

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "debug.vert", m_DebugVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "debug.frag", m_DebugFSShader, true));

	PipelineStage debugStage = PIPELINE_STAGE_UNKNOWN;
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
	SAFE_RELEASE_SHADER(m_SceneVSInstanceShader);
	SAFE_RELEASE_SHADER(m_SceneFSShader);

	SAFE_RELEASE_SHADER(m_ShadowVSShader);
	SAFE_RELEASE_SHADER(m_ShadowFSShader);

	SAFE_RELEASE_SHADER(m_CascadedShadowVSShader);
	SAFE_RELEASE_SHADER(m_CascadedShadowFSShader);

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

	m_Texture.Release();

	return true;
}

// 安卓上还是有用的
#ifndef __ANDROID__
#	define PRE_Z_DISABLE
#endif

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

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if(stage == PIPELINE_STAGE_OPAQUE || stage == PIPELINE_STAGE_OPAQUE_INSTANCE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		if (stage == PIPELINE_STAGE_OPAQUE)
		{
			pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());
			pipeline->SetShader(ST_VERTEX, m_SceneVSShader);
		}
		else if (stage == PIPELINE_STAGE_OPAQUE_INSTANCE)
		{
			std::vector<VertexFormat> instanceFormats = m_pVertexData->vertexFormats;
			instanceFormats.push_back(VF_INSTANCE);
			pipeline->SetVertexBinding(instanceFormats.data(), instanceFormats.size());
			pipeline->SetShader(ST_VERTEX, m_SceneVSInstanceShader);
		}

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
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

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		IKUniformBufferPtr shaodwBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_SHADOW);
		pipeline->SetConstantBuffer(SHADER_BINDING_SHADOW, ST_VERTEX | ST_FRAGMENT, shaodwBuffer);

		IKUniformBufferPtr cascadedShaodwBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CASCADED_SHADOW);
		pipeline->SetConstantBuffer(SHADER_BINDING_CASCADED_SHADOW, ST_VERTEX | ST_FRAGMENT, cascadedShaodwBuffer);

		bool diffuseReady = false;

		KMeshTextureInfo diffuseMap = m_Texture.GetTexture(MTS_DIFFUSE);
		KMeshTextureInfo specularMap = m_Texture.GetTexture(MTS_SPECULAR);
		KMeshTextureInfo normalMap = m_Texture.GetTexture(MTS_NORMAL);

		if (diffuseMap.texture && diffuseMap.sampler)
		{
			pipeline->SetSampler(SHADER_BINDING_DIFFUSE, diffuseMap.texture, diffuseMap.sampler);
			diffuseReady = true;
		}
		else
		{
			IKTexturePtr texture = nullptr;
			KRenderGlobal::TextrueManager.GetErrorTexture(texture);
			IKSamplerPtr sampler = nullptr;
			KRenderGlobal::TextrueManager.GetErrorSampler(sampler);
			pipeline->SetSampler(SHADER_BINDING_DIFFUSE, texture, sampler);
			diffuseReady = true;
		}
		
		if (!diffuseReady)
		{
			KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
			pipeline = nullptr;
			return false;
		}

		for (size_t i = 0; i < KRenderGlobal::CascadedShadowMap.GetNumCascaded(); ++i)
		{
			IKRenderTargetPtr shadowTarget = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(i, frameIndex);
			pipeline->SetSamplerDepthAttachment((unsigned int)(SHADER_BINDING_CSM0 + i), shadowTarget, KRenderGlobal::CascadedShadowMap.GetSampler());
		}

		for (size_t i = KRenderGlobal::CascadedShadowMap.GetNumCascaded(); i < 4; ++i)
		{
			IKRenderTargetPtr shadowTarget = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, frameIndex);
			pipeline->SetSamplerDepthAttachment((unsigned int)(SHADER_BINDING_CSM0 + i), shadowTarget, KRenderGlobal::CascadedShadowMap.GetSampler());
		}

		ASSERT_RESULT(pipeline->Init());
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

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_GEN || stage == PIPELINE_STAGE_CASCADED_SHADOW_GEN_INSTANCE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		if (stage == PIPELINE_STAGE_CASCADED_SHADOW_GEN)
		{
			pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());
			pipeline->SetShader(ST_VERTEX, m_CascadedShadowVSShader);
		}
		else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_GEN_INSTANCE)
		{
			std::vector<VertexFormat> instanceFormats = m_pVertexData->vertexFormats;
			instanceFormats.push_back(VF_INSTANCE);
			pipeline->SetVertexBinding(instanceFormats.data(), instanceFormats.size());
			pipeline->SetShader(ST_VERTEX, m_CascadedShadowVSInstanceShader);
		}

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(false, false, false, false);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetDepthBiasEnable(true);
		pipeline->SetShader(ST_FRAGMENT, m_CascadedShadowFSShader);

		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CASCADED_SHADOW);
		pipeline->SetConstantBuffer(CBT_CASCADED_SHADOW, ST_VERTEX, shadowBuffer);

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if (stage == PIPELINE_STAGE_DEBUG_LINE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_LINE_LIST);
		pipeline->SetBlendEnable(true);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_LINE);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetColorBlend(BF_SRC_ALPHA, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		pipeline->SetDepthFunc(CF_ALWAYS, true, true);

		pipeline->SetDepthBiasEnable(false);

		pipeline->SetShader(ST_VERTEX, m_DebugVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_DebugFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if (stage == PIPELINE_STAGE_DEBUG_TRIANGLE)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);

		pipeline->SetVertexBinding((m_pVertexData->vertexFormats).data(), m_pVertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(true);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetColorBlend(BF_SRC_ALPHA, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		pipeline->SetDepthFunc(CF_ALWAYS, true, true);

		pipeline->SetDepthBiasEnable(false);

		pipeline->SetShader(ST_VERTEX, m_DebugVSShader);
		pipeline->SetShader(ST_FRAGMENT, m_DebugFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
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
		command.objectData.clear();

		return true;
	}
	else
	{
		return false;
	}
}

bool KSubMesh::Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&&)> func)
{
	KRenderCommand command;
	if(GetRenderCommand(stage, frameIndex, command))
	{
		func(std::move(command));
		return true;
	}
	return false;
}