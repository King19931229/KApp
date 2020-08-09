#pragma once
#include "Interface/IKRenderDevice.h"
#include "KFrameGraphHandle.h"

class KFrameGraphResource;
class KFrameGraphPass;
class KFrameGraph;

class KFrameGraphBuilder
{
protected:
	KFrameGraphPass* m_Pass;
	KFrameGraph* m_MasterGraph;
public:
	KFrameGraphBuilder(KFrameGraphPass* pass, KFrameGraph* master);
	~KFrameGraphBuilder();

	bool Read(KFrameGraphHandlePtr handle);
	bool Write(KFrameGraphHandlePtr handle);
};