#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphBuilder.h"

class KFrameGraphPass
{
protected:
	std::vector<KFrameGraphHandlePtr> m_ReadResources;
	std::vector<KFrameGraphHandlePtr> m_WriteResources;
public:
	KFrameGraphPass();
	~KFrameGraphPass();

	virtual bool Compile(KFrameGraphBuilder& builder) = 0;
	virtual bool Execute(KFrameGraphBuilder& builder) = 0;
};