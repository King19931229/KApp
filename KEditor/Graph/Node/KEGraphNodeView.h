#pragma once
#include <QtCore/QUuid>
#include <QtWidgets/QGraphicsObject>
#include "Graph/KEGraphPredefine.h"

class KEGraphNodeView : public QGraphicsObject
{
	Q_OBJECT
protected:
	KEGraphScene* m_Scene;
	KEGraphNodeControl* m_Control;
public:
	KEGraphNodeView(KEGraphScene* scene, KEGraphNodeControl* control);
	virtual ~KEGraphNodeView();

	enum { Type = UserType + 1 };

	void SetGeometryChanged();	
	void MoveConnections();

	// override
	virtual void paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget = 0);

	virtual QRectF boundingRect() const;
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
};