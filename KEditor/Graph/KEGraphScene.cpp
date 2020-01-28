#include "KEGraphScene.h"

#include "Node/KEGraphNodeControl.h"
#include "Node/KEGraphNodeView.h"
#include "Node/KEGraphNodeModel.h"

#include "Connection/KEGraphConnectionControl.h"
#include "Connection/KEGraphConnectionView.h"

#include <QGraphicsView>

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

KEGraphConnectionControl* KEGraphScene::CreateConnection(PortType connectedPort,
	KEGraphNodeControl* node,
	PortIndexType portIndex)
{
	KEGraphConnectionControlPtr connection = KEGraphConnectionControlPtr(new KEGraphConnectionControl(
		connectedPort,
		node,
		portIndex));

	KEGraphConnectionControl* conn = connection.get();
	KEGraphConnectionViewPtr view = KEGraphConnectionViewPtr(new KEGraphConnectionView(this, conn));

	// after this function connection points are set to node port
	connection->SetView(std::move(view));

	m_Connection[connection->ID()] = std::move(connection);

	// Note: this connection isn't truly created yet. It's only partially created.
	// Thus, don't send the connectionCreated(...) signal.

	connect(conn,
		&KEGraphConnectionControl::SingalConnectionCompleted,
		this,
		[this](const KEGraphConnectionControl* c)
	{
		SingalConnectionCreated(c);
	});

	return conn;
}

KEGraphConnectionControl* KEGraphScene::CreateConnection(KEGraphNodeControl* nodeIn,
	PortIndexType portIndexIn,
	KEGraphNodeControl* nodeOut,
	PortIndexType portIndexOut,
	const GraphNodeDataConverterFunc& converter)
{
	KEGraphConnectionControlPtr connection = KEGraphConnectionControlPtr(new KEGraphConnectionControl(
		nodeIn,
		portIndexIn,
		nodeOut,
		portIndexOut,
		converter));

	KEGraphConnectionControl* conn = connection.get();
	KEGraphConnectionViewPtr view = KEGraphConnectionViewPtr(new KEGraphConnectionView(this, conn));

	nodeIn->GetNodeState().SetConnection(PT_IN, portIndexIn, *connection);
	nodeOut->GetNodeState().SetConnection(PT_OUT, portIndexOut, *connection);

	// after this function connection points are set to node port
	connection->SetView(std::move(view));

	// trigger data propagation
	nodeOut->OnDataUpdated(portIndexOut);

	m_Connection[connection->ID()] = std::move(connection);

	SingalConnectionCreated(conn);

	return conn;
}

void KEGraphScene::DeleteConnection(KEGraphConnectionControl* connection)
{
	if (connection)
	{
		auto it = m_Connection.find(connection->ID());
		if (it != m_Connection.end())
		{
			connection->RemoveFromNodes();
			m_Connection.erase(it);
		}
	}
}

KEGraphNodeControl* KEGraphScene::LocateNodeAt(QPointF scenePoint)
{
	const QTransform viewTransform = views()[0]->transform();

	// items under cursor
	QList<QGraphicsItem*> items = this->items(
		scenePoint,
		Qt::IntersectsItemShape,
		Qt::DescendingOrder,
		viewTransform);

	//// items convertable to GraphNodeView
	std::vector<QGraphicsItem*> filteredItems;

	std::copy_if(items.begin(),
		items.end(),
		std::back_inserter(filteredItems),
		[](QGraphicsItem * item)
	{
		return (dynamic_cast<KEGraphNodeView*>(item) != nullptr);
	});

	KEGraphNodeControl* resultNode = nullptr;

	if (!filteredItems.empty())
	{
		QGraphicsItem* graphicsItem = filteredItems.front();
		KEGraphNodeView *view = dynamic_cast<KEGraphNodeView*>(graphicsItem);
		resultNode = view->GetControl();
	}

	return resultNode;
}