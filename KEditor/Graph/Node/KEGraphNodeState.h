#pragma once
#include "Graph/KEGraphConfig.h"
#include "Graph/Node/KEGraphNodeData.h"
#include "Utility/QUuidStdHash.hpp"
#include <QtCore/QUuid>
#include <vector>
#include <unordered_map>

class KEGraphNodeState
{
public:
	enum ReactToConnectionState
	{
		REACTING,
		NOT_REACTING
	};
	typedef std::unordered_map<QUuid, KEGraphConnectionControl*> ConnectionPtrSet;
	typedef std::vector<ConnectionPtrSet> EntryConnectionList;
protected:
	EntryConnectionList m_InConnections;
	EntryConnectionList m_OutConnections;

	ReactToConnectionState m_Reaction;
	PortType m_ReactingPortType;
	KEGraphNodeDataType m_ReactingDataType;

	bool m_Resizing;
public:
	KEGraphNodeState(const KEGraphNodeModelPtr& model);
	~KEGraphNodeState();

	const EntryConnectionList& GetEntries(PortType portType) const;
	EntryConnectionList& GetEntries(PortType portType);

	ConnectionPtrSet Connections(PortType portType, PortIndexType portIndex) const;
	void SetConnection(PortType portType, PortIndexType portIndex, KEGraphConnectionControl& connection);
	void EraseConnection(PortType portType, PortIndexType portIndex, QUuid id);

	ReactToConnectionState Reaction() const;
	PortType ReactingPortType() const;
	KEGraphNodeDataType ReactingDataType() const;

	void SetReaction(ReactToConnectionState reaction,
		PortType reactingPortType = PT_NONE,
		KEGraphNodeDataType reactingDataType = KEGraphNodeDataType());

	bool IsReacting() const;
	void SetResizing(bool resizing);
	bool Resizing() const;
};