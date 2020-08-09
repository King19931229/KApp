#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphBuilder.h"

class KFrameGraphPass
{
	friend class KFrameGraph;
private:
	std::vector<KFrameGraphHandlePtr> m_ReadResources;
	std::vector<KFrameGraphHandlePtr> m_WriteResources;
	std::string m_Name;
	unsigned int m_Ref;
	unsigned int m_ExecutedDenpencies;
	bool m_Executed;
public:
	KFrameGraphPass(const std::string& name);
	~KFrameGraphPass();

	bool ReadImpl(KFrameGraphBuilder& builder, KFrameGraphHandlePtr handle);
	bool WriteImpl(KFrameGraphBuilder& builder, KFrameGraphHandlePtr handle);
	bool Clear();

	inline unsigned int GetRef() const { return m_Ref; }
	inline const std::string& GetName() const { return m_Name; }

	virtual bool HasSideEffect() const { return false; }

	virtual bool Setup(KFrameGraphBuilder& builder) = 0;
	virtual bool Execute() = 0;
};