#include "KVoxilzer.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

KVoxilzer::KVoxilzer()
	: m_Scene(nullptr)
	, m_StaticFlag(nullptr)
	, m_VoxelAlbedo(nullptr)
	, m_VoxelNormal(nullptr)
	, m_VoxelEmissive(nullptr)
	, m_VoxelRadiance(nullptr)
	, m_VolumeDimension(256)
	, m_VoxelCount(0)
	, m_VolumeGridSize(0)
	, m_VoxelSize(0)
{
	m_OnSceneChangedFunc = std::bind(&KVoxilzer::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KVoxilzer::~KVoxilzer()
{
}

void KVoxilzer::OnSceneChanged(EntitySceneOp op, IKEntityPtr entity)
{
	UpdateProjectionMatrices();
	VoxelizeStaticScene();
}

void KVoxilzer::UpdateProjectionMatrices()
{
	KAABBBox sceneBox;
	m_Scene->GetSceneObjectBound(sceneBox);

	glm::vec3 axisSize = sceneBox.GetExtend() * 2.0f;
	const glm::vec3& center = sceneBox.GetCenter();

	m_VolumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
	m_VoxelSize = m_VolumeGridSize / m_VolumeDimension;
	auto halfSize = m_VolumeGridSize / 2.0f;

	// projection matrices
	auto projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, m_VolumeGridSize);

	// view matrices
	m_ViewProjectionMatrix[0] = lookAt(center + glm::vec3(halfSize, 0.0f, 0.0f),
		center, glm::vec3(0.0f, 1.0f, 0.0f));
	m_ViewProjectionMatrix[1] = lookAt(center + glm::vec3(0.0f, halfSize, 0.0f),
		center, glm::vec3(0.0f, 0.0f, -1.0f));
	m_ViewProjectionMatrix[2] = lookAt(center + glm::vec3(0.0f, 0.0f, halfSize),
		center, glm::vec3(0.0f, 1.0f, 0.0f));

	for (uint32_t i = 0; i < 3; ++i)
	{
		m_ViewProjectionMatrix[i] = projection * m_ViewProjectionMatrix[i];
		m_ViewProjectionMatrixI[i] = glm::inverse(m_ViewProjectionMatrix[i]);
	}

	for (uint32_t frameIndex = 0; frameIndex < KRenderGlobal::RenderDevice->GetNumFramesInFlight(); ++frameIndex)
	{
		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_VOXEL);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_VOXEL);
		const KConstantDefinition::ConstantBufferDetail& details = KConstantDefinition::GetConstantBufferDetail(CBT_VOXEL);

		for (const KConstantDefinition::ConstantSemanticDetail& detail : details.semanticDetails)
		{
			void* pWritePos = nullptr;
			pWritePos = POINTER_OFFSET(pData, detail.offset);
			if (detail.semantic == CS_VOXEL_VIEW_PROJ)
			{
				assert(sizeof(glm::mat4) * 3 == detail.size);
				for (size_t i = 0; i < 3; i++)
				{
					memcpy(pWritePos, &m_ViewProjectionMatrix[i], sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_VOXEL_VIEW_PROJ_INV)
			{
				assert(sizeof(glm::mat4) * 3 == detail.size);
				for (size_t i = 0; i < 3; i++)
				{
					memcpy(pWritePos, &m_ViewProjectionMatrixI[i], sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_VOXEL_MIDPOINT_SCALE)
			{
				assert(sizeof(glm::vec4) == detail.size);
				glm::vec4 midpointScale(center, 1.0f / m_VolumeGridSize);
				memcpy(pWritePos, &midpointScale, sizeof(glm::vec4));
			}
			if(detail.semantic == CS_VOXEL_DIMENSION)
			{
				assert(sizeof(uint32_t) == detail.size);
				memcpy(pWritePos, &m_VolumeDimension, sizeof(uint32_t));
			}
		}
	}
}

void KVoxilzer::SetupVoxelVolumes(uint32_t dimension)
{
	m_VolumeDimension = dimension;
	m_VoxelCount = m_VolumeDimension * m_VolumeDimension * m_VolumeDimension;
	m_VoxelSize = m_VolumeGridSize / m_VolumeDimension;

	UpdateProjectionMatrices();

	m_VoxelAlbedo->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelAlbedo->InitDevice(false);

	m_VoxelNormal->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelNormal->InitDevice(false);

	m_VoxelRadiance->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelRadiance->InitDevice(false);

	for (uint32_t i = 0; i < 6; ++i)
	{
		m_VoxelTexMipmap[i]->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, true, false);
		m_VoxelTexMipmap[i]->InitDevice(false);
	}

	m_VoxelEmissive->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelEmissive->InitDevice(false);

	m_StaticFlag->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8, false, false);
	m_StaticFlag->InitDevice(false);

	m_CloestSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_CloestSampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	m_CloestSampler->Init(0, 0);

	m_LinearSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_LinearSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_LinearSampler->Init(0, 0);

	m_MipmapSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_MipmapSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_MipmapSampler->Init(0, m_VoxelTexMipmap[0]->GetMipmaps());

	m_RenderPassTarget->InitFromColor(dimension, dimension, 1, EF_R8_UNORM);
	m_RenderPass->SetColorAttachment(0, m_RenderPassTarget->GetFrameBuffer());
	m_RenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_RenderPass->Init();
}

void KVoxilzer::VoxelizeStaticScene()
{
}

bool KVoxilzer::Init(IKRenderScene* scene, uint32_t dimension)
{
	UnInit();

	m_Scene = scene;

	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateTexture(m_StaticFlag);
	renderDevice->CreateTexture(m_VoxelAlbedo);
	renderDevice->CreateTexture(m_VoxelNormal);
	renderDevice->CreateTexture(m_VoxelEmissive);
	renderDevice->CreateTexture(m_VoxelRadiance);

	for (uint32_t i = 0; i < 6; ++i)
	{
		renderDevice->CreateTexture(m_VoxelTexMipmap[i]);
	}

	renderDevice->CreateSampler(m_CloestSampler);
	renderDevice->CreateSampler(m_LinearSampler);
	renderDevice->CreateSampler(m_MipmapSampler);

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	renderDevice->CreateCommandBuffer(m_CommandBuffer);
	m_CommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	renderDevice->CreateRenderTarget(m_RenderPassTarget);
	renderDevice->CreateRenderPass(m_RenderPass);

	SetupVoxelVolumes(dimension);

	m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

	return true;
}

bool KVoxilzer::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
	}
	m_Scene = nullptr;

	SAFE_UNINIT(m_RenderPass);
	SAFE_UNINIT(m_RenderPassTarget);
	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	SAFE_UNINIT(m_StaticFlag);
	SAFE_UNINIT(m_VoxelAlbedo);
	SAFE_UNINIT(m_VoxelNormal);
	SAFE_UNINIT(m_VoxelEmissive);
	SAFE_UNINIT(m_VoxelRadiance);

	for (uint32_t i = 0; i < 6; ++i)
	{
		SAFE_UNINIT(m_VoxelTexMipmap[i]);
	}

	SAFE_UNINIT(m_CloestSampler);
	SAFE_UNINIT(m_LinearSampler);
	SAFE_UNINIT(m_MipmapSampler);

	return true;
}