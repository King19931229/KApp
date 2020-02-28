#pragma once
#include <QGraphicsScene>
#include <unordered_map>

#include "KEGraphConfig.h"

#include "Node/KEGraphNodeControl.h"
#include "Utility/QUuidStdHash.hpp"

class KEGraphScene : public QGraphicsScene
{
	Q_OBJECT
public:
	// Command
	friend class KEGraphNodeRemoveCommand;
	friend class KEGraphNodeCreateCommand;
	friend class KEGraphConnectionRemoveCommand;
	friend class KEGraphConnectionCreateCommand;

	typedef std::unordered_map<QUuid, KEGraphNodeControlPtr> NodeDict;
	typedef std::unordered_map<QUuid, KEGraphConnectionControlPtr> ConnectionDict;
protected:
	NodeDict m_Node;
	ConnectionDict m_Connection;
	KEGraphRegistrarPtr m_Registrar;

public:
	KEGraphScene(QObject * parent);
	~KEGraphScene();

	void ClearScene();

	KEGraphRegistrar* GetRegistrar();

	inline const NodeDict& GetAllNodes() const { return m_Node; }
	inline const ConnectionDict& GetAllConnections() const { return m_Connection; }

	KEGraphNodeControl* CreateNode(KEGraphNodeModelPtr&& model);
	void RemoveNode(KEGraphNodeControlPtr node);
	void RemoveNode(const QUuid& id);

	KEGraphConnectionControl* CreateConnection(PortType connectedPort,
		KEGraphNodeControl* node,
		PortIndexType portIndex);
	KEGraphConnectionControl* CreateConnection(KEGraphNodeControl* nodeIn,
		PortIndexType portIndexIn,
		KEGraphNodeControl* nodeOut,
		PortIndexType portIndexOut,
		const GraphNodeDataConverterFunc& converter = GraphNodeDataConverterFunc{});
	void DeleteConnection(KEGraphConnectionControlPtr connection);
	void DeleteConnection(const QUuid& id);

	KEGraphConnectionControlPtr GetConnection(const QUuid& id);

	KEGraphNodeControl* LocateNodeAt(QPointF scenePoint);
Q_SIGNALS:
	void SingalNodeCreated(KEGraphNodeControl* n);
	void SingalNodePlaced(KEGraphNodeControl* n);
	void SingalNodeDeleted(KEGraphNodeControl* n);
	void SingalNodeMoved(KEGraphNodeControl* node, const QPointF& newLocation);
	void SingalNodeDoubleClicked(KEGraphNodeControl* node);

	void SingalNodeHovered(KEGraphNodeControl* n, QPoint screenPos);
	void SingalNodeHoverLeft(KEGraphNodeControl* n);

	void SingalNodeContextMenu(KEGraphNodeControl* n, const QPointF& pos);

	void SingalConnectionCreated(KEGraphConnectionControl* c);
	void SingalConnectionDeleted(KEGraphConnectionControl* c);

	void SingalConnectionHovered(KEGraphConnectionControl* c, QPoint screenPos);
	void SingalConnectionLeft(KEGraphConnectionControl* c, QPoint screenPos);

	void SingalConnectionContextMenu(KEGraphConnectionControl* c, const QPointF& pos);
};