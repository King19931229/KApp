#pragma once
#include "Interface/IKComputePipeline.h"
#include "KVulkanConfig.h"
#include <unordered_map>

class KVulkanComputePipeline : public IKComputePipeline
{
protected:
	struct DescriptorSetBlock
	{
		VkDescriptorPool pool;
		std::vector<VkDescriptorSet> sets;
		uint32_t useCount;
		uint32_t maxCount;

		DescriptorSetBlock()
		{
			useCount = 0;
			maxCount = 8;
			pool = VK_NULL_HANDLE;
		}
	};

	struct DescriptorSetBlockList
	{
		std::vector<DescriptorSetBlock> blocks;
		size_t currentFrame;

		DescriptorSetBlockList()
		{
			currentFrame = 0;
		}
	};

	VkDescriptorSetLayout m_DescriptorLayout;
	std::vector<DescriptorSetBlockList> m_Descriptors;

	std::vector<VkDescriptorSetLayoutBinding> m_LayoutBindings;
	std::vector<VkDescriptorPoolSize> m_PoolSizes;
	std::vector<VkWriteDescriptorSet> m_StaticWrites;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;

	struct BindingInfo
	{
		struct
		{
			ElementFormat format;
			uint32_t mipmap;
			std::vector<IKFrameBufferPtr> images;
			std::vector<IKSamplerPtr> samplers;
			std::vector<VkDescriptorImageInfo> imageDescriptors;
		}image;

		struct
		{
			IKAccelerationStructurePtr handle;
			VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureDescriptor;
		}as;

		struct
		{
			IKUniformBufferPtr buffer;
			VkDescriptorBufferInfo bufferDescriptor;
		}uniform;

		struct
		{
			IKStorageBufferPtr buffer;
			VkDescriptorBufferInfo bufferDescriptor;
		}storage;

		enum
		{
			SAMPLER,
			IMAGE,
			AS,
			UNIFROM_BUFFER,
			STORAGE_BUFFER,
			DYNAMIC_UNIFROM_BUFFER,
			UNKNOWN
		}type;

		ComputeResourceFlags flags;
		bool dynamicWrite;

		BindingInfo()
		{
			image.format = EF_UNKNOWN;
			image.mipmap = 0;
			image.imageDescriptors = {};

			as.handle = nullptr;
			as.accelerationStructureDescriptor = {};

			uniform.buffer = nullptr;
			uniform.bufferDescriptor = {};

			storage.buffer = nullptr;
			storage.bufferDescriptor = {};

			type = UNKNOWN;
			flags = COMPUTE_RESOURCE_IN;
			dynamicWrite = false;
		}
	};
	std::unordered_map<unsigned int, BindingInfo> m_Bindings;
	KShaderRef m_ComputeShader;

	void CreateLayout();
	void CreatePipeline();

	void DestroyDescriptorSet();
	void DestroyPipeline();

	VkWriteDescriptorSet PopulateImageWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);
	VkWriteDescriptorSet PopulateTopdownASWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);
	VkWriteDescriptorSet PopulateUniformBufferWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);
	VkWriteDescriptorSet PopulateStorageBufferWrite(BindingInfo& binding, VkDescriptorSet dstSet, uint32_t dstBinding);

	VkWriteDescriptorSet PopulateDynamicUniformBufferWrite(BindingInfo& binding, const KDynamicConstantBufferUsage& usage, VkDescriptorSet dstSet);

	VkDescriptorPool CreateDescriptorPool(uint32_t maxCount);
	VkDescriptorSet AllocDescriptorSet(VkDescriptorPool pool);
	VkDescriptorSet Alloc(size_t frameIndex, size_t frameNum);

	bool UpdateDynamicWrite(VkDescriptorSet dstSet, const KDynamicConstantBufferUsage* usage = nullptr);
	bool SetupBarrier(IKCommandBufferPtr buffer, bool input);

	void PreDispatch(IKCommandBufferPtr primaryBuffer, VkDescriptorSet dstSet, const KDynamicConstantBufferUsage* usage);
	void PostDispatch(IKCommandBufferPtr primaryBuffer);
public:
	KVulkanComputePipeline();
	~KVulkanComputePipeline();

	virtual void BindSampler(uint32_t location, IKFrameBufferPtr target, IKSamplerPtr sampler, bool dynamicWrite);
	virtual void BindStorageImage(uint32_t location, IKFrameBufferPtr target, ElementFormat format, ComputeResourceFlags flags, uint32_t mipmap, bool dynamicWrite);
	virtual void BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynamicWrite);
	virtual void BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer);

	virtual void BindSamplers(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite);
	virtual void BindStorageImages(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, ElementFormat format, ComputeResourceFlags flags, uint32_t mipmap, bool dynimicWrite);

	virtual void BindStorageBuffer(uint32_t location, IKStorageBufferPtr buffer, ComputeResourceFlags flags, bool dynamicWrite);

	virtual void BindDynamicUniformBuffer(uint32_t location);

	virtual bool Init(const char* szShader);
	virtual bool UnInit();
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, const KDynamicConstantBufferUsage* usage);
	virtual bool ExecuteIndirect(IKCommandBufferPtr primaryBuffer, IKStorageBufferPtr indirectBuffer, const KDynamicConstantBufferUsage* usage);
	virtual bool Reload();
};