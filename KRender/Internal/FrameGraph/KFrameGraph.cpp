#include "KFrameGraph.h"

KFrameGraph::KFrameGraph()
{
}

KFrameGraph::~KFrameGraph()
{
	ASSERT_RESULT(m_Resources.empty());
}

bool KFrameGraph::Init(IKRenderDevice* device)
{
	m_Builder.Init(device);
	return true;
}

bool KFrameGraph::UnInit()
{
	m_Builder.UnInit();
	return true;
}

KFrameGraphHandlePtr KFrameGraph::CreateTexture(IKTexturePtr texture)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphTexture* textureResource = static_cast<KFrameGraphTexture*>(resource.get());
	textureResource->SetTexture(texture);

	KFrameGraphHandlePtr handle = KFrameGraphHandlePtr(KNEW KFrameGraphHandle(m_HandlePool));

	m_Resources[handle] = resource;

	return handle;
}

KFrameGraphHandlePtr KFrameGraph::CreateColorRenderTarget(size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
	targetResource->CreateAsColor(m_Builder, width, height, bDepth, bStencil, uMsaaCount);

	KFrameGraphHandlePtr handle = KFrameGraphHandlePtr(KNEW KFrameGraphHandle(m_HandlePool));

	m_Resources[handle] = resource;

	return handle;
}

KFrameGraphHandlePtr KFrameGraph::CreateDepthStecnilRenderTarget(size_t width, size_t height, bool bStencil)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
	targetResource->CreateAsDepthStencil(m_Builder, width, height, bStencil);

	KFrameGraphHandlePtr handle = KFrameGraphHandlePtr(KNEW KFrameGraphHandle(m_HandlePool));

	m_Resources[handle] = resource;

	return handle;
}

bool KFrameGraph::RecreateColorRenderTarget(KFrameGraphHandlePtr handle, size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	if (handle)
	{
		auto it = m_Resources.find(handle);
		assert(it != m_Resources.end());
		if (it != m_Resources.end())
		{
			KFrameGraphResourcePtr resource = it->second;
			KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
			targetResource->Destroy(m_Builder);
			targetResource->CreateAsColor(m_Builder, width, height, bDepth, bStencil, uMsaaCount);
			return true;
		}
	}
	return false;
}

bool KFrameGraph::RecreateDepthStecnilRenderTarget(KFrameGraphHandlePtr handle, size_t width, size_t height, bool bStencil)
{
	if (handle)
	{
		auto it = m_Resources.find(handle);
		assert(it != m_Resources.end());
		if (it != m_Resources.end())
		{
			KFrameGraphResourcePtr resource = it->second;
			KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
			targetResource->Destroy(m_Builder);
			targetResource->CreateAsDepthStencil(m_Builder, width, height, bStencil);
			return true;
		}
	}
	return false;
}

bool KFrameGraph::Destroy(KFrameGraphHandlePtr handle)
{
	if (handle)
	{
		auto it = m_Resources.find(handle);
		assert(it != m_Resources.end());
		if (it != m_Resources.end())
		{
			KFrameGraphResourcePtr resource = it->second;
			resource->Destroy(m_Builder);
			m_Resources.erase(it);
			return true;
		}
	}
	return false;
}