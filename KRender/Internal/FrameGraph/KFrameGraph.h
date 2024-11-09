#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphResource.h"
#include "KFrameGraphPass.h"
#include "KFrameGraphBuilder.h"
#include "KFrameGraphExecutor.h"

#include <unordered_set>

class KFrameGraph
{
	friend class KFrameGraphBuilder;
protected:
	KFrameGraphHandlePool m_HandlePool;
	ResourceMap m_Resources;
	KFrameGraphRenderPassMap m_RenderPassMap;
	IKRenderDevice* m_Device;
	std::unordered_set<KFrameGraphPass*> m_Passes;

	KFrameGraphResourcePtr GetResource(const KFrameGraphID& handle);
public:
	KFrameGraph();
	~KFrameGraph();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	struct RenderTargetCreateParameter
	{
		uint32_t width;
		uint32_t height;
		uint32_t msaaCount;
		ElementFormat format;
		bool bDepth;
		bool bStencil;

		RenderTargetCreateParameter()
		{
			width = 0;
			height = 0;
			msaaCount = 1;
			format = EF_UNKNOWN;
			bDepth = false;
			bStencil = false;
		}
	};

	KFrameGraphID CreateTexture(IKTexturePtr texture);
	KFrameGraphID CreateRenderTarget(const RenderTargetCreateParameter& parameter);
	bool Destroy(const KFrameGraphID& handle);

	const IKRenderTargetPtr GetTarget(const KFrameGraphID& handle);

	bool RegisterPass(KFrameGraphPass* pass);
	bool UnRegisterPass(KFrameGraphPass* pass);

	bool Resize();
	bool Compile();
	bool Execute(KRHICommandList& commandList, uint32_t chainIndex);
	bool Alloc();
	bool Release();
};