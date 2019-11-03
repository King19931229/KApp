#pragma once
#include "Interface/IKPipeline.h"
#include "KVulkanConfig.h"
#include <map>

class KVulkanRenderTarget;
class KVulkanPipeline;

class KVulkanPipelineHandle : public IKPipelineHandle
{
protected:
	KVulkanPipeline* m_Parent;
	VkPipeline m_GraphicsPipeline;
public:
	KVulkanPipelineHandle(KVulkanPipeline* parent);
	~KVulkanPipelineHandle();

	bool Init(IKRenderTarget* target);
	bool UnInit();

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

	// Shader信息
	IKProgramPtr m_Program;

	// Constant Buffer信息
	struct UniformBufferBindingInfo
	{
		ShaderTypes shaderTypes;
		IKUniformBufferPtr buffer;
	};
	std::map<unsigned int, UniformBufferBindingInfo> m_Uniforms;

	struct PushConstantBindingInfo
	{
		PushConstant constant;
		PushConstantLocation location;
	};
	std::vector<PushConstantBindingInfo> m_PushContants;

	// Sampler 信息
	struct SamplerBindingInfo
	{
		VkImageView vkImageView;
		VkSampler vkSampler;
	};
	std::map<unsigned int, SamplerBindingInfo> m_Samplers;

	// 设备句柄
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
	virtual bool SetVertexBinding(VertexInputDetail* inputDetails, unsigned int count);

	virtual bool SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op);
	virtual bool SetBlendEnable(bool enable);

	virtual bool SetCullMode(CullMode cullMode);
	virtual bool SetFrontFace(FrontFace frontFace);
	virtual bool SetPolygonMode(PolygonMode polygonMode);

	virtual bool SetDepthFunc(CompareFunc func, bool depthWrtie, bool depthTest);

	virtual bool SetShader(ShaderTypeFlag shaderType, IKShaderPtr shader);

	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer);
	virtual bool SetSampler(unsigned int location, const ImageView& view, IKSamplerPtr sampler);
	virtual bool PushConstantBlock(const PushConstant& constant, PushConstantLocation& location);

	virtual bool Init();
	virtual bool UnInit();

	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& handle);

	inline VkPipelineLayout GetVkPipelineLayout() { return m_PipelineLayout; }
	inline VkDescriptorSet GetVkDescriptorSet() { return m_DescriptorSet; }
};