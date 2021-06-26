#pragma once
#include "Interface/IKAccelerationStructure.h"
#include "KVulkanConfig.h"
#include "KVulkanInitializer.h"

struct KVulkanRayTraceInstance
{
	glm::mat4		transform;
	glm::mat4		transformIT;
	uint32_t		objIndex;
	uint32_t		txtOffset;
	VkDeviceAddress vertices;
	VkDeviceAddress indices;
	VkDeviceAddress materials;
	VkDeviceAddress materialIndices;
};

class KVulkanAccelerationStructure : public IKAccelerationStructure
{
protected:
	KVulkanInitializer::AccelerationStructureHandle m_BottomUpAS;
	KVulkanInitializer::AccelerationStructureHandle m_TopDownAS;
	std::vector<KVulkanRayTraceInstance> m_Instances;
	const class KVulkanIndexBuffer* m_IndexBuffer;
	const class KVulkanVertexBuffer* m_VertexBuffer;
public:
	KVulkanAccelerationStructure();
	~KVulkanAccelerationStructure();

	const KVulkanInitializer::AccelerationStructureHandle& GetBottomUp() const { return m_BottomUpAS; }
	const KVulkanVertexBuffer* GetVertexBuffer() const { return m_VertexBuffer; }
	const KVulkanIndexBuffer* GetIndexBuffer() const { return m_IndexBuffer; }

	const KVulkanInitializer::AccelerationStructureHandle& GetTopDown() const { return m_TopDownAS; }
	const std::vector<KVulkanRayTraceInstance>& GetInstances() const { return m_Instances; }

	virtual bool InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer);
	virtual bool InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs);
	virtual bool UnInit();
};