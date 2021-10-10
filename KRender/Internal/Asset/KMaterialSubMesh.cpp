#include "KMaterialSubMesh.h"
#include "KSubMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"

#include "Internal/KRenderGlobal.h"

KMaterialSubMesh::KMaterialSubMesh(KSubMesh* mesh)
	: m_pSubMesh(mesh),
	m_pMaterial(nullptr),
	m_FrameInFlight(0),
	m_MaterialPipelineCreated(false)
{
}

KMaterialSubMesh::~KMaterialSubMesh()
{
}

bool KMaterialSubMesh::CreateFixedPipeline()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/prez.vert", m_PreZVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "others/prez.frag", m_PreZFSShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/shadow.vert", m_ShadowVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/shadow.frag", m_ShadowFSShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/cascadedshadow.vert", m_CascadedShadowVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/cascadedshadowinstance.vert", m_CascadedShadowVSInstanceShader, true));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "gbuffer/gbuffer.vert", m_GBufferVSShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "gbuffer/gbufferinstance.vert", m_GBufferVSInstanceShader, true));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "gbuffer/gbuffer.frag", m_GBufferFSShader, true));

	for (PipelineStage stage : {PIPELINE_STAGE_GBUFFER, PIPELINE_STAGE_GBUFFER_INSTANCE, PIPELINE_STAGE_PRE_Z, PIPELINE_STAGE_SHADOW_GEN, PIPELINE_STAGE_CASCADED_SHADOW_GEN, PIPELINE_STAGE_CASCADED_SHADOW_GEN_INSTANCE})
	{
		FramePipelineList& pipelines = m_Pipelines[stage];
		pipelines.resize(m_FrameInFlight);
		for (size_t frameIndex = 0; frameIndex < m_FrameInFlight; ++frameIndex)
		{
			IKPipelinePtr& pipeline = pipelines[frameIndex];
			assert(!pipeline);
			CreateFixedPipeline(stage, frameIndex, pipeline);
		}
	}

	return true;
}

bool KMaterialSubMesh::Init(IKMaterial* material, size_t frameInFlight)
{
	UnInit();

	ASSERT_RESULT(material);

	m_pMaterial = material;
	m_FrameInFlight = frameInFlight;
	m_MaterialPipelineCreated = false;
	m_MateriaShaderTriggerLoaded = false;

	ASSERT_RESULT(CreateFixedPipeline());

	return true;
}

bool KMaterialSubMesh::InitDebug(DebugPrimitive primtive, size_t frameInFlight)
{
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

	m_FrameInFlight = frameInFlight;

	FramePipelineList& pipelines = m_Pipelines[debugStage];
	pipelines.resize(m_FrameInFlight);
	for (size_t frameIndex = 0; frameIndex < m_FrameInFlight; ++frameIndex)
	{
		IKPipelinePtr& pipeline = pipelines[frameIndex];
		assert(pipeline == nullptr);
		CreateFixedPipeline(debugStage, frameIndex, pipeline);
	}

	return true;
}

bool KMaterialSubMesh::UnInit()
{
	m_pMaterial = nullptr;
	m_FrameInFlight = 0;

#define SAFE_RELEASE_SHADER(shader)\
	if(shader)\
	{\
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(shader));\
		shader = nullptr;\
	}

	SAFE_RELEASE_SHADER(m_DebugVSShader);
	SAFE_RELEASE_SHADER(m_DebugFSShader);

	SAFE_RELEASE_SHADER(m_PreZVSShader);
	SAFE_RELEASE_SHADER(m_PreZFSShader);

	SAFE_RELEASE_SHADER(m_GBufferVSShader);
	SAFE_RELEASE_SHADER(m_GBufferVSInstanceShader);
	SAFE_RELEASE_SHADER(m_GBufferFSShader);

	SAFE_RELEASE_SHADER(m_ShadowVSShader);
	SAFE_RELEASE_SHADER(m_ShadowFSShader);

	SAFE_RELEASE_SHADER(m_CascadedShadowVSShader);
	SAFE_RELEASE_SHADER(m_CascadedShadowVSInstanceShader);

#undef SAFE_RELEASE_SHADER

	for (size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		FramePipelineList& pipelines = m_Pipelines[i];
		for (IKPipelinePtr& pipeline : pipelines)
		{
			if (pipeline)
			{
				SAFE_UNINIT(pipeline);
			}
		}
		pipelines.clear();
	}

	m_MaterialPipelineCreated = false;

	return true;
}

