#include "KClipmapVoxilzer.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "KBase/Interface/IKLog.h"

KClipmapVoxilzerLevel::KClipmapVoxilzerLevel(KClipmapVoxilzer* parent, uint32_t level)
	: m_Parent(parent)
	, m_VoxelSize(0)
	, m_LevelIdx(level)
	, m_MinUpdateChange(4)
	, m_Extent(0)
{
	m_VoxelSize = m_Parent->GetBaseVoxelSize() * (1 << m_LevelIdx);
	m_Extent = m_Parent->GetVoxelDimension();
	m_Region.min = glm::ivec3(-m_Extent / 2);
	m_Region.max = glm::ivec3(m_Extent / 2);
}

KClipmapVoxilzerLevel::~KClipmapVoxilzerLevel()
{
}

void KClipmapVoxilzerLevel::SetUpdateMovement(const glm::ivec3& movement)
{
	m_Movement = movement;
}

void KClipmapVoxilzerLevel::ApplyUpdateMovement()
{
	m_Region.min += m_Movement;
	m_Region.max += m_Movement;
	m_Movement = glm::ivec3(0);
}

void KClipmapVoxilzerLevel::SetMinPosition(const glm::ivec3& min)
{
	m_Region.min = min;
	m_Region.max = m_Region.min + glm::ivec3(m_Extent);
}

const glm::vec3 KClipmapVoxilzerLevel::WorldPositionToClipUVW(const glm::vec3& worldPos) const
{
	glm::vec3 extent = glm::vec3((float)m_Extent);
	glm::vec3 uvw = worldPos / (extent * m_VoxelSize);
	uvw = uvw - glm::floor(uvw);
	return uvw;
}

const glm::vec3 KClipmapVoxilzerLevel::ClipUVWToWorldPosition(const glm::vec3& uvw) const
{
	glm::vec3 extent = glm::vec3((float)m_Extent);
	glm::vec3 worldPos = (glm::floor(glm::vec3(m_Region.min) / extent) + uvw)
		* extent * m_VoxelSize;
	return worldPos;
}

const glm::ivec3 KClipmapVoxilzerLevel::WorldPositionToClipCoord(const glm::vec3& worldPos) const
{
	glm::ivec3 clipCoord = glm::ivec3(glm::floor(worldPos / m_VoxelSize));
	return clipCoord;
}

const glm::vec3 KClipmapVoxilzerLevel::ClipCoordToWorldPosition(const glm::vec3& clipCoord) const
{
	return clipCoord * m_VoxelSize;
}

const glm::ivec3 KClipmapVoxilzerLevel::ClipCoordToImagePosition(const glm::ivec3& clipCoord) const
{
	glm::ivec3 imagePos = clipCoord % glm::ivec3(m_Extent);
	imagePos += glm::ivec3(m_Extent);
	imagePos %= glm::ivec3(m_Extent);
	return imagePos;
}

const glm::ivec3 KClipmapVoxilzerLevel::ImagePositionToClipCoord(const glm::ivec3& imagePos) const
{
	glm::ivec3 minImagePos = ClipCoordToImagePosition(m_Region.min);
	glm::ivec3 maxImagePos = ClipCoordToImagePosition(m_Region.max);

	glm::ivec3 clipCoord = imagePos;

	if (clipCoord.x >= maxImagePos.x)
		clipCoord.x = m_Region.min.x + clipCoord.x - maxImagePos.x;
	else
		clipCoord.x = m_Region.max.x - maxImagePos.x + clipCoord.x;

	if (clipCoord.y >= maxImagePos.y)
		clipCoord.y = m_Region.min.y + clipCoord.y - maxImagePos.y;
	else
		clipCoord.y = m_Region.max.y - maxImagePos.y + clipCoord.y;

	if (clipCoord.z >= maxImagePos.z)
		clipCoord.z = m_Region.min.z + clipCoord.z - maxImagePos.z;
	else
		clipCoord.z = m_Region.max.z - maxImagePos.z + clipCoord.z;

	return clipCoord;
}

void KClipmapVoxilzerLevel::UpdateProjectionMatrices()
{
	glm::vec3 worldMin = GetWorldMin();
	glm::vec3 worldMax = GetWorldMax();
	glm::vec3 center = (worldMin + worldMax) * 0.5f;

	float fullSize = m_VoxelSize * m_Extent;
	float halfSize = fullSize * 0.5f;

	// projection matrices
	glm::mat4 projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, fullSize);

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
}

