#include "KMaterialSubMesh.h"
#include "KSubMesh.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKSampler.h"

#include "Internal/KRenderGlobal.h"

KMaterialSubMesh::KMaterialSubMesh()
{
}

KMaterialSubMesh::~KMaterialSubMesh()
{
}

bool KMaterialSubMesh::CreateFixedPipeline()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/shadow.vert", m_ShadowVSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/shadow.frag", m_ShadowFSShader, false));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/cascadedshadow_static.vert", m_CascadedShadowStaticVSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/cascadedshadow_static_instance.vert", m_CascadedShadowStaticVSInstanceShader, false));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/cascadedshadow_dynamic.vert", m_CascadedShadowDynamicVSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/cascadedshadow_dynamic_instance.vert", m_CascadedShadowDynamicVSInstanceShader, false));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/svo/voxelzation/voxelzation.vert", m_VoxelVSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/svo/voxelzation/voxelzation.geom", m_VoxelGSShader, false));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/svo/voxelzation/voxelzation.frag", m_VoxelFSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/svo/voxelzation/voxelzation_sparse.frag", m_VoxelSparseFSShader, false));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/clipmap/voxelzation/voxelzation_clipmap.vert", m_VoxelClipmapVSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/clipmap/voxelzation/voxelzation_clipmap.geom", m_VoxelClipmapGSShader, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/clipmap/voxelzation/voxelzation_clipmap.frag", m_VoxelClipmapFSShader, false));

	for (PipelineStage stage :
	{
		PIPELINE_STAGE_SHADOW_GEN,
		PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN,
		PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN,
		PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN_INSTANCE,
		PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN_INSTANCE,
	})
	{
		IKPipelinePtr& pipeline = m_Pipelines[stage];
		assert(!pipeline);
		CreateFixedPipeline(stage, pipeline);
	}

	return true;
}

bool KMaterialSubMesh::Init(KSubMeshPtr subMesh, KMaterialRef material)
{
	UnInit();
	m_SubMesh = subMesh;
	m_Material = material;
	ASSERT_RESULT(CreateFixedPipeline());
	ASSERT_RESULT(CreateMaterialPipeline());
	ASSERT_RESULT(CreateGBufferPipeline());
	ASSERT_RESULT(CreateVoxelPipeline());
	return true;
}

bool KMaterialSubMesh::InitDebug(KSubMeshPtr subMesh, DebugPrimitive primtive)
{
	m_SubMesh = subMesh;

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

	IKPipelinePtr& pipeline = m_Pipelines[debugStage];
	assert(pipeline == nullptr);
	CreateFixedPipeline(debugStage, pipeline);

	return true;
}

bool KMaterialSubMesh::UnInit()
{
	m_SubMesh = nullptr;
	m_Material.Release();

	m_DebugVSShader.Release();
	m_DebugFSShader.Release();

	m_ShadowVSShader.Release();
	m_ShadowFSShader.Release();

	m_CascadedShadowStaticVSShader.Release();
	m_CascadedShadowStaticVSShader.Release();

	m_CascadedShadowDynamicVSShader.Release();
	m_CascadedShadowDynamicVSInstanceShader.Release();

	for (size_t i = 0; i < PIPELINE_STAGE_COUNT; ++i)
	{
		SAFE_UNINIT(m_Pipelines[i]);
	}

	return true;
}

