#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanDepthBuffer.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	VkRenderPass	m_RenderPass;
	VkFramebuffer	m_FrameBuffer;
	VkFormat		m_ColorFormat;
	VkClearValue	m_ColorClear;
	VkClearValue	m_DepthStencilClear;
	VkExtent2D		m_Extend;

	VkImage			m_ColorImage;
	VkImageView		m_ColorImageView;

	bool			m_bMsaaCreated;
	VkSampleCountFlagBits m_MsaaFlag;
	VkImage			m_MsaaImage;
	VkImageView		m_MsaaImageView;

	KVulkanHeapAllocator::AllocInfo m_MsaaAlloc;

	KVulkanDepthBufferPtr m_pDepthBuffer;

	bool CreateImage(void* imageHandle, void* imageFormatHandle, bool bDepthStencil, unsigned short uMsaaCount);
	bool CreateFramebuffer();
public:
	KVulkanRenderTarget();
	~KVulkanRenderTarget();

	virtual bool SetSize(size_t width, size_t height);
	virtual bool SetColorClear(float r, float g, float b, float a);
	virtual bool SetDepthStencilClear(float depth, unsigned int stencil);

	virtual bool InitFromImage(void* imageHandle, void* imageFormatHandle, bool bDepthStencil, unsigned short uMsaaCount);
	virtual bool UnInit();

	inline VkRenderPass GetRenderPass() { return m_RenderPass; }
	inline VkFramebuffer GetFrameBuffer() { return m_FrameBuffer; }
	inline VkSampleCountFlagBits GetMsaaFlag(){ return m_MsaaFlag; }
};