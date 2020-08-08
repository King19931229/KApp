#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphBuilder.h"

class KFrameGraphPass
{
	friend class KFrameGraphBuilder;
protected:
	std::vector<KFrameGraphHandlePtr> m_ReadResources;
	std::vector<KFrameGraphHandlePtr> m_WriteResources;
	unsigned int m_Ref;
public:
	KFrameGraphPass();
	~KFrameGraphPass();

	bool ReadImpl(KFrameGraphBuilder& builder, KFrameGraphHandlePtr handle);
	bool WriteImpl(KFrameGraphBuilder& builder, KFrameGraphHandlePtr handle);

	bool Clear();
	inline unsigned int GetRef() const { return m_Ref; }

	virtual bool Compile(KFrameGraphBuilder& builder) = 0;
	virtual bool Execute(KFrameGraphBuilder& builder) = 0;
};