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

	KEGraphNodeControl* ret = node.get();
	ret->SetView(std::move(view));

	m_Node[node->ID()] = std::move(node);

	SingalNodeCreated(ret);
	return ret;
}

void KEGraphScene::RemoveNode(KEGraphNodeControl* node)
{
	SingalNodeDeleted(node);

	// TODO

	m_Node.erase(node->ID());
}