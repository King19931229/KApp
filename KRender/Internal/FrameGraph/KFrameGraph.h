#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphResource.h"
#include "KFrameGraphPass.h"
#include "KFrameGraphBuilder.h"

#include <unordered_set>

class KFrameGraph
{
	friend class KFrameGraphBuilder;
protected:
	KFrameGraphHandlePool m_HandlePool;
	ResourceMap m_Resources;
	IKRenderDevice* m_Device;
	std::unordered_set<KFrameGraphPass*> m_Passes;

	KFrameGraphResourcePtr GetResource(KFrameGraphHandlePtr handle);
public:
	KFrameGraph();
	~KFrameGraph();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	KFrameGraphHandlePtr CreateTexture(IKTexturePtr texture);
	KFrameGraphHandlePtr CreateColorRenderTarget(size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	KFrameGraphHandlePtr CreateDepthStecnilRenderTarget(size_t width, size_t height, bool bStencil);
	bool RecreateColorRenderTarget(KFrameGraphHandlePtr handle, size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool RecreateDepthStecnilRenderTarget(KFrameGraphHandlePtr handle, size_t width, size_t height, bool bStencil);
	bool Destroy(KFrameGraphHandlePtr handle);

	bool RegisterPass(KFrameGraphPass* pass);
	bool UnRegisterPass(KFrameGraphPass* pass);

	bool Compile();
	bool Execute();
};