#pragma once
#include "Internal/KPipelineBase.h"
#include "Internal/KRenderGlobal.h"
#include "KVulkanConfig.h"
#include "KVulkanDescripiorPool.h"
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
	VkPrimitiveTopology	m_PrimitiveTopology;

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
	virtual bool UnInit();
	virtual bool SetDebugName(const char* name);

	inline VkPipeline GetVkPipeline() { return m_GraphicsPipeline; }
};

class KVulkanPipeline : public KPipelineBase
{
	friend class KVulkanPipelineHandle;
	friend class KVulkanDescriptorPool;
protected:
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkPipelineLayout m_PipelineLayout;
	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBinding;

	std::vector<VkWriteDescriptorSet> m_WriteDescriptorSet;
	std::vector<VkDescriptorBufferInfo> m_BufferWriteInfo;

	uint32_t m_LastTrimFrame;
	static constexpr uint32_t MAX_THREAD_SUPPORT = 512;
	std::array<KVulkanDescriptorPool, MAX_THREAD_SUPPORT> m_Pools;
	std::array<bool, MAX_THREAD_SUPPORT> m_PoolInitializeds;
#ifdef _DEBUG
	std::array<std::atomic_bool, MAX_THREAD_SUPPORT> m_PoolInitialing;
#endif

	bool CreateDescriptionWrite();
	bool CreateDescriptionPool(uint32_t threadIndex);
	bool TrimDescriptionPool();
	bool DestroyDevice() override;
	bool CreateLayout() override;
public:
	KVulkanPipeline();
	virtual ~KVulkanPipeline();

	virtual bool Init();
	virtual bool UnInit();
	virtual bool Reload(bool reloadShader);

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }

	VkDescriptorSet AllocDescriptorSet(uint32_t threadIndex,
		const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
		const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount);
};