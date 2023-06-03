#pragma once
#include "Interface/IKPipeline.h"
#include "Interface/IKShader.h"
#include "Interface/IKRenderPass.h"
#include <unordered_map>

class KPipelineBase : public IKPipeline
{
protected:
	KPipelineState m_State;
	KPipelineBinding m_Binding;
	KPipelineLayoutRef m_Layout;

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

	PushConstantBindingInfo m_PushContant;
	std::unordered_map<unsigned int, UniformBufferBindingInfo> m_Uniforms;
	std::unordered_map<unsigned int, SamplerBindingInfo> m_Samplers;
	std::unordered_map<unsigned int, StorageImageBindingInfo> m_StorageImages;
	std::unordered_map<unsigned int, StorageBufferBindingInfo> m_StorageBuffers;
	std::unordered_map<unsigned int, BindingType> m_BindingType;

	bool CheckBindConflict(unsigned int location, BindingType type);
	bool BindSampler(unsigned int location, const SamplerBindingInfo& info);

	struct PipelineHandle
	{
		size_t hash = 0;
		KPipelineHandleRef handle;
	};

	typedef std::unordered_map<IKRenderPass*, PipelineHandle> PipelineHandleMap;
	PipelineHandleMap m_HandleMap;

	RenderPassInvalidCallback m_RenderPassInvalidCB;
	bool InvaildHandle(IKRenderPass* target);

	ShaderInvalidCallback m_ShaderInvalidCB;
	bool InvaildHandle(IKShader* shader);

	std::string m_Name;

	virtual bool DestroyDevice();
	virtual bool CreateLayout();
public:
	KPipelineBase();
	~KPipelineBase();

	virtual bool DebugDump();

	virtual bool SetPrimitiveTopology(PrimitiveTopology topology);
	virtual bool SetVertexBinding(const VertexFormat* format, size_t count);

	virtual bool SetColorWrite(bool r, bool g, bool b, bool a);
	virtual bool SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op);
	virtual bool SetBlendEnable(bool enable);

	virtual bool SetCullMode(CullMode cullMode);
	virtual bool SetFrontFace(FrontFace frontFace);
	virtual bool SetPolygonMode(PolygonMode polygonMode);

	virtual bool SetDepthFunc(CompareFunc func, bool depthWrite, bool depthTest);
	virtual bool SetDepthBiasEnable(bool enable);

	virtual bool SetStencilFunc(CompareFunc func, StencilOperator failOp, StencilOperator depthFailOp, StencilOperator passOp);
	virtual bool SetStencilRef(uint32_t ref);
	virtual bool SetStencilEnable(bool enable);

	// Binding
	virtual bool SetShader(ShaderType shaderType, IKShaderPtr shader);
	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer);

	virtual bool SetSampler(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, bool dynimicWrite = false);
	virtual bool SetStorageImage(unsigned int location, IKFrameBufferPtr image, ElementFormat format);

	virtual bool SetStorageBuffer(unsigned int location, ShaderTypes shaderTypes, IKStorageBufferPtr buffer);

	virtual bool SetSamplers(unsigned int location, const std::vector<IKFrameBufferPtr>& images, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite = false);
	virtual bool SetSamplerMipmap(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, uint32_t startMip, uint32_t mipNum, bool dynimicWrite = false);
	virtual bool SetStorageImages(unsigned int location, const std::vector<IKFrameBufferPtr>& images, ElementFormat format);

	virtual bool CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size);
	virtual bool DestroyConstantBlock();

	virtual bool Init();
	virtual bool UnInit();
	virtual bool Reload();

	virtual bool GetHandle(IKRenderPassPtr renderPass, IKPipelineHandlePtr& handle);

	virtual bool SetDebugName(const char* name);
	virtual const char* GetDebugName() const;
};