#include "KRenderUtil.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KDebugComponent.h"
#include "KBase/Interface/Entity/IKEntity.h"

namespace KRenderUtil
{
	void CalculateInstancesByMesh(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances)
	{
		instances.clear();
		std::map<KSubMeshPtr, size_t> subMeshIndexMap;

		for (IKEntity* entity : entities)
		{
			KRenderComponent* render = nullptr;
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&render) && entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
			{
				const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();

				for (size_t i = 0; i < materialSubMeshes.size(); ++i)
				{
					size_t index = 0;
					KSubMeshPtr subMesh = materialSubMeshes[i]->GetSubMesh();

					auto it = subMeshIndexMap.find(subMesh);
					if (it != subMeshIndexMap.end())
					{
						index = it->second;
					}
					else
					{
						index = instances.size();
						subMeshIndexMap[subMesh] = index;
						instances.push_back({ materialSubMeshes[i], {} });
					}

					KMaterialSubMeshInstance& subMeshInstance = instances[index];

					const KConstantDefinition::OBJECT& finalTransform = transform->FinalTransform();
					glm::mat4 curTransform = glm::transpose(finalTransform.MODEL);
					glm::mat4 prevTransform = glm::transpose(finalTransform.PRVE_MODEL);
					subMeshInstance.instanceData.push_back(KVertexDefinition::INSTANCE_DATA_MATRIX4F(curTransform[0], curTransform[1], curTransform[2], prevTransform[0], prevTransform[1], prevTransform[2]));
				}
			}
		}
	}

	void CalculateInstancesByMaterial(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances)
	{
		std::map<std::tuple<KSubMeshPtr, IKMaterialPtr>, size_t> materialSubMeshIndexMap;

		for (IKEntity* entity : entities)
		{
			KRenderComponent* render = nullptr;
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&render) && entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
			{
				const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();

				for (size_t i = 0; i < materialSubMeshes.size(); ++i)
				{
					size_t index = 0;
					KSubMeshPtr subMesh = materialSubMeshes[i]->GetSubMesh();
					KMaterialRef material = materialSubMeshes[i]->GetMaterial();

					std::tuple<KSubMeshPtr, IKMaterialPtr> pair = std::make_tuple(subMesh, *material);

					auto it = materialSubMeshIndexMap.find(pair);
					if (it != materialSubMeshIndexMap.end())
					{
						index = it->second;
					}
					else
					{
						index = instances.size();
						materialSubMeshIndexMap[pair] = index;
						instances.push_back({ materialSubMeshes[i], {} });
					}

					KMaterialSubMeshInstance& subMeshInstance = instances[index];

					const KConstantDefinition::OBJECT& finalTransform = transform->FinalTransform();
					glm::mat4 curTransform = glm::transpose(finalTransform.MODEL);
					glm::mat4 prevTransform = glm::transpose(finalTransform.PRVE_MODEL);
					subMeshInstance.instanceData.push_back(KVertexDefinition::INSTANCE_DATA_MATRIX4F(curTransform[0], curTransform[1], curTransform[2], prevTransform[0], prevTransform[1], prevTransform[2]));
				}
			}
		}
	}

	void CalculateInstancesByVirtualTexture(const std::vector<IKEntity*>& entities, uint32_t targetBinding, IKTexturePtr virtualTexture, std::vector<KMaterialSubMeshInstance>& instances)
	{
		for (IKEntity* entity : entities)
		{
			KRenderComponent* render = nullptr;
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&render) && entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
			{
				const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();
				for (size_t i = 0; i < materialSubMeshes.size(); ++i)
				{
					KMaterialRef material = materialSubMeshes[i]->GetMaterial();
					IKMaterialTextureBindingPtr textureBinding = material->GetTextureBinding();

					if (textureBinding->GetIsVirtualTexture(targetBinding) && textureBinding->GetTexture(targetBinding) == virtualTexture)
					{
						const KConstantDefinition::OBJECT& finalTransform = transform->FinalTransform();
						glm::mat4 curTransform = glm::transpose(finalTransform.MODEL);
						glm::mat4 prevTransform = glm::transpose(finalTransform.PRVE_MODEL);
						instances.push_back({ materialSubMeshes[i], {KVertexDefinition::INSTANCE_DATA_MATRIX4F(curTransform[0], curTransform[1], curTransform[2], prevTransform[0], prevTransform[1], prevTransform[2])} });
					}
				}
			}
		}
	}

	void GetInstances(const std::vector<IKEntity*>& entities, std::vector<KMaterialSubMeshInstance>& instances, KMaterialSubMeshInstanceCompareFunction comp)
	{
		instances.clear();

		for (IKEntity* entity : entities)
		{
			KRenderComponent* render = nullptr;
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&render) && entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
			{
				const std::vector<KMaterialSubMeshPtr>& materialSubMeshes = render->GetMaterialSubMeshs();

				for (size_t i = 0; i < materialSubMeshes.size(); ++i)
				{
					const KConstantDefinition::OBJECT& finalTransform = transform->FinalTransform();
					glm::mat4 curTransform = glm::transpose(finalTransform.MODEL);
					glm::mat4 prevTransform = glm::transpose(finalTransform.PRVE_MODEL);

					KMaterialSubMeshInstance subMeshInstance;
					subMeshInstance.materialSubMesh = materialSubMeshes[i];
					subMeshInstance.instanceData.push_back(KVertexDefinition::INSTANCE_DATA_MATRIX4F(curTransform[0], curTransform[1], curTransform[2], prevTransform[0], prevTransform[1], prevTransform[2]));

					instances.push_back(subMeshInstance);
				}
			}
		}

		std::sort(instances.begin(), instances.end(), comp);
	}

	bool AssignShadingParameter(KRenderCommand& command, KMaterialRef material)
	{
		if (material)
		{
			const IKMaterialParameterPtr parameter = material->GetParameter();
			if (!parameter)
			{
				return false;
			}

			const KShaderInformation::Constant* constant = material->GetShadingInfo();
			if (!constant)
			{
				return false;
			}

			if (constant->size)
			{
				std::vector<char> fsShadingBuffer;
				fsShadingBuffer.resize(constant->size);

				KDynamicConstantBufferUsage shadingUsage;
				shadingUsage.binding = SHADER_BINDING_SHADING;
				shadingUsage.range = constant->size;

				for (const KShaderInformation::Constant::ConstantMember& member : constant->members)
				{
					IKMaterialValuePtr value = parameter->GetValue(member.name);
					ASSERT_RESULT(value);
					memcpy(POINTER_OFFSET(fsShadingBuffer.data(), member.offset), value->GetData(), member.size);
				}

				KRenderGlobal::DynamicConstantBufferManager.Alloc(fsShadingBuffer.data(), shadingUsage);
				command.dynamicConstantUsages.push_back(shadingUsage);
			}

			uint32_t virtualID[MAX_MATERIAL_TEXTURE_BINDING] = { 0 };
			bool hasVirtualTexture = false;

			IKMaterialTextureBindingPtr textureBinding = material->GetTextureBinding();
			if (textureBinding)
			{
				for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURE_BINDING; ++i)
				{
					if (textureBinding->GetIsVirtualTexture(i))
					{
						KVirtualTexture* virtualTexture = (KVirtualTexture*)textureBinding->GetVirtualTextureSoul(i);
						virtualID[i] = virtualTexture->GetVirtualID();
						hasVirtualTexture = true;
					}
				}
			}

			if (hasVirtualTexture)
			{
				KDynamicConstantBufferUsage virtualBindingUsage;
				virtualBindingUsage.binding = SHADER_BINDING_VIRTUAL_TEXTURE_BINDING;
				virtualBindingUsage.range = sizeof(virtualID);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(virtualID, virtualBindingUsage);
				command.dynamicConstantUsages.push_back(virtualBindingUsage);
			}

			return true;
		}
		return false;
	}

	bool AssignVirtualFeedbackParameter(KRenderCommand& command, uint32_t targetBinding, KVirtualTexture* virtualTexture)
	{
		struct
		{
			glm::uvec4 misc;
			glm::uvec4 misc2;
		} feedbackData;

		feedbackData.misc[0] = targetBinding;

		if (virtualTexture)
		{
			feedbackData.misc[1] = virtualTexture->GetVirtualID();
			feedbackData.misc[2] = (uint32_t)virtualTexture->GetTableTexture()->GetWidth();
			feedbackData.misc[3] = (uint32_t)virtualTexture->GetTableTexture()->GetHeight();
			feedbackData.misc2[1] = (uint32_t)virtualTexture->GetTableTexture()->GetMipmaps();
		}
		else
		{
			feedbackData.misc[1] = 0xFFFFFFFF;
			feedbackData.misc[2] = 0;
			feedbackData.misc[3] = 0;
			feedbackData.misc2[1] = 0;
		}

		feedbackData.misc2[0] = 0;
		feedbackData.misc2[2] = feedbackData.misc[2] * KRenderGlobal::VirtualTextureManager.GetTileSize();
		feedbackData.misc2[3] = feedbackData.misc[3] * KRenderGlobal::VirtualTextureManager.GetTileSize();

		KDynamicConstantBufferUsage virtualUsage;
		virtualUsage.binding = SHADER_BINDING_VIRTUAL_TEXTURE_FEEDBACK_TARGET;
		virtualUsage.range = sizeof(feedbackData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&feedbackData, virtualUsage);

		command.dynamicConstantUsages.push_back(virtualUsage);

		return true;
	}
}