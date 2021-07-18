#pragma once
#include "Interface/IKAccelerationStructure.h"
#include "KVulkanConfig.h"
#include "KVulkanInitializer.h"

struct KVulkanRayTraceInstance
{
	glm::mat4		transform;
	glm::mat4		transformIT;
	uint64_t		objIndex;
	VkDeviceAddress	materials;
	VkDeviceAddress vertices;
	VkDeviceAddress indices;
};

struct KVulkanRayTraceMaterial
{
	int32_t diffuseTex;
	int32_t specularTex;
	int32_t normalTex;
	int32_t placeholder;
};

class KVulkanAccelerationStructure : public IKAccelerationStructure
{
protected:
	struct MaterialBuffer
	{
		VkBuffer buffer;
		KVulkanHeapAllocator::AllocInfo allocInfo;

		MaterialBuffer()
		{
			buffer = VK_NULL_HANDEL;
		}
	};
	KVulkanInitializer::AccelerationStructureHandle m_BottomUpAS;
	KVulkanInitializer::AccelerationStructureHandle m_TopDownAS;

	std::vector<KVulkanRayTraceInstance> m_Instances;
	std::vector<MaterialBuffer> m_Materials;
	std::vector<VkDescriptorImageInfo> m_Textures;

	const class KVulkanIndexBuffer* m_IndexBuffer;
	const class KVulkanVertexBuffer* m_VertexBuffer;
	const class KMaterialTextureBinding* m_TextureBinding;
public:
	KVulkanAccelerationStructure();
	~KVulkanAccelerationStructure();

	const KVulkanInitializer::AccelerationStructureHandle& GetBottomUp() const { return m_BottomUpAS; }
	const KVulkanVertexBuffer* GetVertexBuffer() const { return m_VertexBuffer; }
	const KVulkanIndexBuffer* GetIndexBuffer() const { return m_IndexBuffer; }
	const KMaterialTextureBinding* GetTextureBinding() const { return m_TextureBinding; }

	const KVulkanInitializer::AccelerationStructureHandle& GetTopDown() const { return m_TopDownAS; }
	const std::vector<KVulkanRayTraceInstance>& GetInstances() const { return m_Instances; }

	virtual bool InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer, IKMaterialTextureBinding* textureBinding);
	virtual bool InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs);
	virtual bool UnInit();
};