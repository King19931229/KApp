#include "KEGraphConnectionState.h"
#include "Graph/Node/KEGraphNodeControl.h"

KEGraphConnectionState::KEGraphConnectionState(PortType port)
	: m_RequiredPort(port),
	m_LastHorveredNode(nullptr)
{
}

KEGraphConnectionState::~KEGraphConnectionState()
{
	ResetLastHoveredNode();
}

void KEGraphConnectionState::InteractWithNode(KEGraphNodeControl* node)
{
	if (node)
	{
		m_LastHorveredNode = node;
	}
	else
	{
		ResetLastHoveredNode();
	}
}

void KEGraphConnectionState::SetLastHoveredNode(KEGraphNodeControl* node)
{
	m_LastHorveredNode = node;
}

void KEGraphConnectionState::ResetLastHoveredNode()
{
	if (m_LastHorveredNode)
	{
		m_LastHorveredNode->ResetReactionToConnection();
	}
	m_LastHorveredNode = nullptr;
}