KClipmapVoxilzer::KClipmapVoxilzer()
	: m_Scene(nullptr)
	, m_Camera(nullptr)
	, m_Width(0)
	, m_Height(0)
	, m_VolumeDimension(128)
	, m_ClipmapVolumeDimensionX(128)
	, m_ClipmapVolumeDimensionY(128)
	, m_ClipmapVolumeDimensionZ(128)
	, m_ClipmapVolumeDimensionX6Face(128 * 6)
	, m_BorderSize(1)
	, m_ClipLevelCount(3)
	, m_BaseVoxelSize(10.0f)
	, m_VoxelDrawEnable(false)
	, m_VoxelDrawWireFrame(true)
	, m_VoxelBorderEnable(true)
	, m_VoxelDebugUpdate(true)
	, m_VoxelNeedUpdate(false)
	, m_VoxelEmpty(true)
{
	m_OnSceneChangedFunc = std::bind(&KClipmapVoxilzer::OnSceneChanged, this, std::placeholders::_1, std::placeholders::_2);
}

KClipmapVoxilzer::~KClipmapVoxilzer()
{
}

void KClipmapVoxilzer::Resize(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;

	m_LightPassTarget->UnInit();
	m_LightPassTarget->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM);

	m_LightPassRenderPass->UnInit();
	m_LightPassRenderPass->SetColorAttachment(0, m_LightPassTarget->GetFrameBuffer());
	m_LightPassRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_LightPassRenderPass->Init();
}

bool KClipmapVoxilzer::RenderVoxel(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
{
	if (!m_VoxelDrawEnable)
		return true;

	m_DrawCommandBuffer->BeginSecondary(renderPass);
	m_DrawCommandBuffer->SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &m_VoxelDrawVertexData;
	command.indexData = nullptr;
	command.pipeline = m_VoxelDrawWireFrame ? m_VoxelWireFrameDrawPipeline : m_VoxelDrawPipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = false;

	for (uint32_t i = 0; i < m_ClipLevelCount; ++i)
	{
		struct ObjectData
		{
			glm::uvec4 params;
		} objectData;
		objectData.params[0] = i;
		objectData.params[1] = m_ClipLevelCount;

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);
		m_DrawCommandBuffer->Render(command);
	}

	m_DrawCommandBuffer->End();

	buffers.push_back(m_DrawCommandBuffer);

	return true;
}

