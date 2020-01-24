#include "KEGraphScene.h"
#include "Node/KEGraphNodeControl.h"
#include "Node/KEGraphNodeView.h"
#include "Node/KEGraphNodeModel.h"

KEGraphScene::KEGraphScene(QObject * parent)
	: QGraphicsScene(parent)
{
	setItemIndexMethod(QGraphicsScene::NoIndex);
}

KEGraphScene::~KEGraphScene()
{
}

KEGraphNodeControl* KEGraphScene::CreateNode(KEGraphNodeModelPtr&& model)
{
	KEGraphNodeControlPtr node = KEGraphNodeControlPtr(new KEGraphNodeControl(std::move(model)));
	KEGraphNodeViewPtr view = KEGraphNodeViewPtr(new KEGraphNodeView(this, node.get()));

	m_Node[node->ID()] = std::move(node);

	NodeCreated(node.get());

	return node.get();
}

void KEGraphScene::RemoveNode(KEGraphNodeControl* node)
{
	NodeDeleted(node);

	// TODO

	m_Node.erase(node->ID());
}