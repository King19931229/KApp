#pragma once
#include "Interface/IKPipeline.h"
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
public:
	KVulkanPipelineHandle();
	virtual~KVulkanPipelineHandle();

	virtual bool Init(IKPipeline* pipeline, IKRenderPass* renderPass);
	virtual bool UnInit();

	inline VkPipeline GetVkPipeline() { return m_GraphicsPipeline; }
};

class KVulkanPipeline : public IKPipeline
{
	friend class KVulkanPipelineHandle;
	friend class KVulkanDescriptorPool;
public:
	enum
	{
		VERTEX,
		FRAGMENT,
		GEOMETRY,
		TASK,
		MESH,
		COUNT
	};
protected:
	// 顶点装配信息
	std::vector<VkVertexInputBindingDescription> m_BindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
	VkPrimitiveTopology	m_PrimitiveTopology;

	// Alpha混合信息
	VkColorComponentFlags m_ColorWriteMask;
	VkBlendFactor m_ColorSrcBlendFactor;
	VkBlendFactor m_ColorDstBlendFactor;
	VkBlendOp m_ColorBlendOp;
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

	// Constant Buffer信息
	struct UniformBufferBindingInfo
	{
		ShaderTypes shaderTypes;
		IKUniformBufferPtr buffer;
	};

	struct PushConstantBindingInfo
	{
		ShaderTypes shaderTypes;
		uint32_t size;
	};

	// Sampler 信息
	struct SamplerBindingInfo
	{
		std::vector<IKFrameBufferPtr> images;
		std::vector<IKSamplerPtr> samplers;
		std::vector<std::tuple<uint32_t, uint32_t>> mipmaps;

		SamplerBindingInfo()
		{
			images = {};
			samplers = {};
			mipmaps = {};
		}
	};

	// Storage Image信息
	struct StorageImageBindingInfo
	{
		std::vector<IKFrameBufferPtr> images;
		ElementFormat format;

		StorageImageBindingInfo()
		{
			format = EF_UNKNOWN;
		}
	};

	// Storage Buffer信息
	struct StorageBufferBindingInfo
	{
		ShaderTypes shaderTypes;
		IKStorageBufferPtr buffer;

		StorageBufferBindingInfo()
		{
			shaderTypes = 0;
			buffer = nullptr;
		}
	};

	enum BindingType
	{
		BINDING_UNIFORM,
		BINDING_SAMPLER,
		BINDING_STORAGE_IMAGE,
		BINDING_STORAGE_BUFFER
	};

	std::unordered_map<unsigned int, BindingType> m_BindingType;

	PushConstantBindingInfo m_PushContant;
	std::unordered_map<unsigned int, UniformBufferBindingInfo> m_Uniforms;
	std::unordered_map<unsigned int, SamplerBindingInfo> m_Samplers;
	std::unordered_map<unsigned int, StorageImageBindingInfo> m_StorageImages;
	std::unordered_map<unsigned int, StorageBufferBindingInfo> m_StorageBuffers;

	IKShaderPtr m_Shaders[COUNT];

	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBinding;
	std::vector<VkWriteDescriptorSet> m_WriteDescriptorSet;
	std::vector<VkDescriptorBufferInfo> m_BufferWriteInfo;

	// 设备句柄
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkPipelineLayout m_PipelineLayout;

	static constexpr uint32_t MAX_THREAD_SUPPORT = 512;
	std::array<KVulkanDescriptorPool, MAX_THREAD_SUPPORT> m_Pools;
	std::array<bool, MAX_THREAD_SUPPORT> m_PoolInitializeds;
#ifdef _DEBUG
	std::array<std::atomic_bool, MAX_THREAD_SUPPORT> m_PoolInitialing;
#endif
	std::string m_Name;

	bool CreateLayout();
	bool CreateDestcriptionWrite();
	bool CreateDestcriptionPool(uint32_t threadIndex);
	bool DestroyDevice();
	bool ClearHandle();

	bool CheckBindConflict(unsigned int location, BindingType type);
	bool BindSampler(unsigned int location, const SamplerBindingInfo& info);

	bool CheckDependencyResource();

	typedef std::unordered_map<IKRenderPass*, IKPipelineHandlePtr> PipelineHandleMap;
	PipelineHandleMap m_HandleMap;

	RenderPassInvalidCallback m_RenderPassInvalidCB;
	bool InvaildHandle(IKRenderPass* target);
public:
	KVulkanPipeline();
	virtual ~KVulkanPipeline();

	virtual bool SetPrimitiveTopology(PrimitiveTopology topology);
	virtual bool SetVertexBinding(const VertexFormat* formats, size_t count);

	virtual bool SetColorWrite(bool r, bool g, bool b, bool a);
	virtual bool SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op);
	virtual bool SetBlendEnable(bool enable);

	virtual bool SetCullMode(CullMode cullMode);
	virtual bool SetFrontFace(FrontFace frontFace);
	virtual bool SetPolygonMode(PolygonMode polygonMode);

	virtual bool SetDepthFunc(CompareFunc func, bool depthWrtie, bool depthTest);
	virtual bool SetDepthBiasEnable(bool enable);

	virtual bool SetStencilFunc(CompareFunc func, StencilOperator failOp, StencilOperator depthFailOp, StencilOperator passOp);
	virtual bool SetStencilRef(uint32_t ref);
	virtual bool SetStencilEnable(bool enable);

	virtual bool SetShader(ShaderType shaderType, IKShaderPtr shader);

	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer);

	virtual bool SetSampler(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, bool dynimicWrite);
	virtual bool SetSamplerMipmap(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, uint32_t startMip, uint32_t mipNum, bool dynimicWrite);
	virtual bool SetStorageImage(unsigned int location, IKFrameBufferPtr image, ElementFormat format);

	virtual bool SetStorageBuffer(unsigned int location, ShaderTypes shaderTypes, IKStorageBufferPtr buffer);

	virtual bool SetSamplers(unsigned int location, const std::vector<IKFrameBufferPtr>& images, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite);
	virtual bool SetStorageImages(unsigned int location, const std::vector<IKFrameBufferPtr>& images, ElementFormat format);

	virtual bool CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size);
	virtual bool DestroyConstantBlock();

	virtual bool Init();
	virtual bool UnInit();
	virtual bool Reload();

	virtual bool SetDebugName(const char* name);
	virtual const char* GetDebugName() const;

	virtual bool GetHandle(IKRenderPassPtr renderPass, IKPipelineHandlePtr& handle);

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }
	VkDescriptorSet AllocDescriptorSet(uint32_t threadIndex,
		const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
		const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount);
};