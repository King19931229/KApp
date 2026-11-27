#pragma once
#include "Internal/KPipelineBase.h"
#include "Internal/KRenderGlobal.h"
#include "KVulkanConfig.h"
#include "KVulkanDescriptorManager.h"
#include <unordered_map>

class KVulkanRenderTarget;
class KVulkanPipeline;

class KVulkanPipelineHandle : public IKPipelineHandle
{
protected:
	VkPipeline m_GraphicsPipeline;

	// 顶点装配信息
	std::vector<VkVertexInputBindingDescription> m_BindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
	PrimitiveTopology m_PrimitiveTopology;

	// Alpha混合信息
	VkColorComponentFlags m_ColorWriteMask;

	VkBlendFactor m_ColorSrcBlendFactor;
	VkBlendFactor m_ColorDstBlendFactor;
	VkBlendOp m_ColorBlendOp;

	VkBlendFactor m_AlphaSrcBlendFactor;
	VkBlendFactor m_AlphaDstBlendFactor;
	VkBlendOp m_AlphaBlendOp;

	VkBool32 m_BlendEnable;

	// 光栅化信息
	VkPolygonMode m_PolygonMode;
	VkCullModeFlagBits m_CullMode;
	VkFrontFace m_FrontFace;

	// 深度信息
	VkBool32 m_DepthWrite;
	VkBool32 m_DepthTest;
	VkCompareOp m_DepthCompareOp;

	VkBool32 m_DepthBiasEnable;

	// 模板信息
	VkStencilOp m_StencilFailOp;
	VkStencilOp m_StencilDepthFailOp;
	VkStencilOp m_StencilPassOp;
	VkCompareOp m_StencilCompareOp;
	uint32_t m_StencilRef;
	VkBool32 m_StencilEnable;

	std::string m_Name;

	bool InitializeVulkanTerminology(const KPipelineState& state, const KPipelineBinding& binding);
public:
	KVulkanPipelineHandle();
	virtual~KVulkanPipelineHandle();

	virtual bool Init(IKPipelineLayout* layout, IKRenderPass* renderPass, const KPipelineState& state, const KPipelineBinding& binding);
	virtual bool UnInit(bool resourcesComplete = false);
	virtual bool SetDebugName(const char* name);

	inline VkPipeline GetVkPipeline() { return m_GraphicsPipeline; }
	inline PrimitiveTopology GetPrimitiveTopology() const { return m_PrimitiveTopology; }
};

class KVulkanPipeline : public KPipelineBase
{
	friend class KVulkanPipelineHandle;
	friend class KVulkanGraphicsDescriptorWriter;
protected:
	VkPipelineLayout m_PipelineLayout;

	static constexpr uint32_t MAX_THREAD_SUPPORT = 512;
	KVulkanDescriptorTicket* m_DescriptorTicket;

	bool DestroyDevice() override;
	bool CreateLayout() override;
public:
	KVulkanPipeline();
	virtual ~KVulkanPipeline();

	virtual bool Init();
	virtual bool UnInit(bool resourcesComplete = false);
	virtual bool Reload(bool reloadShader);

	virtual bool SetDebugName(const char* name);

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }

	VkDescriptorSet AllocDescriptorSet(uint32_t threadIndex, VkCommandPool commandPool,
		const KDynamicConstantBufferUsage** ppConstantUsage, uint32_t dynamicBufferUsageCount,
		const KStorageBufferUsage** ppStorageUsage, uint32_t storageBufferUsageCount);
};
