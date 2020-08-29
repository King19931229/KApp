#include "KFrameGraphPass.h"

KFrameGraphPass::KFrameGraphPass(const std::string& name)
	: m_Name(name),
	m_Ref(0),
	m_ExecutedDenpencies(0),
	m_PriamryCommandBuffer(nullptr),
	m_CurrentFrameIndex(0),
	m_Executed(false)
{
}

KFrameGraphPass::~KFrameGraphPass()
{
}

bool KFrameGraphPass::ReadImpl(KFrameGraphBuilder& builder, KFrameGraphID handle)
{
	auto it = std::find(m_ReadResources.begin(), m_ReadResources.end(), handle);
	if (it == m_ReadResources.end())
	{
		m_ReadResources.push_back(handle);
	}
	return true;
}

bool KFrameGraphPass::WriteImpl(KFrameGraphBuilder& builder, KFrameGraphID handle)
{
	auto it = std::find(m_WriteResources.begin(), m_WriteResources.end(), handle);
	// TODO
	if (it == m_WriteResources.end())
	{
		m_WriteResources.push_back(handle);
	}
	return true;
}

bool KFrameGraphPass::Clear()
{
	m_ReadResources.clear();
	m_WriteResources.clear();
	m_Ref = 0;
	m_ExecutedDenpencies = 0;
	m_Executed = false;
	return true;
}