bool KMaterialSubMesh::CreateMaterialPipeline()
{
	if (m_SubMesh && m_Material)
	{
		const KVertexData* vertexData = m_SubMesh->m_pVertexData;

		IKShaderPtr vsShader = m_Material->GetVSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		IKShaderPtr fsShader = m_Material->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), false);
		IKShaderPtr msShader = m_Material->GetMSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		IKShaderPtr mfsShader = m_Material->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), true);

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

		MaterialShadingMode shaingMode = m_Material->GetShadingMode();

		for (PipelineStage stage : {PIPELINE_STAGE_OPAQUE, PIPELINE_STAGE_OPAQUE_INSTANCE, PIPELINE_STAGE_TRANSPRANT})
		{
			SAFE_UNINIT(m_Pipelines[stage]);
		}

		enum
		{
			DEFAULT_IDX,
			MESH_IDX,
			INSTANCE_IDX,
			IDX_COUNT
		};

		// 构造所需要的Pipeline
		IKPipelinePtr* pipelines[IDX_COUNT] = { nullptr };
		
		switch (shaingMode)
		{
			case MSM_OPAQUE:
				pipelines[DEFAULT_IDX] = &m_Pipelines[PIPELINE_STAGE_OPAQUE];
				pipelines[MESH_IDX] = msShader ? &m_Pipelines[PIPELINE_STAGE_OPAQUE_MESH] : nullptr;
				pipelines[INSTANCE_IDX] = &m_Pipelines[PIPELINE_STAGE_OPAQUE_INSTANCE];
				break;
			case MSM_TRANSRPANT:
				pipelines[DEFAULT_IDX] = &m_Pipelines[PIPELINE_STAGE_TRANSPRANT];
				pipelines[MESH_IDX] = nullptr;
				pipelines[INSTANCE_IDX] = nullptr;
				break;
			default:
				break;
		}

		const KShaderInformation& vsInfo = vsShader->GetInformation();
		const KShaderInformation& fsInfo = fsShader->GetInformation();

		for (size_t i = 0; i < IDX_COUNT; ++i)
		{
			if (pipelines[i])
			{
				IKPipelinePtr& pipeline = *pipelines[i];

				if (i == DEFAULT_IDX)
				{
					pipeline = m_Material->CreatePipeline(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
				}
				else if (i == MESH_IDX)
				{
					pipeline = m_Material->CreateMeshPipeline(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
				}
				else if (i == INSTANCE_IDX)
				{
					pipeline = m_Material->CreateInstancePipeline(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
				}

				ASSERT_RESULT(pipeline->Init());
			}
		}
		return true;
	}
	return false;
}

bool KMaterialSubMesh::SetupMaterialGeneratedCode(std::string& code)
{
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);
	IKSourceFilePtr materialSourceFile = GetSourceFile();
	materialSourceFile->SetIOHooker(IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(system)));
	if (materialSourceFile->Open("material/diffuse.glsl"))
	{
		code = materialSourceFile->GetFinalSource();
		return true;
	}
	else
	{
		code.clear();
		return false;
	}
}

bool KMaterialSubMesh::CreateGBufferPipeline()
{
	SAFE_UNINIT(m_Pipelines[PIPELINE_STAGE_BASEPASS]);
	SAFE_UNINIT(m_Pipelines[PIPELINE_STAGE_BASEPASS_INSTANCE]);

	if (m_SubMesh && m_Material->GetShadingMode() == MSM_OPAQUE)
	{
		const KVertexData* vertexData = m_SubMesh->m_pVertexData;

		IKShaderPtr vsShader = m_Material->GetVSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		IKShaderPtr vsInstanceShader = m_Material->GetVSInstanceShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size());
		IKShaderPtr fsShader = m_Material->GetFSShader(vertexData->vertexFormats.data(), vertexData->vertexFormats.size(), false);

		for (PipelineStage stage : {PIPELINE_STAGE_BASEPASS, PIPELINE_STAGE_BASEPASS_INSTANCE})
		{
			IKPipelinePtr pipeline = nullptr;
			KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

			if (stage == PIPELINE_STAGE_BASEPASS)
			{
				pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());
				pipeline->SetShader(ST_VERTEX, vsShader);
			}
			else if (stage == PIPELINE_STAGE_BASEPASS_INSTANCE)
			{
				std::vector<VertexFormat> instanceFormats = vertexData->vertexFormats;
				instanceFormats.push_back(VF_INSTANCE);
				pipeline->SetVertexBinding(instanceFormats.data(), instanceFormats.size());
				pipeline->SetShader(ST_VERTEX, vsInstanceShader);
			}

			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_FRAGMENT, fsShader);

			IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
			pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_VERTEX | ST_FRAGMENT, cameraBuffer);

			ASSERT_RESULT(pipeline->Init());

			m_Pipelines[stage] = pipeline;
		}
	}

	return true;
}

