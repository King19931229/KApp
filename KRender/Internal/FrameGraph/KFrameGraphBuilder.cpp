#include "KFrameGraphBuilder.h"
#include "KFrameGraphResource.h"

KFrameGraphBuilder::KFrameGraphBuilder()
	: m_Device(nullptr)
{
}

KFrameGraphBuilder::~KFrameGraphBuilder()
{
	ASSERT_RESULT(!m_Device);
}

bool KFrameGraphBuilder::Init(IKRenderDevice* device)
{
	m_Device = device;
	return true;
}

bool KFrameGraphBuilder::UnInit()
{
	m_Device = nullptr;
	return true;
}

bool KFrameGraphBuilder::Alloc(KFrameGraphResource* resource)
{
	if (m_Device && resource)
	{
		if (resource->IsImported())
		{
			return false;
		}

		if (resource->IsVaild())
		{
			Release(resource);
		}

		if (resource->GetType() == FrameGraphResourceType::RENDER_TARGET)
		{
			KFrameGraphRenderTarget* target = static_cast<KFrameGraphRenderTarget*>(resource);
		}
	}

	return false;
}

bool KFrameGraphBuilder::Release(KFrameGraphResource* resource)
{
	if (m_Device && resource)
	{

	}
	
	return false;
}