void KClipmapVoxilzer::UpdateVoxelBuffer()
{
	glm::mat4 viewProjections[3 * 6];
	glm::mat4 viewProjectionIs[3 * 6];
	glm::vec4 updateRegionMins[3 * 6];
	glm::vec4 updateRegionMaxs[3 * 6];
	glm::vec4 minAndVoxelSize[6];
	glm::vec4 maxAndExtent[6];

	ZERO_MEMORY(viewProjections);
	ZERO_MEMORY(viewProjectionIs);
	ZERO_MEMORY(updateRegionMins);
	ZERO_MEMORY(updateRegionMaxs);
	ZERO_MEMORY(minAndVoxelSize);
	ZERO_MEMORY(maxAndExtent);

	for (size_t i = 0; i < 6; ++i)
	{
		if (i < m_ClipLevels.size())
		{
			KClipmapVoxilzerLevel& level = m_ClipLevels[i];
			level.UpdateProjectionMatrices();

			memcpy(viewProjections + i * 3, level.GetViewProjectionMatrix(), sizeof(glm::mat4) * 3);
			memcpy(viewProjectionIs + i * 3, level.GetViewProjectionMatrixInv(), sizeof(glm::mat4) * 3);

			glm::vec3 worldMin = level.GetWorldMin();
			glm::vec3 worldMax = level.GetWorldMax();
			memcpy(minAndVoxelSize + i, &glm::vec4(worldMin, level.GetVoxelSize()), sizeof(glm::vec4));
			memcpy(maxAndExtent + i, &glm::vec4(worldMax, level.GetWorldExtent()), sizeof(glm::vec4));

			const std::vector<KClipmapVoxelizationRegion>& updateRegions = level.GetUpdateRegions();
			for (uint32_t j = 0; j < 3; ++j)
			{
				if (j < updateRegions.size())
				{
					updateRegionMins[i * 3 + j] = glm::vec4(glm::vec3(updateRegions[j].min) * level.GetVoxelSize(), 1.0f);
					updateRegionMaxs[i * 3 + j] = glm::vec4(glm::vec3(updateRegions[j].max) * level.GetVoxelSize(), 1.0f);
				}
				else
				{
					updateRegionMins[i * 3 + j].w = 0;
					updateRegionMaxs[i * 3 + j].w = 0;
				}
			}
		}
	}

	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);

	void* pData = KConstantGlobal::GetGlobalConstantData(CBT_VOXEL_CLIPMAP);
	const KConstantDefinition::ConstantBufferDetail& details = KConstantDefinition::GetConstantBufferDetail(CBT_VOXEL_CLIPMAP);

	for (const KConstantDefinition::ConstantSemanticDetail& detail : details.semanticDetails)
	{
		void* pWritePos = nullptr;
		pWritePos = POINTER_OFFSET(pData, detail.offset);
		if (detail.semantic == CS_VOXEL_CLIPMAP_VIEW_PROJ)
		{
			assert(sizeof(glm::mat4) * 3 * 6 == detail.size);
			memcpy(pWritePos, viewProjections, sizeof(viewProjections));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(viewProjections));
		}
		if (detail.semantic == CS_VOXEL_CLIPMAP_VIEW_PROJ_INV)
		{
			assert(sizeof(glm::mat4) * 3 * 6 == detail.size);
			memcpy(pWritePos, viewProjectionIs, sizeof(viewProjectionIs));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(viewProjectionIs));
		}
		if (detail.semantic == CS_VOXEL_CLIPMAP_UPDATE_REGION_MIN)
		{
			assert(sizeof(glm::vec4) * 3 * 6 == detail.size);
			memcpy(pWritePos, updateRegionMins, sizeof(updateRegionMins));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(updateRegionMins));
		}
		if (detail.semantic == CS_VOXEL_CLIPMAP_UPDATE_REGION_MAX)
		{
			assert(sizeof(glm::vec4) * 3 * 6 == detail.size);
			memcpy(pWritePos, updateRegionMaxs, sizeof(updateRegionMaxs));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(updateRegionMaxs));
		}
		if (detail.semantic == CS_VOXEL_CLIPMAP_REIGION_MIN_AND_VOXELSIZE)
		{
			assert(sizeof(glm::vec4) * 6 == detail.size);
			memcpy(pWritePos, minAndVoxelSize, sizeof(minAndVoxelSize));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(minAndVoxelSize));
		}
		if (detail.semantic == CS_VOXEL_CLIPMAP_REIGION_MAX_AND_EXTENT)
		{
			assert(sizeof(glm::vec4) * 6 == detail.size);
			memcpy(pWritePos, maxAndExtent, sizeof(maxAndExtent));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(maxAndExtent));
		}
		if (detail.semantic == CS_VOXEL_CLIPMAP_MISCS)
		{
			assert(sizeof(glm::uvec4) == detail.size);
			glm::uvec4 miscs;
			// volumeDimension
			miscs[0] = m_VolumeDimension;
			// borderSize
			miscs[1] = m_VoxelBorderEnable ? m_BorderSize : 0;
			// storeVisibility
			miscs[2] = true;
			// normalWeightedLambert
			miscs[3] = 1;
			memcpy(pWritePos, &miscs, sizeof(glm::uvec4));
			pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::uvec4));
		}
	}

	voxelBuffer->Write(pData);
}

void KClipmapVoxilzer::UpdateInternal()
{
	m_PrimaryCommandBuffer->BeginPrimary();

	ClearUpdateRegion(m_PrimaryCommandBuffer);
	VoxelizeStaticScene(m_PrimaryCommandBuffer);

	DownSampleVisibility(m_PrimaryCommandBuffer);
	UpdateRadiance(m_PrimaryCommandBuffer);
	DownSampleRadiance(m_PrimaryCommandBuffer);

	m_PrimaryCommandBuffer->End();
	m_PrimaryCommandBuffer->Flush();
}

glm::ivec3 KClipmapVoxilzer::ComputeMovementByCamera(uint32_t levelIdx)
{
	KClipmapVoxilzerLevel& level = m_ClipLevels[levelIdx];

	const glm::vec3 center = (level.GetWorldMin() + level.GetWorldMax()) * 0.5f;
	const glm::vec3 cameraPos = m_Camera->GetPosition() + level.GetVoxelSize() * 0.5f;

	const int32_t minUpdate = level.GetMinUpdateChange();
	const float voxelSize = level.GetVoxelSize();
	const float minUpdateVoxelSize = minUpdate * voxelSize;

	glm::ivec3 movement = glm::ivec3(glm::trunc((cameraPos - center) / minUpdateVoxelSize)) * minUpdate;
	return movement;
}

