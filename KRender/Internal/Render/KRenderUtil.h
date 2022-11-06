#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/KRenderGlobal.h"
#include "KRender/Interface/IKRenderCommand.h"
#include "KBase/Interface/Entity/IKEntity.h"

struct KRenderUtil
{
	struct InstanceGroup
	{
		KRenderComponent* render;
		std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F> instance;

		InstanceGroup()
		{
			render = nullptr;
		}
	};

	typedef std::shared_ptr<InstanceGroup> InstanceGroupPtr;
	typedef std::unordered_map<KMeshPtr, InstanceGroupPtr> MeshInstanceGroup;

	struct MaterialMap
	{
		std::unordered_map<IKMaterialPtr, InstanceGroupPtr> mapping;
	};
	typedef std::shared_ptr<MaterialMap> MaterialMapPtr;
	typedef std::unordered_map<KMeshPtr, MaterialMapPtr> MeshMaterialInstanceGroup;

	static void CalculateInstanceGroupByMesh(const std::vector<KRenderComponent*>& cullRes, MeshInstanceGroup& meshGroups)
	{
		for (KRenderComponent* component : cullRes)
		{
			IKEntity* entity = component->GetEntityHandle();
			KTransformComponent* transform = nullptr;
			KRenderComponent* render = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform) && entity->GetComponent(CT_RENDER, (IKComponentBase**)&render))
			{
				InstanceGroupPtr instanceGroup = nullptr;
				KMeshPtr mesh = component->GetMesh();

				auto it = meshGroups.find(mesh);
				if (it != meshGroups.end())
				{
					instanceGroup = it->second;
				}
				else
				{
					instanceGroup = std::make_shared<InstanceGroup>();
					meshGroups[mesh] = instanceGroup;
				}

				instanceGroup->render = render;		

				const KConstantDefinition::OBJECT& finalTransform = transform->FinalTransform();
				glm::mat4 curTransform = glm::transpose(finalTransform.MODEL);
				glm::mat4 prevTransform = glm::transpose(finalTransform.PRVE_MODEL);
				instanceGroup->instance.push_back(KVertexDefinition::INSTANCE_DATA_MATRIX4F(curTransform[0], curTransform[1], curTransform[2], prevTransform[0], prevTransform[1], prevTransform[2]));
			}
		}
	}

	static void CalculateInstanceGroupByMaterial(const std::vector<KRenderComponent*>& cullRes, bool useMateiralTex, MeshMaterialInstanceGroup& meshMaterialGroups)
	{
		for (KRenderComponent* render : cullRes)
		{
			if (!render->IsOcclusionVisible())
			{
				continue;
			}

			if (render->GetUseMaterialTexture() != useMateiralTex)
			{
				continue;
			}

			IKEntity* entity = render->GetEntityHandle();

			KTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
			{
				MaterialMapPtr mateiralMap = nullptr;
				InstanceGroupPtr instanceArray = nullptr;

				KMeshPtr mesh = render->GetMesh();

				auto itMesh = meshMaterialGroups.find(mesh);
				if (itMesh != meshMaterialGroups.end())
				{
					mateiralMap = itMesh->second;
				}
				else
				{
					mateiralMap = std::make_shared<MaterialMap>();
					meshMaterialGroups[mesh] = mateiralMap;
				}
				ASSERT_RESULT(mateiralMap);

				IKMaterialPtr material = render->GetMaterial();

				auto itMat = mateiralMap->mapping.find(material);
				if (itMat == mateiralMap->mapping.end())
				{
					instanceArray = std::make_shared<InstanceGroup>();
					(mateiralMap->mapping)[material] = instanceArray;
				}
				else
				{
					instanceArray = itMat->second;
				}

				instanceArray->render = render;

				const KConstantDefinition::OBJECT& finalTransform = transform->FinalTransform();
				glm::mat4 curTransform = glm::transpose(finalTransform.MODEL);
				glm::mat4 prevTransform = glm::transpose(finalTransform.PRVE_MODEL);
				instanceArray->instance.push_back(KVertexDefinition::INSTANCE_DATA_MATRIX4F(curTransform[0], curTransform[1], curTransform[2], prevTransform[0], prevTransform[1], prevTransform[2]));
			}
		}
	}

	static bool AssignShadingParameter(KRenderCommand& command, IKMaterial* material, bool useMaterialTex)
	{
		if (material)
		{
			const IKMaterialParameterPtr vsParameter = material->GetVSParameter();
			const IKMaterialParameterPtr fsParameter = material->GetFSParameter();
			if (!(vsParameter && fsParameter))
			{
				return false;
			}

			const KShaderInformation::Constant* vsConstant = material->GetVSShadingInfo();
			const KShaderInformation::Constant* fsConstant = material->GetFSShadingInfo();
			if (!(vsConstant && fsConstant))
			{
				return false;
			}

			if (vsConstant->size)
			{
				static std::vector<char> vsShadingBuffer;

				command.vertexShadingUsage.binding = SHADER_BINDING_VERTEX_SHADING;
				command.vertexShadingUsage.range = vsConstant->size;

				if (vsConstant->size > vsShadingBuffer.size())
				{
					vsShadingBuffer.resize(vsConstant->size);
				}

				for (const KShaderInformation::Constant::ConstantMember& member : vsConstant->members)
				{
					IKMaterialValuePtr value = vsParameter->GetValue(member.name);
					ASSERT_RESULT(value);
					memcpy(POINTER_OFFSET(vsShadingBuffer.data(), member.offset), value->GetData(), member.size);
				}

				KRenderGlobal::DynamicConstantBufferManager.Alloc(vsShadingBuffer.data(), command.vertexShadingUsage);
			}

			if (fsConstant->size)
			{
				static std::vector<char> fsShadingBuffer;

				command.fragmentShadingUsage.binding = SHADER_BINDING_FRAGMENT_SHADING;
				command.fragmentShadingUsage.range = fsConstant->size;

				if (fsConstant->size > fsShadingBuffer.size())
				{
					fsShadingBuffer.resize(fsConstant->size);
				}

				for (const KShaderInformation::Constant::ConstantMember& member : fsConstant->members)
				{
					IKMaterialValuePtr value = fsParameter->GetValue(member.name);
					ASSERT_RESULT(value);
					memcpy(POINTER_OFFSET(fsShadingBuffer.data(), member.offset), value->GetData(), member.size);
				}

				KRenderGlobal::DynamicConstantBufferManager.Alloc(fsShadingBuffer.data(), command.fragmentShadingUsage);
			}

			const IKMaterialTextureBinding* textureBinding = useMaterialTex ? material->GetDefaultMaterialTexture().get() : command.textureBinding;

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

	static bool AssignMeshStorageParameter(KRenderCommand& command)
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
};