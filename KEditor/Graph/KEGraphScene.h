#pragma once
#include "KEGraphPredefine.h"
#include "Node/KEGraphNodeControl.h"
#include "Utility/QUuidStdHash.hpp"
#include <QGraphicsScene>
#include <unordered_map>

class KEGraphScene : public QGraphicsScene
{
	Q_OBJECT
protected:
	std::unordered_map<QUuid, KEGraphNodeControlPtr> m_Node;
public:
	KEGraphScene(QObject * parent);
	~KEGraphScene();

	KEGraphNodeControl* CreateNode(KEGraphNodeModelPtr&& model);
	void RemoveNode(KEGraphNodeControl* node);

Q_SIGNALS:
	void NodeCreated(KEGraphNodeControl* n);
	void NodePlaced(KEGraphNodeControl* n);
	void NodeDeleted(KEGraphNodeControl* n);
	void NodeMoved(KEGraphNodeControl* node, const QPointF& newLocation);
	void NodeDoubleClicked(KEGraphNodeControl* node);
};