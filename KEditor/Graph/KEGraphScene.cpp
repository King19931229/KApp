#include "KEGraphScene.h"
#include "KEGraphRegistrar.h"

#include "Node/KEGraphNodeControl.h"
#include "Node/KEGraphNodeView.h"
#include "Node/KEGraphNodeModel.h"

#include "Connection/KEGraphConnectionControl.h"
#include "Connection/KEGraphConnectionView.h"

#include "Command/KEGraphNodeCommand.h"
#include "Command/KEGraphConnectionCommand.h"

#include <QGraphicsView>

KEGraphScene::KEGraphScene(QObject * parent)
	: QGraphicsScene(parent),
	m_Registrar(KEGraphRegistrarPtr(KNEW KEGraphRegistrar()))
{
	setItemIndexMethod(QGraphicsScene::NoIndex);
	m_Registrar->Init();
}

KEGraphScene::~KEGraphScene()
{
	m_Registrar->UnInit();
	ClearScene();
}

KEGraphRegistrar* KEGraphScene::GetRegistrar()
{
	return m_Registrar.get();
}

void KEGraphScene::ClearScene()
{
	// Manual node cleanup. Simply clearing the holding datastructures doesn't work, the code crashes when
	// there are both nodes and connections in the scene. (The data propagation internal logic tries to propagate
	// data through already freed connections.)
	while (m_Connection.size() > 0)
	{
		DeleteConnection(m_Connection.begin()->second);
	}

	while (m_Node.size() > 0)
	{
		RemoveNode(m_Node.begin()->second);
	}
}

KEGraphNodeControl* KEGraphScene::CreateNode(KEGraphNodeModelPtr&& model)
{
	KEGraphNodeControlPtr node = KEGraphNodeControlPtr(KNEW KEGraphNodeControl(std::move(model)));
	KEGraphNodeViewPtr view = KEGraphNodeViewPtr(KNEW KEGraphNodeView(this, node.get()));

	KEGraphNodeControl* ret = node.get();
	ret->SetView(std::move(view));

	auto command = KECommandPtr(KNEW KEGraphNodeCreateCommand(this, node));
	KEditorGlobal::CommandInvoker.Execute(command);

	SingalNodeCreated(ret);
	return ret;
}

void KEGraphScene::RemoveNode(KEGraphNodeControlPtr node)
{
	SingalNodeDeleted(node.get());

	for (auto portType : { PT_IN, PT_OUT })
	{
		const KEGraphNodeState& nodeState = node->GetNodeState();
		const auto& nodeEntries = nodeState.GetEntries(portType);

		for (auto connections : nodeEntries)
		{
			for (auto const& pair : connections)
				DeleteConnection(pair.second->ID());
		}
	}

	auto command = KECommandPtr(KNEW KEGraphNodeRemoveCommand(this, node));
	KEditorGlobal::CommandInvoker.Execute(command);
}

void KEGraphScene::RemoveNode(const QUuid& id)
{
	KEGraphNodeControlPtr node = nullptr;

	auto it = m_Node.find(id);	
	if (it != m_Node.end())
	{
		node = it->second;
	}

	if (node)
	{
		RemoveNode(node);
	}
}

KEGraphConnectionControl* KEGraphScene::CreateConnection(PortType connectedPort,
	KEGraphNodeControl* node,
	PortIndexType portIndex)
{
	KEGraphConnectionControlPtr connection = KEGraphConnectionControlPtr(KNEW KEGraphConnectionControl(
		connectedPort,
		node,
		portIndex));

	KEGraphConnectionControl* conn = connection.get();
	KEGraphConnectionViewPtr view = KEGraphConnectionViewPtr(KNEW KEGraphConnectionView(this, conn));

	// after this function connection points are set to node port
	connection->SetView(std::move(view));

	auto command = KECommandPtr(KNEW KEGraphConnectionCreateCommand(this, connection));
	// Don't push into stack. A completed command will be pushed inside KEGraphInteraction
	command->Execute();

	return conn;
}

KEGraphConnectionControl* KEGraphScene::CreateConnection(KEGraphNodeControl* nodeIn,
	PortIndexType portIndexIn,
	KEGraphNodeControl* nodeOut,
	PortIndexType portIndexOut,
	const GraphNodeDataConverterFunc& converter)
{
	KEGraphConnectionControlPtr connection = KEGraphConnectionControlPtr(KNEW KEGraphConnectionControl(
		nodeIn,
		portIndexIn,
		nodeOut,
		portIndexOut,
		converter));

	KEGraphConnectionControl* conn = connection.get();
	KEGraphConnectionViewPtr view = KEGraphConnectionViewPtr(KNEW KEGraphConnectionView(this, conn));

	// after this function connection points are set to node port
	connection->SetView(std::move(view));

	auto command = KECommandPtr(KNEW KEGraphConnectionCreateCommand(this, connection));
	KEditorGlobal::CommandInvoker.Execute(command);

	return conn;
}

void KEGraphScene::DeleteConnection(KEGraphConnectionControlPtr connection)
{
	if (connection)
	{
		auto command = KECommandPtr(KNEW KEGraphConnectionRemoveCommand(this, connection));
		if (connection->Complete())
		{
			KEditorGlobal::CommandInvoker.Execute(command);
		}
		else
		{
			command->Execute();
		}
	}
}

void KEGraphScene::DeleteConnection(const QUuid& id)
{
	auto it = m_Connection.find(id);
	if (it != m_Connection.end())
	{
		DeleteConnection(it->second);
	}
}

KEGraphConnectionControlPtr KEGraphScene::GetConnection(const QUuid& id)
{
	auto it = m_Connection.find(id);
	if (it != m_Connection.end())
	{
		return it->second;
	}
	return nullptr;
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