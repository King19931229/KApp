#pragma once
#include "KFrameGraphHandle.h"
#include "KFrameGraphBuilder.h"
#include "KFrameGraphExecutor.h"

class KFrameGraphPass
{
	friend class KFrameGraph;
private:
	std::vector<KFrameGraphID> m_ReadResources;
	std::vector<KFrameGraphID> m_WriteResources;
	std::string m_Name;
	unsigned int m_Ref;
	unsigned int m_ExecutedDenpencies;
	bool m_Executed;
public:
	KFrameGraphPass(const std::string& name);
	~KFrameGraphPass();

	bool ReadImpl(KFrameGraphBuilder& builder, KFrameGraphID handle);
	bool WriteImpl(KFrameGraphBuilder& builder, KFrameGraphID handle);
	bool Clear();

	inline unsigned int GetRef() const { return m_Ref; }
	inline const std::string& GetName() const { return m_Name; }

	virtual bool HasSideEffect() const { return false; }

	virtual bool Setup(KFrameGraphBuilder& builder) = 0;
	virtual bool Resize(KFrameGraphBuilder& builder) { return true; }
	virtual bool Execute(KFrameGraphExecutor& executor) = 0;
};