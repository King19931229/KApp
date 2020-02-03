#pragma once
#include <QGraphicsScene>
#include <unordered_map>

#include "KEGraphConfig.h"

#include "Node/KEGraphNodeControl.h"
#include "Utility/QUuidStdHash.hpp"

class KEGraphScene : public QGraphicsScene
{
	Q_OBJECT
protected:
	std::unordered_map<QUuid, KEGraphNodeControlPtr> m_Node;
	std::unordered_map<QUuid, KEGraphConnectionControlPtr> m_Connection;
	KEGraphRegistrarPtr m_Registrar;

	void ClearScene();
public:
	KEGraphScene(QObject * parent);
	~KEGraphScene();

	KEGraphRegistrar* GetRegistrar();

	KEGraphNodeControl* CreateNode(KEGraphNodeModelPtr&& model);
	void RemoveNode(KEGraphNodeControl* node);

	KEGraphConnectionControl* CreateConnection(PortType connectedPort,
		KEGraphNodeControl* node,
		PortIndexType portIndex);
	KEGraphConnectionControl* CreateConnection(KEGraphNodeControl* nodeIn,
		PortIndexType portIndexIn,
		KEGraphNodeControl* nodeOut,
		PortIndexType portIndexOut,
		const GraphNodeDataConverterFunc& converter = GraphNodeDataConverterFunc{});
	void DeleteConnection(KEGraphConnectionControl* connection);

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

	void SingalConnectionCreated(const KEGraphConnectionControl* c);
	void SingalConnectionDeleted(const KEGraphConnectionControl* c);

	void SingalConnectionHovered(KEGraphConnectionControl* c, QPoint screenPos);
	void SingalConnectionLeft(KEGraphConnectionControl* c, QPoint screenPos);

	void SingalConnectionContextMenu(KEGraphConnectionControl* c, const QPointF& pos);
};