std::vector<KClipmapVoxelizationRegion> KClipmapVoxilzer::ComputeRevoxelizationRegionsByMovement(uint32_t levelIdx, const glm::ivec3& movement)
{
	KClipmapVoxilzerLevel& level = m_ClipLevels[levelIdx];

	glm::ivec3 newMin = level.GetMin() + movement;
	glm::ivec3 newMax = level.GetMax() + movement;

	level.SetUpdateMovement(movement);

	std::vector<KClipmapVoxelizationRegion> regions;

	if (abs(movement.x != 0))
	{
		if (movement.x > 0)
		{
			KClipmapVoxelizationRegion region = KClipmapVoxelizationRegion(glm::ivec3(std::max(newMax.x - movement.x, newMin.x), newMin.y, newMin.z), glm::ivec3(newMax.x, newMax.y, newMax.z));
			regions.push_back(region);
		}
		else
		{
			KClipmapVoxelizationRegion region = KClipmapVoxelizationRegion(glm::ivec3(newMin.x, newMin.y, newMin.z), glm::ivec3(std::min(newMax.x, newMin.x - movement.x), newMax.y, newMax.z));
			regions.push_back(region);
		}
	}

	if (abs(movement.y != 0))
	{
		if (movement.y > 0)
		{
			KClipmapVoxelizationRegion region = KClipmapVoxelizationRegion(glm::ivec3(newMin.x, std::max(newMax.y - movement.y, newMin.y), newMin.z), glm::ivec3(newMax.x, newMax.y, newMax.z));
			regions.push_back(region);
		}
		else
		{
			KClipmapVoxelizationRegion region = KClipmapVoxelizationRegion(glm::ivec3(newMin.x, newMin.y, newMin.z), glm::ivec3(newMax.x, std::min(newMax.y, newMin.y - movement.y), newMax.z));
			regions.push_back(region);
		}
	}

	if (abs(movement.z != 0))
	{
		if (movement.z > 0)
		{
			KClipmapVoxelizationRegion region = KClipmapVoxelizationRegion(glm::ivec3(newMin.x, newMin.y, std::max(newMax.z - movement.z, newMin.z)), glm::ivec3(newMax.x, newMax.y, newMax.z));
			regions.push_back(region);
		}
		else
		{
			KClipmapVoxelizationRegion region = KClipmapVoxelizationRegion(glm::ivec3(newMin.x, newMin.y, newMin.z), glm::ivec3(newMax.x, newMax.y, std::min(newMax.z, newMin.z - movement.z)));
			regions.push_back(region);
		}
	}

	return regions;
}

void KClipmapVoxilzer::ClearUpdateRegion(IKCommandBufferPtr commandBuffer)
{
	for (uint32_t levelIdx = 0; levelIdx < m_ClipLevelCount; ++levelIdx)
	{
		const KClipmapVoxilzerLevel& level = m_ClipLevels[levelIdx];

		for (const KClipmapVoxelizationRegion& region : level.GetUpdateRegions())
		{
			struct ObjectData
			{
				glm::ivec4 regionMin;
				glm::ivec4 regionMax;
				glm::uvec4 params;
			} objectData;

			objectData.regionMin = glm::ivec4(region.min, 0);
			objectData.regionMax = glm::ivec4(region.max, 0);
			objectData.params[0] = levelIdx;

			glm::uvec3 group = (region.max - region.min + glm::ivec3(VOXEL_CLIPMAP_GROUP_SIZE - 1)) / glm::ivec3(VOXEL_CLIPMAP_GROUP_SIZE);

			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(objectData);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, usage);

			m_ClearRegionPipeline->Execute(commandBuffer, group.x, group.y, group.z, &usage);
		}
	}
}

void KClipmapVoxilzer::ApplyUpdateMovement()
{
	for (uint32_t level = 0; level < m_ClipLevelCount; ++level)
	{
		KClipmapVoxilzerLevel& clipLevel = m_ClipLevels[level];
		clipLevel.ApplyUpdateMovement();
	}

	UpdateVoxelBuffer();
}

void KClipmapVoxilzer::VoxelizeStaticScene(IKCommandBufferPtr commandBuffer)
{
	commandBuffer->BeginDebugMarker("VoxelizeClipmapStaticScene", glm::vec4(0, 1, 1, 0));
	commandBuffer->BeginRenderPass(m_VoxelRenderPass, SUBPASS_CONTENTS_INLINE);
	commandBuffer->SetViewport(m_VoxelRenderPass->GetViewPort());

	for (uint32_t level = 0; level < m_ClipLevelCount; ++level)
	{
		const KClipmapVoxilzerLevel& clipLevel = m_ClipLevels[level];
		const glm::vec3& cameraPos = m_Camera->GetPosition();

		KAABBBox sceneBox;
		sceneBox.InitFromMinMax(clipLevel.GetWorldMin(), clipLevel.GetWorldMax());

		std::vector<KRenderComponent*> cullRes;
		((KRenderScene*)m_Scene)->GetRenderComponent(sceneBox, false, cullRes);

		if (cullRes.size() == 0)
			continue;

		std::vector<KRenderCommand> commands;
		for (KRenderComponent* render : cullRes)
		{
			render->Visit(PIPELINE_STAGE_CLIPMAP_VOXEL, [&](KRenderCommand& command)
			{
				IKEntity* entity = render->GetEntityHandle();

				KTransformComponent* transform = nullptr;
				if (entity->GetComponent(CT_TRANSFORM, &transform))
				{
					const glm::mat4& finalTran = transform->GetFinal();

					struct ObjectData
					{
						glm::mat4 model;
						uint32_t level;
					} objectData;

					objectData.model = finalTran;
					objectData.level = level;

					command.objectUsage.binding = SHADER_BINDING_OBJECT;
					command.objectUsage.range = sizeof(objectData);

					KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, command.objectUsage);

					command.pipeline->GetHandle(m_VoxelRenderPass, command.pipelineHandle);

					commands.push_back(command);
				}
			});
		}

		for (KRenderCommand& command : commands)
		{
			commandBuffer->Render(command);
		}
	}

	commandBuffer->EndRenderPass();
	commandBuffer->EndDebugMarker();
}