bool KMaterialSubMesh::CreateMaterialPipeline()
{
	if (m_pSubMesh && m_pMaterial)
	{
		const KVertexData* vertexData = m_pSubMesh->m_pVertexData;

		IKShaderPtr vsShader = m_pMaterial->GetVSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		IKShaderPtr fsShader = m_pMaterial->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture, false);
		IKShaderPtr msShader = m_pMaterial->GetMSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		IKShaderPtr mfsShader = m_pMaterial->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture, true);

		if (vsShader->GetResourceState() != RS_DEVICE_LOADED || fsShader->GetResourceState() != RS_DEVICE_LOADED)
		{
			return false;
		}

		if (msShader && msShader->GetResourceState() != RS_DEVICE_LOADED)
		{
			return false;
		}

		if (mfsShader && mfsShader->GetResourceState() != RS_DEVICE_LOADED)
		{
			return false;
		}

		MaterialBlendMode blendMode = m_pMaterial->GetBlendMode();

		for (PipelineStage stage : {PIPELINE_STAGE_OPAQUE, PIPELINE_STAGE_OPAQUE_INSTANCE, PIPELINE_STAGE_TRANSPRANT})
		{
			FramePipelineList& pipelineList = m_Pipelines[stage];
			for (size_t frameIdx = 0; frameIdx < pipelineList.size(); ++frameIdx)
			{
				SAFE_UNINIT(pipelineList[frameIdx]);
			}
			pipelineList.clear();
		}

		enum
		{
			DEFAULT_IDX,
			MESH_IDX,
			INSTANCE_IDX,
			IDX_COUNT
		};

		// 构造所需要的Pipeline
		FramePipelineList* pipelineLists[IDX_COUNT] = { nullptr };
		
		switch (blendMode)
		{
			case OPAQUE:
				pipelineLists[DEFAULT_IDX] = &m_Pipelines[PIPELINE_STAGE_OPAQUE];
				pipelineLists[MESH_IDX] = msShader ? &m_Pipelines[PIPELINE_STAGE_OPAQUE_MESH] : nullptr;
				pipelineLists[INSTANCE_IDX] = &m_Pipelines[PIPELINE_STAGE_OPAQUE_INSTANCE];
				break;
			case TRANSRPANT:
				pipelineLists[DEFAULT_IDX] = &m_Pipelines[PIPELINE_STAGE_TRANSPRANT];
				pipelineLists[MESH_IDX] = nullptr;
				pipelineLists[INSTANCE_IDX] = nullptr;
				break;
			default:
				break;
		}

		const KShaderInformation& vsInfo = vsShader->GetInformation();
		const KShaderInformation& fsInfo = fsShader->GetInformation();

		for (size_t i = 0; i < IDX_COUNT; ++i)
		{
			if (pipelineLists[i])
			{
				FramePipelineList& pipelineList = *pipelineLists[i];

				for (size_t frameIdx = 0; frameIdx < pipelineList.size(); ++frameIdx)
				{
					SAFE_UNINIT(pipelineList[frameIdx]);
				}
				pipelineList.clear();

				pipelineList.resize(m_FrameInFlight);
				for (size_t frameIdx = 0; frameIdx < m_FrameInFlight; ++frameIdx)
				{
					if (i == DEFAULT_IDX)
					{
						pipelineList[frameIdx] = m_pMaterial->CreatePipeline(frameIdx, vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture);
						ASSERT_RESULT(pipelineList[frameIdx]);
					}
					else if (i == MESH_IDX)
					{
						pipelineList[frameIdx] = m_pMaterial->CreateMeshPipeline(frameIdx, vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture);
						ASSERT_RESULT(pipelineList[frameIdx]);
					}
					else if (i == INSTANCE_IDX)
					{
						pipelineList[frameIdx] = m_pMaterial->CreateInstancePipeline(frameIdx, vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture);
						ASSERT_RESULT(pipelineList[frameIdx]);
					}
				}

				for (const KShaderInformation& info : { vsInfo, fsInfo })
				{
					auto BINDING_TO_SLOT = [](uint16_t bindingIndex)->uint8_t
					{
						uint8_t slot = 0;
						if (bindingIndex == SHADER_BINDING_DIFFUSE)
						{
							slot = MTS_DIFFUSE;
						}
						else if (bindingIndex == SHADER_BINDING_SPECULAR)
						{
							slot = MTS_SPECULAR;
						}
						else if (bindingIndex == SHADER_BINDING_NORMAL)
						{
							slot = MTS_NORMAL;
						}
						return slot;
					};

					for (const KShaderInformation::Texture& shaderTexture : info.textures)
					{
						if (shaderTexture.bindingIndex >= SHADER_BINDING_MATERIAL_BEGIN && shaderTexture.bindingIndex <= SHADER_BINDING_MATERIAL_END)
						{
							IKTexturePtr texture = nullptr;
							IKSamplerPtr sampler = nullptr;

							const KMaterialTextureBinding& textureBinding = m_pSubMesh->m_Texture;
							texture = textureBinding.GetTexture(BINDING_TO_SLOT(shaderTexture.bindingIndex));
							sampler = textureBinding.GetSampler(BINDING_TO_SLOT(shaderTexture.bindingIndex));

							if (texture && sampler)
							{
								for (size_t frameIdx = 0; frameIdx < m_FrameInFlight; ++frameIdx)
								{
									pipelineList[frameIdx]->SetSampler(shaderTexture.bindingIndex, texture, sampler);
								}
							}
						}
					}
				}

				for (size_t frameIdx = 0; frameIdx < m_FrameInFlight; ++frameIdx)
				{
					ASSERT_RESULT(pipelineList[frameIdx]->Init());
				}
			}
		}
		return true;
	}
	return false;
}

