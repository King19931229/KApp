#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/Render/KRHICommandList.h"
#include "KBase/Interface/Entity/IKEntity.h"

class KABufferDepthPeeling
{
protected:
	enum
	{
		ABUFFER_BINDING_LINK_HEADER,
		ABUFFER_BINDING_LINK_NEXT,
		ABUFFER_BINDING_LINK_DEPTH,
		ABUFFER_BINDING_LINK_RESULT,
		ABUFFER_BINDING_OBJECT,
		GROUP_SIZE = 32
	};

	KShaderRef m_QuadVS;
	KShaderRef m_BlendFS;

	IKRenderTargetPtr m_LinkHeaderTarget;
	IKStorageBufferPtr m_LinkNextBuffer;
	IKStorageBufferPtr m_LinkResultBuffer;
	IKStorageBufferPtr m_LinkDepthBuffer;
	IKComputePipelinePtr m_ResetLinkHeaderPipeline;
	IKComputePipelinePtr m_ResetLinkNextPipeline;
	IKComputePipelinePtr m_ShadingABufferPipeline;
	IKPipelinePtr m_BlendPipeline;

	uint32_t m_MaxElementCount;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
public:
	KABufferDepthPeeling();
	~KABufferDepthPeeling();

	IKRenderTargetPtr GetLinkHeaderTarget() { return m_LinkHeaderTarget; }
	IKStorageBufferPtr GetLinkNextBuffer() { return m_LinkNextBuffer; }
	IKStorageBufferPtr GetLinkResultBuffer() { return m_LinkResultBuffer; }
	IKStorageBufferPtr GetLinkDepthBuffer() { return m_LinkDepthBuffer; }

	bool Init(uint32_t width, uint32_t height, uint32_t depth);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	uint32_t GetMaxElementCount() const { return m_MaxElementCount; }

	bool Execute(KRHICommandList& commandList, IKRenderPassPtr renderPass, const std::vector<IKEntity*>& cullRes);
};