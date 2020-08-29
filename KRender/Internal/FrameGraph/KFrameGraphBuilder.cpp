#include "KFrameGraphBuilder.h"
#include "KFrameGraphResource.h"
#include "KFrameGraphPass.h"
#include "KFrameGraph.h"

KFrameGraphBuilder::KFrameGraphBuilder(KFrameGraphPass* pass, KFrameGraph* master)
	: m_Pass(pass),
	m_MasterGraph(master)
{
}

KFrameGraphBuilder::~KFrameGraphBuilder()
{
}

bool KFrameGraphBuilder::Read(const KFrameGraphID& handle)
{
	if (m_Pass && m_MasterGraph && handle.IsVaild())
	{
		KFrameGraphResourcePtr resource = m_MasterGraph->GetResource(handle);
		if (resource)
		{
			m_Pass->ReadImpl(*this, handle);
			resource->AddReaderImpl(m_Pass);
			return true;
		}
	}
	return false;
}

bool KFrameGraphBuilder::Write(const KFrameGraphID& handle)
{
	if (m_Pass && m_MasterGraph && handle.IsVaild())
	{
		KFrameGraphResourcePtr resource = m_MasterGraph->GetResource(handle);
		if (resource)
		{
			// TODO 暂时不支持多次写入一个Resource
			m_Pass->WriteImpl(*this, handle);
			resource->SetWriterImpl(m_Pass);
			return true;
		}
	}
	return false;
}