void KClipmapVoxilzer::UpdateRadiance(IKCommandBufferPtr commandBuffer)
{
	ClearRadiance(commandBuffer);
	InjectRadiance(commandBuffer);
}

void KClipmapVoxilzer::ClearRadiance(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension + (VOXEL_CLIPMAP_GROUP_SIZE - 1)) / VOXEL_CLIPMAP_GROUP_SIZE;

	for (uint32_t level = 0; level < m_ClipLevelCount; ++level)
	{
		KDynamicConstantBufferUsage usage;
		usage.binding = SHADER_BINDING_OBJECT;

		struct ObjectData
		{
			glm::uint32_t level;
		} objectData;

		objectData.level = level;

		usage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, usage);

		m_ClearRadiancePipeline->Execute(commandBuffer, group, group, group, &usage);
	}
}

void KClipmapVoxilzer::InjectRadiance(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension + (VOXEL_CLIPMAP_GROUP_SIZE - 1)) / VOXEL_CLIPMAP_GROUP_SIZE;

	for (uint32_t level = 0; level < m_ClipLevelCount; ++level)
	{
		KDynamicConstantBufferUsage usage;
		usage.binding = SHADER_BINDING_OBJECT;

		struct ObjectData
		{
			glm::uvec4 params;
		} objectData;

		objectData.params[0] = level;
		objectData.params[1] = m_ClipLevelCount;

		usage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, usage);

		m_InjectRadiancePipeline->Execute(commandBuffer, group, group, group, &usage);
	}
}

void KClipmapVoxilzer::UpdateVoxel()
{
	if (m_VoxelDebugUpdate || m_VoxelEmpty)
	{
		for (uint32_t levelIdx = 0; levelIdx < m_ClipLevelCount; ++levelIdx)
		{
			KClipmapVoxilzerLevel& level = m_ClipLevels[levelIdx];
			glm::vec3 cameraPos = m_Camera->GetPosition();
			cameraPos += level.GetVoxelSize() * 0.5f;
			cameraPos -= level.GetWorldExtent() * 0.5f;
			glm::ivec3 pos = level.WorldPositionToClipCoord(cameraPos);
			level.SetMinPosition(pos);
			level.SetUpdateRegions({ KClipmapVoxelizationRegion(level.GetMin(), level.GetMax()) });
		}

		m_VoxelNeedUpdate = true;
	}
	else
	{
		for (uint32_t levelIdx = 0; levelIdx < m_ClipLevelCount; ++levelIdx)
		{
			KClipmapVoxilzerLevel& level = m_ClipLevels[levelIdx];

			glm::ivec3 movement = ComputeMovementByCamera(levelIdx);
			std::vector<KClipmapVoxelizationRegion> updateRegions = ComputeRevoxelizationRegionsByMovement(levelIdx, movement);

			level.SetUpdateRegions(updateRegions);

			m_VoxelNeedUpdate |= !updateRegions.empty();
		}
	}

	ApplyUpdateMovement();
	UpdateVoxelBuffer();

	if(m_VoxelNeedUpdate)
	{
		UpdateInternal();
		m_VoxelNeedUpdate = false;
		m_VoxelEmpty = false;
	}
}

void KClipmapVoxilzer::DownSampleVisibility(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension / 2 + (VOXEL_CLIPMAP_GROUP_SIZE - 1)) / VOXEL_CLIPMAP_GROUP_SIZE;

	for (uint32_t level = 1; level < m_ClipLevelCount; ++level)
	{
		KDynamicConstantBufferUsage usage;
		usage.binding = SHADER_BINDING_OBJECT;

		struct ObjectData
		{
			glm::uint32_t level;
		} objectData;

		objectData.level = level;

		usage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, usage);

		m_DownSampleVisibilityPipeline->Execute(commandBuffer, group, group, group, &usage);
	}
}

