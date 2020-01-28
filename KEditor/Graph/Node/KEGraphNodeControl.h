#pragma once
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QJsonObject>

#include "Graph/KEGraphConfig.h"
#include "KEGraphNodeGeometry.h"
#include "KEGraphNodeState.h"

class KEGraphNodeControl : public QObject
{
	Q_OBJECT
protected:
	KEGraphNodeViewPtr m_View;
	KEGraphNodeModelPtr m_Model;
	KEGraphNodeState m_NodeState;
	KEGraphNodeGeometry m_Geometry;
	QUuid m_ID;
public:
	KEGraphNodeControl(KEGraphNodeModelPtr&& model);
	virtual ~KEGraphNodeControl();

	void SetView(KEGraphNodeViewPtr&& view);
	inline KEGraphNodeView* GetView() { return m_View.get(); }

	inline KEGraphNodeGeometry& GetGeometry() { return m_Geometry; }
	inline KEGraphNodeState& GetNodeState() { return m_NodeState; }
	inline KEGraphNodeModel* GetModel() { return m_Model.get(); }

	inline QUuid ID() const { return m_ID; }

	void ReactToPossibleConnection(PortType, const KEGraphNodeDataType&, const QPointF& scenePoint);
	void ResetReactionToConnection();

public Q_SLOTS:
	/// Propagates incoming data to the underlying model.
	void PropagateData(KEGraphNodeDataPtr data, PortIndexType inPortIndex);
	/// Fetches data from model's OUT #index port
	/// and propagates it to the connection
	void OnDataUpdated(PortIndexType index);
	/// update the graphic part if the size of the embeddedwidget changes
	void OnNodeSizeUpdated();
};