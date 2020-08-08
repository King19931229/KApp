#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphResource.h"
#include "KFrameGraphBuilder.h"
#include <unordered_map>

class KFrameGraph
{
protected:
	KFrameGraphHandlePool m_HandlePool;
	KFrameGraphBuilder m_Builder;
	std::unordered_map<KFrameGraphHandlePtr, KFrameGraphResourcePtr> m_Resources;
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

	bool Compile();
	bool Execute();
};