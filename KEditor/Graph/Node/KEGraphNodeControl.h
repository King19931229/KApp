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
	QUuid m_ID;
	KEGraphNodeState m_NodeState;
	KEGraphNodeGeometry m_Geometry;
public:
	KEGraphNodeControl(KEGraphNodeModelPtr&& model);
	virtual ~KEGraphNodeControl();

	void SetView(KEGraphNodeViewPtr&& view);
	KEGraphNodeView* GetView() { return m_View.get(); }

	inline KEGraphNodeGeometry& GetGeometry() { return m_Geometry; }
	inline KEGraphNodeState& GetNodeState() { return m_NodeState; }
	inline KEGraphNodeModel* GetModel() { return m_Model.get(); }

	inline QUuid ID() const { return m_ID; }

	void ResetReactionToConnection();
	void PropagateData(KEGraphNodeDataPtr data, PortIndexType inPortIndex);
};