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
	void SingalNodeCreated(KEGraphNodeControl* n);
	void SingalNodePlaced(KEGraphNodeControl* n);
	void SingalNodeDeleted(KEGraphNodeControl* n);
	void SingalNodeMoved(KEGraphNodeControl* node, const QPointF& newLocation);
	void SingalNodeDoubleClicked(KEGraphNodeControl* node);
};