void KClipmapVoxilzer::DownSampleRadiance(IKCommandBufferPtr commandBuffer)
{
	uint32_t group = (m_VolumeDimension / 2 + (VOXEL_CLIPMAP_GROUP_SIZE - 1)) / VOXEL_CLIPMAP_GROUP_SIZE;

	for (uint32_t level = 1; level < m_ClipLevelCount; ++level)
	{
		KDynamicConstantBufferUsage usage;
		usage.binding = SHADER_BINDING_OBJECT;

		struct ObjectData
		{
			glm::uint32_t level;
		} objectData;

		objectData.level = level;

		usage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, usage);

		m_DownSampleRadiancePipeline->Execute(commandBuffer, group, group, group, &usage);
	}
}

void KClipmapVoxilzer::ReloadShader()
{
	m_VoxelDrawVS->Reload();
	m_VoxelDrawGS->Reload();
	m_VoxelWireFrameDrawGS->Reload();
	m_VoxelDrawFS->Reload();

	m_VoxelDrawPipeline->Reload();
	m_VoxelWireFrameDrawPipeline->Reload();

	m_ClearRegionPipeline->Reload();
	m_ClearRadiancePipeline->Reload();
	m_InjectRadiancePipeline->Reload();
}

void KClipmapVoxilzer::SetupVoxelBuffer()
{
	m_VoxelAlbedo->InitFromStorage3D(m_ClipmapVolumeDimensionX, m_ClipmapVolumeDimensionY, m_ClipmapVolumeDimensionZ, 1, EF_R8GB8BA8_UNORM);
	m_VoxelNormal->InitFromStorage3D(m_ClipmapVolumeDimensionX, m_ClipmapVolumeDimensionY, m_ClipmapVolumeDimensionZ, 1, EF_R8GB8BA8_UNORM);
	m_VoxelEmissive->InitFromStorage3D(m_ClipmapVolumeDimensionX, m_ClipmapVolumeDimensionY, m_ClipmapVolumeDimensionZ, 1, EF_R8GB8BA8_UNORM);
	m_StaticFlag->InitFromStorage3D(m_ClipmapVolumeDimensionX, m_ClipmapVolumeDimensionY, m_ClipmapVolumeDimensionZ, 1, EF_R8_UNORM);
	m_VoxelRadiance->InitFromStorage3D(m_ClipmapVolumeDimensionX6Face, m_ClipmapVolumeDimensionY, m_ClipmapVolumeDimensionZ, 1, EF_R8GB8BA8_UNORM);
	m_VoxelVisibility->InitFromStorage3D(m_ClipmapVolumeDimensionX6Face, m_ClipmapVolumeDimensionY, m_ClipmapVolumeDimensionZ, 1, EF_R8GB8BA8_UNORM);
}

void KClipmapVoxilzer::SetupVoxelPipeline()
{
	IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);

	m_ClearRegionPipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_ClearRegionPipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_ClearRegionPipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_EMISSION, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_ClearRegionPipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_STATIC_FLAG, m_StaticFlag->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_ClearRegionPipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, m_VoxelVisibility->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);

	m_ClearRegionPipeline->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, voxelBuffer);
	m_ClearRegionPipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);

	m_ClearRegionPipeline->Init("voxel/clipmap/lighting/clear_region.comp");

	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);

	// Clear Radiance
	m_ClearRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, voxelBuffer);
	m_ClearRadiancePipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_ClearRadiancePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_ClearRadiancePipeline->Init("voxel/clipmap/lighting/clear_radiance.comp");

	// Inject Radiance
	m_InjectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_GLOBAL, globalBuffer);
	m_InjectRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, voxelBuffer);

	m_InjectRadiancePipeline->BindSampler(VOXEL_CLIPMAP_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), m_LinearSampler, false);
	m_InjectRadiancePipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
	m_InjectRadiancePipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_EMISSION_MAP, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
	m_InjectRadiancePipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, false);
	m_InjectRadiancePipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, m_VoxelVisibility->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
	m_InjectRadiancePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_InjectRadiancePipeline->Init("voxel/clipmap/lighting/inject_radiance.comp");

	// DownSample Visibility
	m_DownSampleVisibilityPipeline->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, voxelBuffer);
	m_DownSampleVisibilityPipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, m_VoxelVisibility->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);
	m_DownSampleVisibilityPipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_DownSampleVisibilityPipeline->Init("voxel/clipmap/lighting/downsample_visibility.comp");

	// DownSample Visibility
	m_DownSampleRadiancePipeline->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, voxelBuffer);
	m_DownSampleRadiancePipeline->BindStorageImage(VOXEL_CLIPMAP_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);
	m_DownSampleRadiancePipeline->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	m_DownSampleRadiancePipeline->Init("voxel/clipmap/lighting/downsample_radiance.comp");
}

