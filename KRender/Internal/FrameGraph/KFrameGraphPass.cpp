#include "KFrameGraphPass.h"

KFrameGraphPass::KFrameGraphPass()
	: m_Ref(0)
{
}

KFrameGraphPass::~KFrameGraphPass()
{
}

bool KFrameGraphPass::ReadImpl(KFrameGraphBuilder& builder, KFrameGraphHandlePtr handle)
{
	if (handle)
	{
		auto it = std::find(m_ReadResources.begin(), m_ReadResources.end(), handle);
		if (it == m_ReadResources.end())
		{
			m_ReadResources.push_back(handle);
		}
		return true;
	}
	return false;
}

bool KFrameGraphPass::WriteImpl(KFrameGraphBuilder& builder, KFrameGraphHandlePtr handle)
{
	if (handle)
	{
		auto it = std::find(m_WriteResources.begin(), m_WriteResources.end(), handle);
		// TODO
		if (it == m_WriteResources.end())
		{
			m_WriteResources.push_back(handle);
		}
		return true;
	}
	return false;
}

bool KFrameGraphPass::Clear()
{
	m_ReadResources.clear();
	m_WriteResources.clear();
	m_Ref = 0;
	return true;
}