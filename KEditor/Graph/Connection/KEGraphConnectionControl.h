#pragma once

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QVariant>

#include "Graph/KEGraphConfig.h"

#include "Graph/Node/KEGraphNodeData.h"
#include "Graph/Node/KEGraphNodeControl.h"
#include "Graph/Node/KEGraphNodeView.h"
#include "Graph/Node/KEGraphNodeModel.h"

#include "KEGraphConnectionState.h"
#include "KEGraphConnectionGeometry.h"
#include "KEGraphConnectionView.h"

class KEGraphConnectionControl : public QObject
{
	Q_OBJECT
protected:
	QUuid m_ID;

	KEGraphNodeControl* m_OutNode = nullptr;
	KEGraphNodeControl* m_InNode = nullptr;

	PortIndexType m_OutPortIndex;
	PortIndexType m_InPortIndex;

	KEGraphConnectionState m_ConnectionState;
	KEGraphConnectionGeometry m_ConnectionGeometry;

	KEGraphConnectionViewPtr m_View;
	GraphNodeDataConverterFunc m_Converter;
public:
	/// New Connection is attached to the port of the given Node.
	/// The port has parameters (portType, portIndex).
	/// The opposite connection end will require anothre port.
	KEGraphConnectionControl(PortType portType,
		KEGraphNodeControl* node,
		PortIndexType portIndex);

	KEGraphConnectionControl(KEGraphNodeControl* nodeIn,
		PortIndexType portIndexIn,
		KEGraphNodeControl* nodeOut,
		PortIndexType portIndexOut,
		GraphNodeDataConverterFunc converter =
		GraphNodeDataConverterFunc{});

	~KEGraphConnectionControl();

	QUuid ID() const;

	/// Remembers the end being dragged.
	/// Invalidates Node address.
	/// Grabs mouse.
	void SetRequiredPort(PortType portType);
	PortType RequiredPort() const;

	void SetView(KEGraphConnectionViewPtr&& graphics);
	inline KEGraphConnectionView* GetView() const { return m_View.get(); }

	/// Assigns a node to the required port.
	/// It is assumed that there is a required port, no extra checks
	void SetNodeToPort(KEGraphNodeControl* node, PortType portType, PortIndexType portIndex);
	void RemoveFromNodes() const;
public:
	KEGraphConnectionView* GetConnectionView() const { return m_View.get(); }

	const KEGraphConnectionState & ConnectionState() const { return m_ConnectionState; }
	KEGraphConnectionState& ConnectionState() { return m_ConnectionState; }

	KEGraphConnectionGeometry& ConnectionGeometry() { return m_ConnectionGeometry; }
	const KEGraphConnectionGeometry& ConnectionGeometry() const { return m_ConnectionGeometry; }

	KEGraphNodeControl*& GetNode(PortType portType);

	PortIndexType GetPortIndex(PortType portType) const;
	void ClearNode(PortType portType);

	KEGraphNodeDataType DataType(PortType portType) const;

	void SetTypeConverter(GraphNodeDataConverterFunc converter);
	bool Complete() const;

public: // data propagation
	void PropagateData(KEGraphNodeDataPtr nodeData) const;
	void PropagateEmptyData() const;
Q_SIGNALS:
	void SingalConnectionCompleted(KEGraphConnectionControl*) const;
	void SingalConnectionMadeIncomplete(KEGraphConnectionControl*) const;
	void SingalUpdated(KEGraphConnectionControl* conn) const;
};