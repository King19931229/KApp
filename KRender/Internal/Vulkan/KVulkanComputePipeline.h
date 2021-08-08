#pragma once
#include "Interface/IKComputePipeline.h"
#include "KVulkanConfig.h"
#include <unordered_map>

class KVulkanComputePipeline : public IKComputePipeline
{
protected:
	struct Descriptor
	{
		VkDescriptorSetLayout layout;
		VkDescriptorPool pool;
		std::vector<VkDescriptorSet> sets;

		Descriptor()
		{
			layout = VK_NULL_HANDEL;
			pool = VK_NULL_HANDEL;
		}
	};
	Descriptor m_Descriptor;

	struct Pipeline
	{
		VkPipelineLayout layout;
		VkPipeline pipeline;

		Pipeline()
		{
			layout = VK_NULL_HANDEL;
			pipeline = VK_NULL_HANDEL;
		}
	};
	Pipeline m_Pipeline;

	struct BindingInfo
	{
		IKRenderTargetPtr target;
		IKAccelerationStructurePtr as;
		bool dynamicWrite;

		BindingInfo()
		{
			target = nullptr;
			as = nullptr;
			dynamicWrite = false;
		}
	};
	std::unordered_map<unsigned int, BindingInfo> m_Bindings;
	IKShaderPtr m_ComputeShader;

	void CreateDescriptorSet();
	void CreatePipeline();

	void DestroyDescriptorSet();
	void DestroyPipeline();

	VkWriteDescriptorSet PopulateImageWrite(IKRenderTargetPtr target, VkDescriptorSet dstSet, uint32_t dstBinding);
	VkWriteDescriptorSet PopulateTopdownASWrite(IKAccelerationStructurePtr as, VkDescriptorSet dstSet, uint32_t dstBinding);

	bool UpdateDynamicWrite(uint32_t frameIndex);
public:
	KVulkanComputePipeline();
	~KVulkanComputePipeline();

	virtual void SetStorageImage(uint32_t location, IKRenderTargetPtr target, bool dynimicWrite);
	virtual void SetAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynimicWrite);

	virtual bool Init(const char* szShader);
	virtual bool UnInit();
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);
};