#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphResource.h"
#include <unordered_map>

class KFrameGraph
{
protected:
	typedef std::unordered_map<KFrameGraphHandlePtr, KFrameGraphResourcePtr> m_Resources;
public:

};