#pragma once
#include "Graph/KEGraphConfig.h"

class KEGraphConnectionState
{
protected:
	PortType m_RequiredPort;
	KEGraphNodeControl* m_LastHorveredNode;
public:
	KEGraphConnectionState(PortType port = PT_NONE);
	~KEGraphConnectionState();

	void InteractWithNode(KEGraphNodeControl* node);
	void SetLastHoveredNode(KEGraphNodeControl* node);
	void ResetLastHoveredNode();

	inline void SetRequiredPort(PortType end) { m_RequiredPort = end; }
	inline PortType RequiredPort() const { return m_RequiredPort; }

	inline bool RequiresPort() const { return m_RequiredPort != PT_NONE; }
	inline void SetNoRequiredPort() { m_RequiredPort = PT_NONE; }

	inline KEGraphNodeControl* LastHoveredNode() const { return m_LastHorveredNode; }
};