bool KMaterialSubMesh::CreateVoxelPipeline()
{
	for (PipelineStage stage : {PIPELINE_STAGE_VOXEL, PIPELINE_STAGE_SPARSE_VOXEL, PIPELINE_STAGE_CLIPMAP_VOXEL})
	{
		SAFE_UNINIT(m_Pipelines[stage]);

		IKPipelinePtr& pipeline = m_Pipelines[stage];
		const KVertexData* vertexData = m_SubMesh->m_pVertexData;

		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_NEVER, false, false);

		if (stage == PIPELINE_STAGE_VOXEL)
		{
			pipeline->SetShader(ST_VERTEX, *m_VoxelVSShader);
			pipeline->SetShader(ST_GEOMETRY, *m_VoxelGSShader);
			pipeline->SetShader(ST_FRAGMENT, *m_VoxelFSShader);

			IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
			pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

			pipeline->SetStorageImage(VOXEL_BINDING_ALBEDO, KRenderGlobal::Voxilzer.GetVoxelAlbedo(), EF_R32_UINT);
			pipeline->SetStorageImage(VOXEL_BINDING_NORMAL, KRenderGlobal::Voxilzer.GetVoxelNormal(), EF_R32_UINT);
			pipeline->SetStorageImage(VOXEL_BINDING_EMISSION, KRenderGlobal::Voxilzer.GetVoxelEmissive(), EF_R32_UINT);
			pipeline->SetStorageImage(VOXEL_BINDING_STATIC_FLAG, KRenderGlobal::Voxilzer.GetStaticFlag(), EF_UNKNOWN);
		}
		else if(stage == PIPELINE_STAGE_SPARSE_VOXEL)
		{
			pipeline->SetShader(ST_VERTEX, *m_VoxelVSShader);
			pipeline->SetShader(ST_GEOMETRY, *m_VoxelGSShader);
			pipeline->SetShader(ST_FRAGMENT, *m_VoxelSparseFSShader);

			IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL);
			pipeline->SetConstantBuffer(CBT_VOXEL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

			pipeline->SetStorageBuffer(VOXEL_BINDING_COUNTER, ST_FRAGMENT, KRenderGlobal::Voxilzer.GetCounterBuffer());
			pipeline->SetStorageBuffer(VOXEL_BINDING_FRAGMENTLIST, ST_FRAGMENT, KRenderGlobal::Voxilzer.GetFragmentlistBuffer());
			pipeline->SetStorageBuffer(VOXEL_BINDING_COUNTONLY, ST_FRAGMENT, KRenderGlobal::Voxilzer.GetCountOnlyBuffer());
			pipeline->SetStorageImage(VOXEL_BINDING_STATIC_FLAG, KRenderGlobal::Voxilzer.GetStaticFlag(), EF_UNKNOWN);
		}
		else
		{
			pipeline->SetShader(ST_VERTEX, *m_VoxelClipmapVSShader);
			pipeline->SetShader(ST_GEOMETRY, *m_VoxelClipmapGSShader);
			pipeline->SetShader(ST_FRAGMENT, *m_VoxelClipmapFSShader);

			IKUniformBufferPtr voxelClipmapBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
			pipeline->SetConstantBuffer(CBT_VOXEL_CLIPMAP, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelClipmapBuffer);

			pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_ALBEDO, KRenderGlobal::ClipmapVoxilzer.GetVoxelAlbedo(), EF_R32_UINT);
			pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_NORMAL, KRenderGlobal::ClipmapVoxilzer.GetVoxelNormal(), EF_R32_UINT);
			pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_EMISSION, KRenderGlobal::ClipmapVoxilzer.GetVoxelEmissive(), EF_R32_UINT);
			pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_STATIC_FLAG, KRenderGlobal::ClipmapVoxilzer.GetStaticFlag(), EF_UNKNOWN);
			pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, KRenderGlobal::ClipmapVoxilzer.GetVoxelVisibility(), EF_UNKNOWN);
		}

		IKMaterialTextureBindingPtr textureBinding = m_Material->GetTextureBinding();
		for (uint8_t i = 0; i < textureBinding->GetNumSlot(); ++i)
		{
			IKTexturePtr texture = textureBinding->GetTexture(i);
			IKSamplerPtr sampler = textureBinding->GetSampler(i);

			if (!texture || !sampler)
			{
				KTextureRef errorTexture;
				KRenderGlobal::TextureManager.GetErrorTexture(errorTexture);
				texture = *errorTexture;

				KSamplerRef errorSampler;
				KSamplerDescription desc;
				desc.minFilter = desc.magFilter = FM_LINEAR;
				desc.minMipmap = 0;
				desc.maxMipmap = texture->GetMipmaps() - 1;

				KRenderGlobal::SamplerManager.Acquire(desc, errorSampler);
				sampler = *errorSampler;
			}

			if (i == MTS_DIFFUSE)
			{
				if (stage == PIPELINE_STAGE_CLIPMAP_VOXEL)
				{
					pipeline->SetSampler(VOXEL_CLIPMAP_BINDING_DIFFUSE_MAP, texture->GetFrameBuffer(), sampler);
				}
				else
				{
					pipeline->SetSampler(VOXEL_BINDING_DIFFUSE_MAP, texture->GetFrameBuffer(), sampler);
				}	
			}
		}

		ASSERT_RESULT(pipeline->Init());
	}

	return true;
}

