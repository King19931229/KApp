#pragma once
#include "Interface/IKPipeline.h"
#include "KVulkanConfig.h"
#include <map>

class KVulkanRenderTarget;
class KVulkanPipeline;

class KVulkanPipelineHandle : public IKPipelineHandle
{
protected:
	KVulkanPipeline* m_Pipeline;
	KVulkanRenderTarget* m_Target;

	VkPipeline m_GraphicsPipeline;
public:
	KVulkanPipelineHandle();
	~KVulkanPipelineHandle();

	bool Init(IKPipeline* pipeline, IKRenderTarget* target);
	bool UnInit();

	inline VkPipeline GetVkPipeline() { return m_GraphicsPipeline; }
};


class KVulkanPipeline : public IKPipeline
{
	friend class KVulkanPipelineHandle;
protected:
	// ����װ����Ϣ
	std::vector<VkVertexInputBindingDescription> m_BindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
	VkPrimitiveTopology	m_PrimitiveTopology;

	// Alpha�����Ϣ
	VkBlendFactor m_ColorSrcBlendFactor;
	VkBlendFactor m_ColorDstBlendFactor;
	VkBlendOp m_ColorBlendOp;
	VkBool32 m_BlendEnable;

	// ��դ����Ϣ
	VkPolygonMode m_PolygonMode;
	VkCullModeFlagBits m_CullMode;
	VkFrontFace m_FrontFace;

	// �����Ϣ
	VkBool32 m_DepthWrite;
	VkBool32 m_DepthTest;
	VkCompareOp m_DepthOp;

	// ���ƫ��
	/*
	float m_DepthBiasConstantFactor;
	float m_DepthBiasClamp;
	float m_DepthBiasSlopeFactor;
	*/
	VkBool32 m_DepthBiasEnable;

	// Constant Buffer��Ϣ
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

	// Sampler ��Ϣ
	struct SamplerBindingInfo
	{
		VkImageView vkImageView;
		VkSampler vkSampler;
		bool depthStencil;
	};
	std::map<unsigned int, SamplerBindingInfo> m_Samplers;

	// �豸���
	VkDescriptorSetLayout	m_DescriptorSetLayout;
	VkDescriptorPool		m_DescriptorPool;
	VkDescriptorSet			m_DescriptorSet;
	VkPipelineLayout		m_PipelineLayout;

	bool CreateLayout();
	bool CreateDestcription();
public:
	KVulkanPipeline();
	~KVulkanPipeline();

	virtual bool SetPrimitiveTopology(PrimitiveTopology topology);
	virtual bool SetVertexBinding(const VertexFormat* formats, size_t count);

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
	virtual bool SetSampler(unsigned int location, const ImageView& view, IKSamplerPtr sampler);
	virtual bool PushConstantBlock(ShaderTypes shaderTypes, uint32_t size, uint32_t& offset);

	virtual bool Init();
	virtual bool UnInit();

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }
	inline VkDescriptorSet GetVkDescriptorSet() { return m_DescriptorSet; }
};