void KClipmapVoxilzer::SetupVoxelDrawPipeline()
{
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/clipmap/draw/draw_voxels.vert", m_VoxelDrawVS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/clipmap/draw/draw_voxels.geom", m_VoxelDrawGS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_GEOMETRY, "voxel/clipmap/draw/draw_voxels_wireframe.geom", m_VoxelWireFrameDrawGS, false));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "voxel/clipmap/draw/draw_voxels.frag", m_VoxelDrawFS, false));

	enum
	{
		DEFAULT,
		WIREFRAME,
		COUNT
	};

	for (size_t idx = 0; idx < COUNT; ++idx)
	{
		IKPipelinePtr& pipeline = (idx == DEFAULT) ? m_VoxelDrawPipeline : m_VoxelWireFrameDrawPipeline;

		pipeline->SetShader(ST_VERTEX, m_VoxelDrawVS);

		pipeline->SetPrimitiveTopology(PT_POINT_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetShader(ST_GEOMETRY, idx == 0 ? m_VoxelDrawGS : m_VoxelWireFrameDrawGS);
		pipeline->SetShader(ST_FRAGMENT, m_VoxelDrawFS);

		IKUniformBufferPtr voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
		pipeline->SetConstantBuffer(CBT_VOXEL_CLIPMAP, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, voxelBuffer);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

		IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
		pipeline->SetConstantBuffer(CBT_GLOBAL, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, globalBuffer);

		pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_ALBEDO, m_VoxelAlbedo->GetFrameBuffer(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_NORMAL, m_VoxelNormal->GetFrameBuffer(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_EMISSION, m_VoxelEmissive->GetFrameBuffer(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_RADIANCE, m_VoxelRadiance->GetFrameBuffer(), EF_UNKNOWN);
		pipeline->SetStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, m_VoxelVisibility->GetFrameBuffer(), EF_UNKNOWN);

		pipeline->Init();
	}

	m_VoxelDrawVertexData.vertexCount = (uint32_t)(m_VolumeDimension * m_VolumeDimension * m_VolumeDimension);
	m_VoxelDrawVertexData.vertexStart = 0;
}

void KClipmapVoxilzer::SetupVoxelReleatedData()
{
	m_VoxelRenderPassTarget->InitFromColor(m_VolumeDimension, m_VolumeDimension, 1, EF_R8_UNORM);
	m_VoxelRenderPass->UnInit();
	m_VoxelRenderPass->SetColorAttachment(0, m_VoxelRenderPassTarget->GetFrameBuffer());
	m_VoxelRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_VoxelRenderPass->Init();

	m_LightPassTarget->InitFromColor(m_Width, m_Height, 1, EF_R8GB8BA8_UNORM);
	m_LightPassRenderPass->UnInit();
	m_LightPassRenderPass->SetColorAttachment(0, m_LightPassTarget->GetFrameBuffer());
	m_LightPassRenderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_LightPassRenderPass->Init();

	m_CloestSampler->SetAddressMode(AM_REPEAT, AM_REPEAT, AM_REPEAT);
	m_CloestSampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	m_CloestSampler->Init(0, 0);

	m_LinearSampler->SetAddressMode(AM_REPEAT, AM_REPEAT, AM_REPEAT);
	m_LinearSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_LinearSampler->Init(0, 0);

	SetupVoxelBuffer();
	SetupVoxelPipeline();
	SetupVoxelDrawPipeline();
}

void KClipmapVoxilzer::OnSceneChanged(EntitySceneOp op, IKEntityPtr entity)
{
	IKRenderComponent* render = nullptr;
	if (!entity->GetComponent(CT_RENDER, &render) || render->IsUtility())
	{
		return;
	}
	m_VoxelNeedUpdate = true;
}

bool KClipmapVoxilzer::Init(IKRenderScene* scene, const KCamera* camera, uint32_t dimension, uint32_t levelCount, uint32_t baseVoxelSize, uint32_t width, uint32_t height)
{
	UnInit();

	m_Scene = scene;
	m_Camera = camera;
	m_Width = width;
	m_Height = height;

	m_VolumeDimension = dimension;
	m_BaseVoxelSize = (float)baseVoxelSize;
	m_ClipLevelCount = levelCount;

	m_ClipmapVolumeDimensionX = m_VolumeDimension;
	m_ClipmapVolumeDimensionY = m_VolumeDimension * m_ClipLevelCount;
	m_ClipmapVolumeDimensionZ = m_VolumeDimension;

	if (m_VoxelBorderEnable)
	{
		m_ClipmapVolumeDimensionX += 2 * m_BorderSize;
		m_ClipmapVolumeDimensionY += 2 * m_ClipLevelCount * m_BorderSize;
		m_ClipmapVolumeDimensionZ += 2 * m_BorderSize;
	}

	m_ClipmapVolumeDimensionX6Face = m_ClipmapVolumeDimensionX * 6;

	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

	renderDevice->CreateRenderTarget(m_StaticFlag);
	renderDevice->CreateRenderTarget(m_VoxelAlbedo);
	renderDevice->CreateRenderTarget(m_VoxelNormal);
	renderDevice->CreateRenderTarget(m_VoxelEmissive);
	renderDevice->CreateRenderTarget(m_VoxelRadiance);
	renderDevice->CreateRenderTarget(m_VoxelVisibility);

	renderDevice->CreateComputePipeline(m_ClearRegionPipeline);

	renderDevice->CreateComputePipeline(m_ClearRadiancePipeline);
	renderDevice->CreateComputePipeline(m_InjectRadiancePipeline);
	renderDevice->CreateComputePipeline(m_InjectPropagationPipeline);

	renderDevice->CreateComputePipeline(m_DownSampleVisibilityPipeline);
	renderDevice->CreateComputePipeline(m_DownSampleRadiancePipeline);

	renderDevice->CreatePipeline(m_VoxelDrawPipeline);
	renderDevice->CreatePipeline(m_VoxelWireFrameDrawPipeline);

	renderDevice->CreateSampler(m_CloestSampler);
	renderDevice->CreateSampler(m_LinearSampler);

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateCommandBuffer(m_PrimaryCommandBuffer);
	m_PrimaryCommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	renderDevice->CreateRenderTarget(m_VoxelRenderPassTarget);
	renderDevice->CreateRenderPass(m_VoxelRenderPass);

	renderDevice->CreateRenderTarget(m_LightPassTarget);
	renderDevice->CreateRenderPass(m_LightPassRenderPass);

	renderDevice->CreateCommandBuffer(m_DrawCommandBuffer);
	m_DrawCommandBuffer->Init(m_CommandPool, CBL_SECONDARY);
	renderDevice->CreateCommandBuffer(m_LightingCommandBuffer);
	m_LightingCommandBuffer->Init(m_CommandPool, CBL_SECONDARY);

	const glm::vec3& cameraPos = m_Camera->GetPosition();
	for (uint32_t i = 0; i < m_ClipLevelCount; ++i)
	{
		KClipmapVoxilzerLevel newLevel = KClipmapVoxilzerLevel(this, i);
		glm::ivec3 pos = newLevel.WorldPositionToClipCoord(cameraPos);
		pos -= glm::ivec3(newLevel.GetExtent() / 2);
		newLevel.SetMinPosition(pos);
		m_ClipLevels.push_back(newLevel);
	}

	Resize(width, height);
	SetupVoxelReleatedData();

	m_Scene->RegisterEntityObserver(&m_OnSceneChangedFunc);

	return true;
}

bool KClipmapVoxilzer::UnInit()
{
	if (m_Scene)
	{
		m_Scene->UnRegisterEntityObserver(&m_OnSceneChangedFunc);
	}
	m_Scene = nullptr;
	m_Camera = nullptr;

	SAFE_UNINIT(m_VoxelRenderPass);
	SAFE_UNINIT(m_VoxelRenderPassTarget);

	SAFE_UNINIT(m_LightPassTarget);
	SAFE_UNINIT(m_LightPassRenderPass);

	SAFE_UNINIT(m_DrawCommandBuffer);
	SAFE_UNINIT(m_LightingCommandBuffer);
	SAFE_UNINIT(m_PrimaryCommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	SAFE_UNINIT(m_VoxelDrawPipeline);
	SAFE_UNINIT(m_VoxelWireFrameDrawPipeline);

	SAFE_UNINIT(m_CloestSampler);
	SAFE_UNINIT(m_LinearSampler);

	SAFE_UNINIT(m_InjectPropagationPipeline);
	SAFE_UNINIT(m_InjectRadiancePipeline);
	SAFE_UNINIT(m_ClearRadiancePipeline);
	SAFE_UNINIT(m_ClearRegionPipeline);
	SAFE_UNINIT(m_DownSampleVisibilityPipeline);
	SAFE_UNINIT(m_DownSampleRadiancePipeline);

	SAFE_UNINIT(m_StaticFlag);
	SAFE_UNINIT(m_VoxelAlbedo);
	SAFE_UNINIT(m_VoxelNormal);
	SAFE_UNINIT(m_VoxelEmissive);
	SAFE_UNINIT(m_VoxelRadiance);
	SAFE_UNINIT(m_VoxelVisibility);

	m_ClipLevels.clear();

	return true;
}