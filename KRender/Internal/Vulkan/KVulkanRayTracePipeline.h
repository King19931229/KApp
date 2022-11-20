#pragma once
#include "Interface/IKRayTracePipeline.h"
#include "Interface/IKAccelerationStructure.h"
#include "KVulkanConfig.h"
#include "KVulkanInitializer.h"
#include <unordered_map>

class KVulkanRayTracePipeline : public IKRayTracePipeline
{
protected:
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups;
	IKUniformBufferPtr m_CameraBuffer;
	IKCommandPoolPtr m_CommandPool;
	IKCommandBufferPtr m_CommandBuffer;
	IKAccelerationStructurePtr m_TopDownAS;

	struct Scene
	{
		VkBuffer buffer;
		KVulkanHeapAllocator::AllocInfo allocInfo;

		Scene()
		{
			buffer = VK_NULL_HANDEL;
		}
	};
	Scene m_Scene;

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
			ZERO_MEMORY(stridedDeviceAddressRegion);
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

	IKRenderTargetPtr m_StorageRT;

	struct ShaderInfo
	{
		KShaderRef shader;
		std::string path;
	};

	ShaderInfo m_AnyHitShader;
	ShaderInfo m_ClosestHitShader;
	ShaderInfo m_RayGenShader;
	ShaderInfo m_MissShader;

	ElementFormat m_Format;
	uint32_t m_Width;
	uint32_t m_Height;

	bool m_Inited;
	bool m_ASUpdated;

	void CreateStorageImage();
	void DestroyStorageImage();
	void CreateStrogeScene();
	void DestroyStrogeScene();
	void CreateDescriptorSet();
	void DestroyDescriptorSet();
	void CreateShader();
	void DestroyShader();
	void CreateShaderBindingTables();
	void DestroyShaderBindingTables();
	void CreateCommandBuffers();
	void DestroyCommandBuffers();

	void CreateShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount);
	void DestroyShaderBindingTable(ShaderBindingTable& shaderBindingTable);
	void UpdateCameraDescriptor();
public:
	KVulkanRayTracePipeline();
	~KVulkanRayTracePipeline();

	virtual bool SetShaderTable(ShaderType type, const char* szShader);
	virtual bool SetStorageImage(ElementFormat format);

	virtual bool RecreateFromAS();
	virtual bool ResizeImage(uint32_t width, uint32_t height);
	virtual bool ReloadShader();

	virtual IKRenderTargetPtr GetStorageTarget();

	virtual bool Init(IKUniformBufferPtr cameraBuffer, IKAccelerationStructurePtr topDownAS, uint32_t width, uint32_t height);
	virtual bool UnInit();
	virtual bool MarkASUpdated();

	virtual bool Execute(IKCommandBufferPtr primaryBuffer);
};