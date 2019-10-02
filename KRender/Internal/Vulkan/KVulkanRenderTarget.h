#include "Interface/IKRenderTarget.h"
#include "KVulkanConfig.h"
#include "KVulkanDepthBuffer.h"
#include "KVulkanHeapAllocator.h"

class KVulkanRenderTarget : public IKRenderTarget
{
protected:
	VkRenderPass	m_RenderPass;
	VkFramebuffer	m_FrameBuffer;

	enum ClearTarget
	{
		CT_COLOR,
		CT_DEPTH_STENCIL,

		CT_COUNT
	};
	VkClearValue	m_ClearValues[CT_COUNT];

	VkExtent2D		m_Extend;

	VkFormat		m_ColorFormat;
	VkImage			m_ColorImage;
	VkImageView		m_ColorImageView;

	VkFormat		m_DepthFormat;
	VkImage			m_DepthImage;
	VkImageView		m_DepthImageView;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;

	VkSampleCountFlagBits m_MsaaFlag;
	VkImage			m_MsaaImage;
	VkImageView		m_MsaaImageView;
	KVulkanHeapAllocator::AllocInfo m_MsaaAlloc;

	bool			m_bMsaaCreated;
	bool			m_bDepthStencilCreated;

	static VkFormat FindDepthFormat(bool bStencil);
	bool CreateImage(void* imageHandle, void* imageFormatHandle, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool CreateFramebuffer();
public:
	KVulkanRenderTarget();
	~KVulkanRenderTarget();

	virtual bool SetSize(size_t width, size_t height);
	virtual bool SetColorClear(float r, float g, float b, float a);
	virtual bool SetDepthStencilClear(float depth, unsigned int stencil);

	virtual bool InitFromImage(void* imageHandle, void* imageFormatHandle,
		bool bDepth,
		bool bStencil,
		unsigned short uMsaaCount);

	virtual bool UnInit();

	inline VkRenderPass GetRenderPass() { return m_RenderPass; }
	inline VkFramebuffer GetFrameBuffer() { return m_FrameBuffer; }
	inline VkSampleCountFlagBits GetMsaaFlag(){ return m_MsaaFlag; }
	inline VkExtent2D GetExtend() { return m_Extend; }

	typedef std::pair<VkClearValue*, unsigned int> ClearValues;
	ClearValues GetVkClearValues();
};