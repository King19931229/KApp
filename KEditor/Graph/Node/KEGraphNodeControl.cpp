#include "KEGraphNodeControl.h"
#include "KEGraphNodeModel.h"
#include "KEGraphNodeView.h"
#include "Graph/KEGraphScene.h"
#include "Graph/Connection/KEGraphConnectionControl.h"

KEGraphNodeControl::KEGraphNodeControl(KEGraphNodeModelPtr&& model)
	: m_Model(std::move(model)),
	m_View(nullptr),
	m_ID(QUuid::createUuid()),
	m_Geometry(m_Model),
	m_NodeState(m_Model),
	m_Scene(nullptr)
{
	m_Geometry.RecalculateSize();

	// propagate data: model => node
	connect(m_Model.get(), &KEGraphNodeModel::SingalDataUpdated, this, &KEGraphNodeControl::OnDataUpdated);
	connect(m_Model.get(), &KEGraphNodeModel::SingalEmbeddedWidgetSizeUpdated, this, &KEGraphNodeControl::OnNodeSizeUpdated);
}

KEGraphNodeControl::~KEGraphNodeControl()
{
}

void KEGraphNodeControl::SetView(KEGraphNodeViewPtr&& view)
{
	m_View = std::move(view);
	m_Scene = (KEGraphScene*)(m_View->scene());
}

void KEGraphNodeControl::ReactToPossibleConnection(PortType reactingPortType, const KEGraphNodeDataType& reactingDataType, const QPointF& scenePoint)
{
	const QTransform t = m_View->sceneTransform();

	QPointF p = t.inverted().map(scenePoint);

	m_Geometry.SetDraggingPosition(p);

	m_View->update();

	m_NodeState.SetReaction(KEGraphNodeState::REACTING,
		reactingPortType,
		reactingDataType);
}

void KEGraphNodeControl::ResetReactionToConnection()
{
	m_NodeState.SetReaction(KEGraphNodeState::NOT_REACTING);
	m_View->update();
}

void KEGraphNodeControl::PropagateData(KEGraphNodeDataPtr data, PortIndexType inPortIndex)
{
	m_Model->SetInData(std::move(data), inPortIndex);

	//Recalculate the nodes visuals. A data change can result in the node taking more space than before, so this forces a recalculate+repaint on the affected node
	m_View->SetGeometryChanged();
	m_Geometry.RecalculateSize();
	m_View->update();
	m_View->MoveConnections();
}

void KEGraphNodeControl::OnDataUpdated(PortIndexType index)
{
	KEGraphNodeDataPtr nodeData = m_Model->OutData(index);
	auto connections = m_NodeState.Connections(PT_OUT, index);

	for (auto const& c : connections)
	{
		c.second->PropagateData(nodeData);
	}
}

void KEGraphNodeControl::OnNodeSizeUpdated()
{
	if (m_Model->EmbeddedWidget())
	{
		m_Model->EmbeddedWidget()->adjustSize();
	}
	m_Geometry.RecalculateSize();
	for (PortType type : {PT_IN, PT_OUT})
	{
		for (auto& conn_set : m_NodeState.GetEntries(type))
		{
			for (auto& pair : conn_set)
			{
				KEGraphConnectionControl* conn = pair.second;
				conn->GetView()->Move();
			}
		}
	}
}

void KEGraphNodeControl::Exit()
{
	if (m_View->scene() == m_Scene)
	{
		m_Scene->removeItem(m_View.get());
	}
}

void KEGraphNodeControl::Enter()
{
	Exit();
	m_Scene->addItem(m_View.get());
}