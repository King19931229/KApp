#include "KEGraphNodeState.h"
#include "Graph/Node/KEGraphNodeControl.h"
#include "Graph/Node/KEGraphNodeModel.h"
#include "Graph/Connection/KEGraphConnectionControl.h"
#include <assert.h>

KEGraphNodeState::KEGraphNodeState(const KEGraphNodeModelPtr& model)
	: m_InConnections(model->NumPorts(PT_IN)),
	 m_OutConnections(model->NumPorts(PT_OUT)),
	 m_Reaction(NOT_REACTING),
	 m_ReactingPortType(PT_NONE),
	 m_Resizing(false)
{}

KEGraphNodeState::~KEGraphNodeState()
{
}

const KEGraphNodeState::EntryConnectionList& KEGraphNodeState::GetEntries(PortType portType) const
{
	if (portType == PT_IN)
	{
		return m_InConnections;
	}
	else
	{
		return m_OutConnections;
	}
}

KEGraphNodeState::EntryConnectionList& KEGraphNodeState::GetEntries(PortType portType)
{
	if (portType == PT_IN)
	{
		return m_InConnections;
	}
	else
	{
		return m_OutConnections;
	}
}

KEGraphNodeState::ConnectionPtrSet KEGraphNodeState::Connections(PortType portType, PortIndexType portIndex) const
{
	const EntryConnectionList& connections = GetEntries(portType);
	assert(portIndex < connections.size());
	return connections.at(portIndex);
}

void KEGraphNodeState::SetConnection(PortType portType, PortIndexType portIndex, KEGraphConnectionControl& connection)
{
	auto &connections = GetEntries(portType);
	assert(portIndex < connections.size());
	connections.at(portIndex).insert(std::make_pair(connection.ID(), &connection));
}

void KEGraphNodeState::EraseConnection(PortType portType, PortIndexType portIndex, QUuid id)
{
	auto &connections = GetEntries(portType);
	assert(portIndex < connections.size());
	connections.at(portIndex).erase(id);
}

KEGraphNodeState::ReactToConnectionState KEGraphNodeState::Reaction() const
{
	return m_Reaction;
}

PortType KEGraphNodeState::ReactingPortType() const
{
	return m_ReactingPortType;
}

KEGraphNodeDataType KEGraphNodeState::ReactingDataType() const
{
	return m_ReactingDataType;
}

void KEGraphNodeState::SetReaction(ReactToConnectionState reaction,
	PortType reactingPortType,
	KEGraphNodeDataType reactingDataType)
{
	m_Reaction = reaction;
	m_ReactingPortType = reactingPortType;
	m_ReactingDataType = std::move(reactingDataType);
}

bool KEGraphNodeState::IsReacting() const
{
	return m_Reaction == REACTING;
}

void KEGraphNodeState::SetResizing(bool resizing)
{
	m_Resizing = resizing;
}

bool KEGraphNodeState::Resizing() const
{
	return m_Resizing;
}