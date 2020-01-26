#include "KEGraphNodeControl.h"
#include "KEGraphNodeModel.h"
#include "KEGraphNodeView.h"

KEGraphNodeControl::KEGraphNodeControl(KEGraphNodeModelPtr&& model)
	: m_Model(std::move(model)),
	m_View(nullptr),
	m_ID(QUuid::createUuid()),
	m_Geometry(m_Model),
	m_NodeState(m_Model)
{
	m_Geometry.RecalculateSize();
}

KEGraphNodeControl::~KEGraphNodeControl()
{
	m_Model = nullptr;
	m_View = nullptr;
}

void KEGraphNodeControl::SetView(KEGraphNodeViewPtr&& view)
{
	m_View = std::move(view);
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