bool KMaterialSubMesh::CreateFixedPipeline(PipelineStage stage, IKPipelinePtr& pipeline)
{
	const KVertexData* vertexData = m_SubMesh->m_pVertexData;
	ASSERT_RESULT(vertexData);

	if (stage == PIPELINE_STAGE_SHADOW_GEN)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(false, false, false, false);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetDepthBiasEnable(true);

		pipeline->SetShader(ST_VERTEX, *m_ShadowVSShader);
		pipeline->SetShader(ST_FRAGMENT, *m_ShadowFSShader);

		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_SHADOW);
		pipeline->SetConstantBuffer(CBT_SHADOW, ST_VERTEX, shadowBuffer);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(KConstantDefinition::OBJECT));

		ASSERT_RESULT(pipeline->Init());
		return true;
	}
	else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN || stage == PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN_INSTANCE
		|| stage == PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN || stage == PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN_INSTANCE)
	{
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

		if (stage == PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN)
		{
			pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());
			pipeline->SetShader(ST_VERTEX, *m_CascadedShadowStaticVSShader);
			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_STATIC_CASCADED_SHADOW);
			pipeline->SetConstantBuffer(CBT_STATIC_CASCADED_SHADOW, ST_VERTEX, shadowBuffer);
		}
		else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN)
		{
			pipeline->SetVertexBinding((vertexData->vertexFormats).data(), vertexData->vertexFormats.size());
			pipeline->SetShader(ST_VERTEX, *m_CascadedShadowDynamicVSShader);
			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_DYNAMIC_CASCADED_SHADOW);
			pipeline->SetConstantBuffer(CBT_DYNAMIC_CASCADED_SHADOW, ST_VERTEX, shadowBuffer);
		}
		else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN_INSTANCE)
		{
			std::vector<VertexFormat> instanceFormats = vertexData->vertexFormats;
			instanceFormats.push_back(VF_INSTANCE);
			pipeline->SetVertexBinding(instanceFormats.data(), instanceFormats.size());
			pipeline->SetShader(ST_VERTEX, *m_CascadedShadowStaticVSInstanceShader);
			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_STATIC_CASCADED_SHADOW);
			pipeline->SetConstantBuffer(CBT_STATIC_CASCADED_SHADOW, ST_VERTEX, shadowBuffer);
		}
		else if (stage == PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN_INSTANCE)
		{
			std::vector<VertexFormat> instanceFormats = vertexData->vertexFormats;
			instanceFormats.push_back(VF_INSTANCE);
			pipeline->SetVertexBinding(instanceFormats.data(), instanceFormats.size());
			pipeline->SetShader(ST_VERTEX, *m_CascadedShadowDynamicVSInstanceShader);
			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_DYNAMIC_CASCADED_SHADOW);
			pipeline->SetConstantBuffer(CBT_DYNAMIC_CASCADED_SHADOW, ST_VERTEX, shadowBuffer);
		}

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(false, false, false, false);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetDepthBiasEnable(true);
		pipeline->SetShader(ST_FRAGMENT, *m_ShadowFSShader);

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

		pipeline->SetShader(ST_VERTEX, *m_DebugVSShader);
		pipeline->SetShader(ST_FRAGMENT, *m_DebugFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
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

		pipeline->SetShader(ST_VERTEX, *m_DebugVSShader);
		pipeline->SetShader(ST_FRAGMENT, *m_DebugFSShader);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
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

bool KMaterialSubMesh::GetRenderCommand(PipelineStage stage, KRenderCommand& command)
{
	if (stage >= PIPELINE_STAGE_COUNT)
	{
		assert(false && "pipeline stage count out of bound");
		return false;
	}

	IKPipelinePtr pipeline = m_Pipelines[stage];
	if (pipeline)
	{
		const KVertexData* vertexData = m_SubMesh->m_pVertexData;
		const KIndexData* indexData = &m_SubMesh->m_IndexData;
		const KMeshData* meshData = &m_SubMesh->m_MeshData;
		const IKMaterialTextureBinding* textureBinding = m_Material->GetTextureBinding().get();
		const bool& indexDraw = m_SubMesh->m_IndexDraw;

		ASSERT_RESULT(vertexData);
		ASSERT_RESULT(!indexDraw || indexData);

		command.vertexData = vertexData;
		command.indexData = indexData;
		command.meshData = meshData;
		command.textureBinding = textureBinding;
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