// 安卓上还是有用的
#ifndef __ANDROID__
#	define PRE_Z_DISABLE
#endif

bool KMaterialSubMesh::CreateFixedPipeline(PipelineStage stage, size_t frameIndex, IKPipelinePtr& pipeline)
{
	const KVertexData* vertexData = m_pSubMesh->m_pVertexData;
	ASSERT_RESULT(vertexData);

	if (stage == PIPELINE_STAGE_PRE_Z)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
		pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());

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
	else if (stage == PIPELINE_STAGE_SHADOW_GEN)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());

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
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		if (stage == PIPELINE_STAGE_CASCADED_SHADOW_GEN)
		{
			pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());
			pipeline->SetShader(ST_VERTEX, m_CascadedShadowVSShader);
		}
		else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_GEN_INSTANCE)
		{
			std::vector<VertexFormat> instanceFormats = vertexData->vertexFormats;
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
		pipeline->SetShader(ST_FRAGMENT, m_ShadowFSShader);

		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CASCADED_SHADOW);
		pipeline->SetConstantBuffer(CBT_CASCADED_SHADOW, ST_VERTEX, shadowBuffer);

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if (stage == PIPELINE_STAGE_GBUFFER || stage == PIPELINE_STAGE_GBUFFER_INSTANCE)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		if (stage == PIPELINE_STAGE_GBUFFER)
		{
			pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());
			pipeline->SetShader(ST_VERTEX, m_GBufferVSShader);
		}
		else if (stage == PIPELINE_STAGE_GBUFFER_INSTANCE)
		{
			std::vector<VertexFormat> instanceFormats = vertexData->vertexFormats;
			instanceFormats.push_back(VF_INSTANCE);
			pipeline->SetVertexBinding(instanceFormats.data(), instanceFormats.size());
			pipeline->SetShader(ST_VERTEX, m_GBufferVSInstanceShader);
		}

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetShader(ST_FRAGMENT, m_GBufferFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if (stage == PIPELINE_STAGE_DEBUG_LINE)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());

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
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());

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

bool KMaterialSubMesh::GetRenderCommand(PipelineStage stage, size_t frameIndex, KRenderCommand& command)
{
	if (stage >= PIPELINE_STAGE_COUNT)
	{
		assert(false && "pipeline stage count out of bound");
		return false;
	}
	if (frameIndex >= m_FrameInFlight)
	{
		assert(false && "frame index out of bound");
		return false;
	}
#ifdef PRE_Z_DISABLE
	if (stage == PIPELINE_STAGE_PRE_Z)
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
	if (pipeline)
	{
		const KVertexData* vertexData = m_pSubMesh->m_pVertexData;
		const KIndexData* indexData = &m_pSubMesh->m_IndexData;
		const KMeshData* meshData = &m_pSubMesh->m_MeshData;
		const bool& indexDraw = m_pSubMesh->m_IndexDraw;

		ASSERT_RESULT(vertexData);
		ASSERT_RESULT(!indexDraw || indexData);

		command.vertexData = vertexData;
		command.indexData = indexData;
		command.meshData = meshData;
		command.pipeline = pipeline;
		command.indexDraw = indexDraw;

		if (stage >= PIPELINE_STAGE_OPAQUE_MESH && stage <= PIPELINE_STAGE_OPAQUE_MESH)
		{
			command.meshShaderDraw = true;
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool KMaterialSubMesh::Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&)> func)
{
	if (m_pMaterial && !m_MaterialPipelineCreated)
	{
		const KVertexData* vertexData = m_pSubMesh->m_pVertexData;

		// 促发一下加载
		if (!m_MateriaShaderTriggerLoaded)
		{
			m_pMaterial->GetVSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
			m_pMaterial->GetVSInstanceShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
			m_pMaterial->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture, false);
			if (m_pMaterial->HasMSShader())
			{
				m_pMaterial->GetMSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
				m_pMaterial->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture, true);
			}
			m_MateriaShaderTriggerLoaded = true;
		}

		if (m_pMaterial->IsShaderLoaded(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), &m_pSubMesh->m_Texture))
		{
			if (!m_MaterialPipelineCreated)
			{
				if (CreateMaterialPipeline())
				{
					m_MaterialPipelineCreated = true;
				}
			}
		}
	}

	if (m_pMaterial && !m_MaterialPipelineCreated)
	{
		return false;
	}

	KRenderCommand command;
	if (GetRenderCommand(stage, frameIndex, command))
	{
		func(std::move(command));
		return true;
	}
	return false;
}