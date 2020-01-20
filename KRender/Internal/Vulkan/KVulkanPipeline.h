#pragma once
#include "Interface/IKPipeline.h"
#include "Internal/KRenderGlobal.h"
#include "KVulkanConfig.h"
#include <map>

class KVulkanRenderTarget;
class KVulkanPipeline;

class KVulkanPipelineHandle : public IKPipelineHandle
{
protected:
	VkPipeline m_GraphicsPipeline;

	std::mutex m_LoadTaskLock;
	KTaskUnitProcessorPtr m_LoadTask;

	PipelineHandleState m_State;

	void CancelLoadTask();
	void WaitLoadTask();
	void ReleaseHandle();
public:
	KVulkanPipelineHandle();
	virtual~KVulkanPipelineHandle();

	virtual PipelineHandleState GetState() { return m_State; }

	virtual bool Init(IKPipelinePtr pipeline, IKRenderTargetPtr target, bool async);
	virtual bool UnInit();
	virtual bool WaitDevice();

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
	VkCompareOp m_DepthOp;

	// 深度偏移
	/*
	float m_DepthBiasConstantFactor;
	float m_DepthBiasClamp;
	float m_DepthBiasSlopeFactor;
	*/
	VkBool32 m_DepthBiasEnable;

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
		uint32_t offset;
	};
	std::vector<PushConstantBindingInfo> m_PushContants;

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

	// 设备句柄
	VkDescriptorSetLayout	m_DescriptorSetLayout;
	VkDescriptorPool		m_DescriptorPool;
	VkDescriptorSet			m_DescriptorSet;
	VkPipelineLayout		m_PipelineLayout;

	bool CreateLayout();
	bool CreateDestcription();
	bool DestroyDevice();
	bool BindSampler(unsigned int location, const SamplerBindingInfo& info);
	bool WaitDependencyResource();

	std::mutex m_LoadTaskLock;
	KTaskUnitProcessorPtr m_LoadTask;
	PipelineResourceState m_State;

	void CancelLoadTask();
	void WaitLoadTask();
public:
	KVulkanPipeline();
	virtual ~KVulkanPipeline();

	virtual PipelineResourceState GetState() { return m_State; }

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

	virtual bool SetShader(ShaderTypeFlag shaderType, IKShaderPtr shader);

	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer);
	virtual bool SetSampler(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler);
	virtual bool SetSamplerDepthAttachment(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler);
	virtual bool PushConstantBlock(ShaderTypes shaderTypes, uint32_t size, uint32_t& offset);

	virtual bool Init(bool async);
	virtual bool UnInit();
	virtual bool Reload(bool async);

	virtual bool WaitDevice();

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }
	inline VkDescriptorSet GetVkDescriptorSet() { return m_DescriptorSet; }
};