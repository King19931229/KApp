#pragma once
#include "Interface/IKRayTracePipline.h"
#include "Interface/IKAccelerationStructure.h"
#include "KBase/Publish/KHandleRetriever.h"
#include "KVulkanConfig.h"
#include "KVulkanDescripiorPool.h"
#include "KVulkanInitializer.h"
#include <unordered_map>

class KVulkanRayTracePipeline : public IKRayTracePipline
{
protected:
	IKAccelerationStructurePtr m_TopDown;
	KHandleRetriever<uint32_t> m_Handles;
	std::unordered_map<uint32_t, IKAccelerationStructure::BottomASTransformTuple> m_BottomASMap;
	KVulkanDescriptorPool m_Pool;

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
	void CreateShaderBindingTables();
	void DestroyShaderBindingTables();
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

	virtual bool Init();
	virtual bool UnInit();
};