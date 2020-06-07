#pragma once
#include "Interface/IKPipeline.h"
#include "Internal/KRenderGlobal.h"
#include "KVulkanConfig.h"
#include "KVulkanDescripiorPool.h"
#include <map>

class KVulkanRenderTarget;
class KVulkanPipeline;

class KVulkanPipelineHandle : public IKPipelineHandle
{
protected:
	VkPipeline m_GraphicsPipeline;
public:
	KVulkanPipelineHandle();
	virtual~KVulkanPipelineHandle();

	virtual bool Init(IKPipeline* pipeline, IKRenderTarget* target);
	virtual bool UnInit();

	inline VkPipeline GetVkPipeline() { return m_GraphicsPipeline; }
};

class KVulkanPipeline : public IKPipeline
{
	friend class KVulkanPipelineHandle;
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

	// 深度偏移
	/*
	float m_DepthBiasConstantFactor;
	float m_DepthBiasClamp;
	float m_DepthBiasSlopeFactor;
	*/
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
	std::map<unsigned int, UniformBufferBindingInfo> m_Uniforms;

	struct PushConstantBindingInfo
	{
		ShaderTypes shaderTypes;
		uint32_t size;
	};
	PushConstantBindingInfo m_PushContant;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	// Sampler 信息
	struct SamplerBindingInfo
	{
		VkImageView vkImageView;
		VkSampler vkSampler;

		IKTexturePtr texture;
		IKSamplerPtr sampler;

		bool nakeInfo;
		bool depthStencil;

		SamplerBindingInfo()
		{
			vkImageView = VK_NULL_HANDLE;
			vkSampler = VK_NULL_HANDLE;
			texture = nullptr;
			sampler = nullptr;
			nakeInfo = false;
			depthStencil = false;
		}
	};
	std::map<unsigned int, SamplerBindingInfo> m_Samplers;

	uint32_t				m_UniformBufferDescriptorCount;
	uint32_t				m_SamplerDescriptorCount;

	std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBinding;
	std::vector<VkWriteDescriptorSet> m_WriteDescriptorSet;
	std::vector<VkDescriptorImageInfo> m_ImageWriteInfo;
	std::vector<VkDescriptorBufferInfo> m_BufferWriteInfo;

	// 设备句柄
	VkDescriptorSetLayout	m_DescriptorSetLayout;
	VkPipelineLayout		m_PipelineLayout;

	KVulkanDescriptorPool m_Pool;

	bool CreateLayout();
	bool CreateDestcriptionPool();
	bool DestroyDevice();
	bool ClearHandle();
	bool BindSampler(unsigned int location, const SamplerBindingInfo& info);
	bool CheckDependencyResource();

	typedef std::unordered_map<IKRenderTargetPtr, IKPipelineHandlePtr> RtPipelineHandleMap;
	RtPipelineHandleMap m_HandleMap;
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
	//virtual bool SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
	virtual bool SetDepthBiasEnable(bool enable);

	virtual bool SetStencilFunc(CompareFunc func, StencilOperator failOp, StencilOperator depthFailOp, StencilOperator passOp);
	virtual bool SetStencilRef(uint32_t ref);
	virtual bool SetStencilEnable(bool enable);

	virtual bool SetShader(ShaderType shaderType, IKShaderPtr shader);

	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer);
	virtual bool SetSampler(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler);
	virtual bool SetSamplerDepthAttachment(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler);
	virtual bool CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size);
	virtual bool DestroyConstantBlock();

	virtual bool Init();
	virtual bool UnInit();
	virtual bool Reload();

	virtual bool GetHandle(IKRenderTargetPtr target, IKPipelineHandlePtr& handle);
	virtual bool InvaildHandle(IKRenderTargetPtr target);

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }
	VkDescriptorSet AllocDescriptorSet();
};