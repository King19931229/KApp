#include "KRenderUtil.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "KBase/Interface/Entity/IKEntity.h"

namespace KRenderUtil
{
	void CalculateInstancesByMesh(const std::vector<KRenderComponent*>& renderComponents, std::vector<KMaterialSubMeshInstance>& instances)
	{
		instances.clear();
		std::map<KSubMeshPtr, size_t> subMeshIndexMap;

		for (KRenderComponent* render : renderComponents)
		{
			IKEntity* entity = render->GetEntityHandle();
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
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

	void CalculateInstanceByMaterial(const std::vector<KRenderComponent*>& renderComponents, std::vector<KMaterialSubMeshInstance>& instances)
	{
		std::map<std::tuple<KSubMeshPtr, IKMaterialPtr>, size_t> materialSubMeshIndexMap;

		for (KRenderComponent* render : renderComponents)
		{
			IKEntity* entity = render->GetEntityHandle();
			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
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

				command.shadingUsage.binding = SHADER_BINDING_SHADING;
				command.shadingUsage.range = constant->size;

				for (const KShaderInformation::Constant::ConstantMember& member : constant->members)
				{
					IKMaterialValuePtr value = parameter->GetValue(member.name);
					ASSERT_RESULT(value);
					memcpy(POINTER_OFFSET(fsShadingBuffer.data(), member.offset), value->GetData(), member.size);
				}

				KRenderGlobal::DynamicConstantBufferManager.Alloc(fsShadingBuffer.data(), command.shadingUsage);
			}

			const IKMaterialTextureBinding* textureBinding = material->GetTextureBinding().get();

			uint8_t numSlot = textureBinding->GetNumSlot();
			for (uint8_t i = 0; i < numSlot; ++i)
			{
				IKTexturePtr texture = textureBinding->GetTexture(i);
				IKSamplerPtr sampler = textureBinding->GetSampler(i);
				if (texture && sampler)
				{
					command.pipeline->SetSampler(SHADER_BINDING_MATERIAL_BEGIN + i, texture->GetFrameBuffer(), sampler, true);
				}
			}

			return true;
		}
		return false;
	}

	bool AssignMeshStorageParameter(KRenderCommand& command)
	{
		KStorageBufferUsage usage;

		for (size_t i = 0; i < command.vertexData->vertexFormats.size(); ++i)
		{
			VertexFormat format = command.vertexData->vertexFormats[i];
			IKVertexBufferPtr buffer = command.vertexData->vertexBuffers[i];

			uint32_t binding = ~0;

			switch (format)
			{
				case VF_POINT_NORMAL_UV:
					binding = SBT_POSITION_NORMAL_UV;
					break;
				case VF_DIFFUSE_SPECULAR:
					binding = SBT_DIFFUSE_SPECULAR;
					break;
				case VF_TANGENT_BINORMAL:
					binding = SBT_TANGENT_BINORMAL;
					break;
				default:
					break;
			}

			if (binding != ~0)
			{
				usage.binding = binding;
				usage.buffer = buffer;
				command.meshStorageUsages.push_back(usage);
			}
		}

		usage.binding = SBT_MESHLET_DESC;
		usage.buffer = command.meshData->meshletDescBuffer;
		command.meshStorageUsages.push_back(usage);

		usage.binding = SBT_MESHLET_PRIM;
		usage.buffer = command.meshData->meshletPrimBuffer;
		command.meshStorageUsages.push_back(usage);

		return true;
	}
}