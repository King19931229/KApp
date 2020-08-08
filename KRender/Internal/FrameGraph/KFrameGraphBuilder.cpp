#include "KFrameGraphBuilder.h"
#include "KFrameGraphResource.h"
#include "KFrameGraphPass.h"

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
	if (resource)
	{
		if (!resource->IsImported())
		{
			if (resource->IsVaild())
			{
				Release(resource);
			}
			resource->Alloc(*this);
			return true;
		}
	}

	return false;
}

bool KFrameGraphBuilder::Release(KFrameGraphResource* resource)
{
	if (resource)
	{
		resource->Release(*this);
		return false;
	}	
	return false;
}

bool KFrameGraphBuilder::Read(KFrameGraphPass* pass, KFrameGraphHandlePtr handle)
{
	if (pass && handle)
	{
		pass->ReadImpl(*this, handle);
		return true;
	}
	return false;
}

bool KFrameGraphBuilder::Write(KFrameGraphPass* pass, KFrameGraphHandlePtr handle)
{
	if (pass && handle)
	{
		pass->WriteImpl(*this, handle);
		return true;
	}
	return false;
}