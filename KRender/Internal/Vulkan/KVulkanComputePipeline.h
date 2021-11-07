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
		struct
		{
			ComputeImageFlag flag;
			ElementFormat format;
			std::vector<IKFrameBufferPtr> images;
			std::vector<IKSamplerPtr> samplers;
			std::vector<VkDescriptorImageInfo> imageDescriptors;
		}image;

		struct
		{
			IKAccelerationStructurePtr as;
			VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureDescriptor;
		}as;

		struct
		{
			IKUniformBufferPtr buffer;
			VkDescriptorBufferInfo bufferDescriptor;
		}buffer;

		enum
		{
			SAMPLER,
			IMAGE,
			AS,
			BUFFER,
			DYNAMIC_BUFFER,
			UNKNOWN
		}type;

		bool dynamicWrite;

		BindingInfo()
		{
			image.flag = COMPUTE_IMAGE_IN;
			image.format = EF_UNKNOWN;
			image.imageDescriptors = {};

			as.as = nullptr;
			as.accelerationStructureDescriptor = {};

			buffer.buffer = nullptr;
			buffer.bufferDescriptor = {};

			type = UNKNOWN;
			dynamicWrite = false;
		}
	};
	std::unordered_map<unsigned int, BindingInfo> m_Bindings;
	IKShaderPtr m_ComputeShader;

	void CreateDescriptorSet();
	void CreatePipeline();

	void DestroyDescriptorSet();
	void DestroyPipeline();

	VkWriteDescriptorSet PopulateImageWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);
	VkWriteDescriptorSet PopulateTopdownASWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);
	VkWriteDescriptorSet PopulateUniformBufferWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);

	VkWriteDescriptorSet PopulateDynamicUniformBufferWrite(BindingInfo& binding, const KDynamicConstantBufferUsage& usage, VkDescriptorSet dstSet);

	bool UpdateDynamicWrite(uint32_t frameIndex, const KDynamicConstantBufferUsage* usage = nullptr);
	bool SetupImageBarrier(IKCommandBufferPtr buffer, bool input);
public:
	KVulkanComputePipeline();
	~KVulkanComputePipeline();

	virtual void BindSampler(uint32_t location, IKFrameBufferPtr target, IKSamplerPtr sampler, bool dynimicWrite);
	virtual void BindStorageImage(uint32_t location, IKFrameBufferPtr target, ComputeImageFlag flag, bool dynimicWrite);
	virtual void BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynimicWrite);
	virtual void BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer, bool dynimicWrite);

	virtual void BindSamplers(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite);
	virtual void BindStorageImages(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, ComputeImageFlag flag, bool dynimicWrite);

	virtual void BindDyanmicUniformBuffer(uint32_t location);

	virtual void ReinterpretImageFormat(uint32_t location, ElementFormat format);

	virtual bool Init(const char* szShader);
	virtual bool UnInit();
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, uint32_t frameIndex, const KDynamicConstantBufferUsage* usage);
	virtual bool ReloadShader();
};