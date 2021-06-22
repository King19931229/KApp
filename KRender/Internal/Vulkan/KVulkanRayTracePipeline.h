#pragma once
#include "Interface/IKRayTracePipeline.h"
#include "Interface/IKAccelerationStructure.h"
#include "KBase/Publish/KHandleRetriever.h"
#include "KVulkanConfig.h"
#include "KVulkanInitializer.h"
#include <unordered_map>

class KVulkanRayTracePipeline : public IKRayTracePipeline
{
protected:
	IKAccelerationStructurePtr m_TopDown;
	KHandleRetriever<uint32_t> m_Handles;
	std::unordered_map<uint32_t, IKAccelerationStructure::BottomASTransformTuple> m_BottomASMap;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups;

	std::vector<IKUniformBufferPtr> m_CameraBuffers;

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
		VkPipeline pipline;

		Pipeline()
		{
			layout = VK_NULL_HANDEL;
			pipline = VK_NULL_HANDEL;
		}
	};
	Pipeline m_Pipeline;

	struct ShaderBindingTable
	{
		VkBuffer buffer;
		KVulkanHeapAllocator::AllocInfo allocInfo;
		void* mapped;
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion;

		ShaderBindingTable()
		{
			buffer = VK_NULL_HANDEL;
			mapped = nullptr;
		}
	};
	struct ShaderBindingTables
	{
		ShaderBindingTable raygen;
		ShaderBindingTable miss;
		ShaderBindingTable hit;
		ShaderBindingTable callable;
	};
	ShaderBindingTables m_ShaderBindingTables;

	IKRenderTargetPtr m_StorgeRT;

	IKShaderPtr m_AnyHitShader;
	IKShaderPtr m_ClosestHitShader;
	IKShaderPtr m_RayGenShader;
	IKShaderPtr m_MissShader;

	ElementFormat m_Format;
	uint32_t m_Width;
	uint32_t m_Height;

	bool m_Inited;

	void CreateAccelerationStructure();
	void DestroyAccelerationStructure();
	void CreateStorgeImage();
	void DestroyStorgeImage();
	void CreateDescriptorSet();
	void DestroyDescriptorSet();
	void CreateShaderBindingTables();
	void DestroyShaderBindingTables();

	void CreateShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount);
	void DestroyShaderBindingTable(ShaderBindingTable& shaderBindingTable);
public:
	KVulkanRayTracePipeline();
	~KVulkanRayTracePipeline();

	virtual bool SetShaderTable(ShaderType type, IKShaderPtr shader);
	virtual bool SetStorgeImage(ElementFormat format, uint32_t width, uint32_t height);

	virtual uint32_t AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform);
	virtual bool RemoveBottomLevelAS(uint32_t handle);
	virtual bool ClearBottomLevelAS();

	virtual bool RecreateAS();
	virtual bool ResizeImage(uint32_t width, uint32_t height);

	virtual bool Init(const std::vector<IKUniformBufferPtr>& cameraBuffers);
	virtual bool UnInit();
};