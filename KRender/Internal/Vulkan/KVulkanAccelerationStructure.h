#pragma once
#include "Interface/IKAccelerationStructure.h"
#include "KVulkanConfig.h"
#include "KVulkanInitializer.h"

class KVulkanAccelerationStructure : public IKAccelerationStructure
{
protected:
	KVulkanInitializer::AccelerationStructureHandle m_BottomUpAS;
	KVulkanInitializer::AccelerationStructureHandle m_TopDownAS;
public:
	KVulkanAccelerationStructure();
	~KVulkanAccelerationStructure();

	const KVulkanInitializer::AccelerationStructureHandle& GetBottomUp() const { return m_BottomUpAS; }
	const KVulkanInitializer::AccelerationStructureHandle& GetTopDown() const { return m_TopDownAS; }

	virtual bool InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer);
	virtual bool InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs);
	virtual bool UnInit();
};