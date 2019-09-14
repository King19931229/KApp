#include "Internal/KBufferBase.h"
#include "Vulkan/vulkan.h"

class KVulkanVertexBuffer : public KVertexBufferBase
{
protected:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkDeviceMemory;

	bool m_bDeviceInit;
public:
	KVulkanVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice);
	~KVulkanVertexBuffer();

	virtual bool InitDevice();
	virtual bool UnInit();

	virtual bool Write(const void* pData);
	virtual bool Read(const void* pData);

	virtual bool CopyFrom(IKVertexBufferPtr pSource);
	virtual bool CopyTo(IKVertexBufferPtr pDest);

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
};