#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KBase/Publish/KReferenceHolder.h"

struct KPipelineState
{
	enum { R,G,B,A };
	PrimitiveTopology topology;
	bool colorWrites[4];
	BlendFactor blendSrcFactor;
	BlendFactor blendDstFactor;
	BlendOperator blendOp;
	bool blend;
	CullMode cullMode;
	FrontFace frontFace;
	PolygonMode polygonMode;
	CompareFunc depthComp;
	bool depthWrite;
	bool depthTest;
	bool depthBias;
	CompareFunc stencilComp;
	StencilOperator stencilFailOp;
	StencilOperator stencilDepthFailOp;
	StencilOperator stencilPassOp;
	uint32_t stencilRef;
	bool stencil;

	KPipelineState()
	{
		topology = PT_TRIANGLE_LIST;
		colorWrites[0] = colorWrites[1] = colorWrites[2] = colorWrites[3] = true;
		blendSrcFactor = BF_SRC_ALPHA;
		blendDstFactor = BF_ONE_MINUS_SRC_ALPHA;
		blendOp = BO_ADD;
		blend = false;
		cullMode = CM_BACK;
		frontFace = FF_COUNTER_CLOCKWISE;
		polygonMode = PM_FILL;
		depthComp = CF_LESS_OR_EQUAL;
		depthWrite = true;
		depthTest = true;
		depthBias = false;
		stencilComp = CF_LESS_OR_EQUAL;
		stencilFailOp = stencilDepthFailOp = stencilPassOp = SO_KEEP;
		stencilRef = 0;
		stencil = false;
	}
};

struct KPipelineBinding
{
	IKShaderPtr shaders[LAYOUT_SHADER_COUNT];
	std::vector<VertexFormat> formats;

	KPipelineBinding()
	{}
};

struct IKPipeline
{
	virtual ~IKPipeline() {}

	// State
	virtual bool SetPrimitiveTopology(PrimitiveTopology topology) = 0;
	virtual bool SetVertexBinding(const VertexFormat* format, size_t count) = 0;

	virtual bool SetColorWrite(bool r, bool g, bool b, bool a) = 0;
	virtual bool SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op) = 0;
	virtual bool SetBlendEnable(bool enable) = 0;

	virtual bool SetCullMode(CullMode cullMode) = 0;
	virtual bool SetFrontFace(FrontFace frontFace) = 0;
	virtual bool SetPolygonMode(PolygonMode polygonMode) = 0;

	virtual bool SetDepthFunc(CompareFunc func, bool depthWrtie, bool depthTest) = 0;
	virtual bool SetDepthBiasEnable(bool enable) = 0;

	virtual bool SetStencilFunc(CompareFunc func, StencilOperator failOp, StencilOperator depthFailOp, StencilOperator passOp) = 0;
	virtual bool SetStencilRef(uint32_t ref) = 0;
	virtual bool SetStencilEnable(bool enable) = 0;

	// Binding
	virtual bool SetShader(ShaderType shaderType, IKShaderPtr shader) = 0;
	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer) = 0;

	virtual bool SetSampler(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, bool dynimicWrite = false) = 0;
	virtual bool SetStorageImage(unsigned int location, IKFrameBufferPtr image, ElementFormat format) = 0;

	virtual bool SetStorageBuffer(unsigned int location, ShaderTypes shaderTypes, IKStorageBufferPtr buffer) = 0;

	virtual bool SetSamplers(unsigned int location, const std::vector<IKFrameBufferPtr>& images, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite = false) = 0;
	virtual bool SetSamplerMipmap(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, uint32_t startMip, uint32_t mipNum, bool dynimicWrite = false) = 0;
	virtual bool SetStorageImages(unsigned int location, const std::vector<IKFrameBufferPtr>& images, ElementFormat format) = 0;

	virtual bool CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size) = 0;
	virtual bool DestroyConstantBlock() = 0;

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
	virtual bool Reload() = 0;

	virtual bool SetDebugName(const char* name) = 0;
	virtual const char* GetDebugName() const = 0;

	virtual bool GetHandle(IKRenderPassPtr renderPass, IKPipelineHandlePtr& handle) = 0;
};

struct IKPipelineLayout
{
	virtual ~IKPipelineLayout() {}
	virtual bool Init(const KPipelineBinding& binding) = 0;
	virtual bool UnInit() = 0;
};

typedef KReferenceHolder<IKPipelineLayoutPtr> KPipelineLayoutRef;

struct IKPipelineHandle
{
	virtual ~IKPipelineHandle() {}
	virtual bool Init(IKPipelineLayout* layout, IKRenderPass* renderPass, const KPipelineState& state, const KPipelineBinding& binding) = 0;
	virtual bool UnInit() = 0;
	virtual bool SetDebugName(const char* name) = 0;
};

typedef KReferenceHolder<IKPipelineHandlePtr> KPipelineHandleRef;