#include "KMeshLODView.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Publish/KCamera.h"
#include <cmath>

static float ProjectedHeight(const KAABBBox& box, const glm::mat4& modelView, const KCamera& camera)
{
	const float infinity = std::numeric_limits<float>::infinity();
	if (!box.IsDefault())
	{
		return infinity;
	}
	const auto& projection = camera.GetProjectiveMatrix();
	const glm::vec3& min = box.GetMin();
	const glm::vec3& max = box.GetMax();
	float bottom = infinity, top = -infinity;
	for (uint32_t corner = 0; corner < 8; ++corner)
	{
		const glm::vec4 view = modelView * glm::vec4(
			corner & 1 ? max.x : min.x, corner & 2 ? max.y : min.y, corner & 4 ? max.z : min.z, 1.0f);
		if (view.z >= -camera.GetNear())
		{
			return infinity;
		}
		const glm::vec4 clip = projection * view;
		const float height = clip.y / clip.w;
		if (!std::isfinite(height))
		{
			return infinity;
		}
		bottom = std::min(bottom, height);
		top = std::max(top, height);
	}
	return (top - bottom) * 0.5f;
}

static uint8_t SelectLOD(float height, uint8_t previous)
{
	constexpr float thresholds[] = { 0.25f, 0.08f };
	constexpr float hysteresis = 0.10f;
	if (previous >= KSubMesh::LOD_COUNT)
	{
		return height >= thresholds[0] ? 0 : height >= thresholds[1] ? 1 : 2;
	}
	while (previous > 0 && height >= thresholds[previous - 1] * (1.0f + hysteresis))
	{
		--previous;
	}
	while (previous + 1u < KSubMesh::LOD_COUNT && height < thresholds[previous] * (1.0f - hysteresis))
	{
		++previous;
	}
	return previous;
}

void KMeshLODView::Update(const std::vector<IKEntity*>& entities, const KCamera& camera, int forcedLevel)
{
	if (m_ForcedLevel != forcedLevel)
	{
		m_Entities.clear();
		m_ForcedLevel = forcedLevel;
	}
	++m_Frame;
	m_Stats = {};

	for (IKEntity* entity : entities)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (!entity->GetComponent(CT_RENDER, &render) || !entity->GetComponent(CT_TRANSFORM, &transform))
		{
			continue;
		}
		const auto& slots = render->GetMaterialSubMeshs();
		if (slots.empty() || render->IsUtility())
		{
			continue;
		}
		auto& selection = m_Entities[entity->GetID()];
		if (selection.binding.lock() != slots.front() || selection.levels.size() != slots.size())
		{
			selection.binding = slots.front();
			selection.history.assign(slots.size(), KSubMesh::LOD_COUNT);
			selection.levels.assign(slots.size(), 0);
		}
		selection.lastVisibleFrame = m_Frame;
		const glm::mat4 modelView = camera.GetViewMatrix() * transform->GetFinal_RenderThread();
		for (size_t slot = 0; slot < slots.size(); ++slot)
		{
			const auto base = slots[slot]->GetSubMesh();
			const auto material = slots[slot]->GetMaterial();
			if (!base || !material || material->GetShadingMode() != MSM_OPAQUE)
			{
				selection.levels[slot] = 0;
				continue;
			}

			uint32_t level = 0;
			if (base->ResolveLOD(KSubMesh::LOD_COUNT - 1))
			{
				if (forcedLevel < 0)
				{
					auto& history = selection.history[slot];
					history = SelectLOD(ProjectedHeight(base->GetBound(), modelView, camera), history);
					level = base->ResolveLOD(history);
				}
				else
				{
					level = base->ResolveLOD(static_cast<uint32_t>(forcedLevel));
				}
			}
			selection.levels[slot] = static_cast<uint8_t>(level);
			const uint32_t count = base->GetLOD(level)->GetDrawElementCount();
			if (count)
			{
				++m_Stats.subMeshes[level];
				m_Stats.triangles[level] += count / 3;
			}
		}
	}
	for (auto entry = m_Entities.begin(); entry != m_Entities.end();)
	{
		if (entry->second.binding.expired())
		{
			entry = m_Entities.erase(entry);
		}
		else
		{
			++entry;
		}
	}
}

const std::vector<uint8_t>* KMeshLODView::GetLevels(IKEntity::IDType entity) const
{
	const auto entry = m_Entities.find(entity);
	return entry != m_Entities.end() && entry->second.lastVisibleFrame == m_Frame ? &entry->second.levels : nullptr;
}
