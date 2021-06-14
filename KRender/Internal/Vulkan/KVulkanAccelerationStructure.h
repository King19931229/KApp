#pragma once
#include "Interface/IKAccelerationStructure.h"
#include "KVulkanConfig.h"
#include "KVulkanInitializer.h"

class KVulkanAccelerationStructure : public IKAccelerationStructure
{
protected:
	KVulkanInitializer::AccelerationStructureHandle m_BottomUpAS;
public:
	KVulkanAccelerationStructure();
	~KVulkanAccelerationStructure();

	virtual bool Init(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer);
	virtual bool UnInit();
};