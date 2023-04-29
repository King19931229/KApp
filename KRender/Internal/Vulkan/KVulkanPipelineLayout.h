#pragma once
#include "KVulkanConfig.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKShader.h"

class KVulkanPipelineLayout : public IKPipelineLayout
{
protected:
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkPipelineLayout m_PipelineLayout;
	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBinding;
	size_t m_Hash;

	void MergeLayoutBinding(std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutBinding& newBinding);
	void AddLayoutBinding(const KShaderInformation& information, VkShaderStageFlags stageFlag);
	void AddPushConstantRange(std::vector<VkPushConstantRange>& ranges, const KShaderInformation& information, VkShaderStageFlags stageFlag);
public:
	KVulkanPipelineLayout();
	~KVulkanPipelineLayout();

	bool Init(const KPipelineBinding& binding);
	bool UnInit();

	inline size_t Hash() const { return m_Hash; }
	inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout;	}
	inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
	inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetLayoutBinding() const { return m_DescriptorSetLayoutBinding; }
};