#include "KEGraphNodeControl.h"
#include "KEGraphNodeModel.h"
#include "KEGraphNodeView.h"

KEGraphNodeControl::KEGraphNodeControl(KEGraphNodeModelPtr&& model)
	: m_Model(std::move(model)),
	m_View(nullptr),
	m_ID(QUuid::createUuid()),
	m_Geometry(m_Model)
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