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

	void MergeLayoutBinding(std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutBinding& newBinding);
	void AddLayoutBinding(const KShaderInformation& information, VkShaderStageFlags stageFlag, const KPipelineBinding& binding);
	void AddPushConstantRange(std::vector<VkPushConstantRange>& ranges, const KShaderInformation& information, VkShaderStageFlags stageFlag);
public:
	KVulkanPipelineLayout();
	~KVulkanPipelineLayout();

	bool Init(const KPipelineBinding& binding);
	bool UnInit();

	inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout;	}
	inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
	inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetLayoutBinding() const { return m_DescriptorSetLayoutBinding; }
};