#include "KEGraphInteraction.h"
#include "Node/KEGraphNodeModel.h"
#include "Node/KEGraphNodeControl.h"
#include "Node/KEGraphNodeView.h"
#include "Node/KEGraphNodeGeometry.h"
#include "Node/KEGraphNodeState.h"
#include "Connection/KEGraphConnectionControl.h"
#include "Connection/KEGraphConnectionView.h"
#include "Connection/KEGraphConnectionGeometry.h"
#include "Connection/KEGraphConnectionState.h"

#include "KEGraphPort.h"

KEGraphInteraction::KEGraphInteraction(KEGraphNodeControl& node,
	KEGraphConnectionControl& connection,
	KEGraphScene& scene)
	: m_Node(&node),
	m_Connection(&connection),
	m_Scene(&scene)
{
}

KEGraphInteraction::~KEGraphInteraction()
{
}

PortType KEGraphInteraction::ConnectionRequiredPort() const
{
	const KEGraphConnectionState &state = m_Connection->ConnectionState();
	return state.RequiredPort();
}

QPointF KEGraphInteraction::ConnectionEndScenePosition(PortType portType) const
{
	KEGraphConnectionView* view = m_Connection->GetView();
	KEGraphConnectionGeometry& geometry = m_Connection->ConnectionGeometry();
	QPointF endPoint = geometry.GetEndPoint(portType);
	return endPoint;
}

QPointF KEGraphInteraction::NodePortScenePosition(PortType portType, PortIndexType portIndex) const
{
	const KEGraphNodeGeometry& geometry = m_Node->GetGeometry();
	QPointF p = geometry.PortScenePosition(portIndex, portType);
	KEGraphNodeView* view = m_Node->GetView();
	return view->sceneTransform().map(p);
}

PortIndexType KEGraphInteraction::NodePortIndexUnderScenePoint(PortType portType, QPointF const &scenePoint) const
{
	const KEGraphNodeGeometry& geometry = m_Node->GetGeometry();
	QTransform sceneTransform = m_Node->GetView()->sceneTransform();
	PortIndexType portIndex = geometry.CheckHitScenePoint(portType, scenePoint, sceneTransform);
	return portIndex;
}

bool KEGraphInteraction::NodePortIsEmpty(PortType portType, PortIndexType portIndex) const
{
	const KEGraphNodeState& nodeState = m_Node->GetNodeState();

	auto const & entries = nodeState.GetEntries(portType);

	if (entries[portIndex].empty())
	{
		return true;
	}

	ConnectionPolicy outPolicy = m_Node->GetModel()->PortOutConnectionPolicy(portIndex);
	return (portType == PT_OUT && outPolicy == CP_MANY);
}

bool KEGraphInteraction::CanConnect(PortIndexType& portIndex, GraphNodeDataConverterFunc& converter) const
{
	// 1) Connection requires a port
	PortType requiredPort = ConnectionRequiredPort();
	if (requiredPort == PT_NONE)
	{
		return false;
	}

	// 1.5) Forbid connecting the node to itself
	KEGraphNodeControl* node = m_Connection->GetNode(OppositePort(requiredPort));
	if (node == nullptr)
	{
		return false;
	}

	if (node == m_Node)
	{
		return false;
	}

	// 2) connection point is on top of the node port
	QPointF connectionPoint = ConnectionEndScenePosition(requiredPort);
	portIndex = NodePortIndexUnderScenePoint(requiredPort, connectionPoint);

	if (portIndex == INVALID_PORT_INDEX)
	{
		return false;
	}

	// 3) Node port is vacant
	// port should be empty
	if (!NodePortIsEmpty(requiredPort, portIndex))
	{
		return false;
	}

	// 4) Connection type equals node port type, or there is a registered type conversion that can translate between the two
	auto connectionDataType = m_Connection->DataType(OppositePort(requiredPort));

	const KEGraphNodeModel *modelTarget = m_Node->GetModel();
	KEGraphNodeDataType candidateNodeDataType = modelTarget->DataType(requiredPort, portIndex);

	if (connectionDataType.id != candidateNodeDataType.id)
	{
		if (requiredPort == PT_IN)
		{
			// TODO
			// converter = m_Scene->registry().getTypeConverter(connectionDataType, candidateNodeDataType);
		}
		else if (requiredPort == PT_OUT)
		{
			// TODO
			// converter = m_Scene->registry().getTypeConverter(candidateNodeDataType, connectionDataType);
		}

		return (converter != nullptr);
	}

	return true;
}

bool KEGraphInteraction::TryConnect() const
{
	// 1) Check conditions from 'canConnect'
	PortIndexType portIndex = INVALID_PORT_INDEX;
	GraphNodeDataConverterFunc converter;
	if (!CanConnect(portIndex, converter))
	{
		return false;
	}

	// 1.5) If the connection is possible but a type conversion is needed,
	//      assign a convertor to connection
	if (converter)
	{
		m_Connection->SetTypeConverter(converter);
	}

	// 2) Assign node to required port in Connection
	PortType requiredPort = ConnectionRequiredPort();
	m_Node->GetNodeState().SetConnection(requiredPort, portIndex, *m_Connection);

	// 3) Assign Connection to empty port in NodeState
	// The port is not longer required after this function
	m_Connection->SetNodeToPort(m_Node, requiredPort, portIndex);

	// 4) Adjust Connection geometry
	m_Node->GetView()->MoveConnections();

	// 5) Poke model to intiate data transfer
	KEGraphNodeControl* outNode = m_Connection->GetNode(PT_OUT);
	if (outNode)
	{
		PortIndexType outPortIndex = m_Connection->GetPortIndex(PT_OUT);
		outNode->OnDataUpdated(outPortIndex);
	}

	return true;
}

/// 1) Node and Connection should be already connected
/// 2) If so, clear Connection entry in the NodeState
/// 3) Set Connection end to 'requiring a port'
bool KEGraphInteraction::Disconnect(PortType portToDisconnect) const
{
	PortIndexType portIndex = m_Connection->GetPortIndex(portToDisconnect);

	KEGraphNodeState& state = m_Node->GetNodeState();

	// clear pointer to Connection in the NodeState
	state.GetEntries(portToDisconnect)[portIndex].clear();

	// 4) Propagate invalid data to IN node
	m_Connection->PropagateEmptyData();

	// clear Connection side
	m_Connection->ClearNode(portToDisconnect);

	m_Connection->SetRequiredPort(portToDisconnect);

	m_Connection->GetView()->grabMouse();

	return true;
}