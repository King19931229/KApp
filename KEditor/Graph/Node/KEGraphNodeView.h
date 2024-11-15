#pragma once
#include <QtCore/QUuid>
#include <QtWidgets/QGraphicsObject>
#include <QGraphicsProxyWidget>

#include "Graph/KEGraphConfig.h"

class KEGraphNodeView : public QGraphicsObject
{
	Q_OBJECT
protected:
	KEGraphScene* m_Scene;
	KEGraphNodeControl* m_Node;
	QGraphicsProxyWidget* m_ProxyWidget;
	bool m_Locked;

	void EmbedQWidget();
public:
	KEGraphNodeView(KEGraphScene* scene, KEGraphNodeControl* control);
	virtual ~KEGraphNodeView();

	inline KEGraphNodeControl* GetControl() { return m_Node; }

	enum { Type = UserType + 1 };
	int	type() const override { return Type; }

	void Lock(bool locked);
	void SetGeometryChanged();	
	void MoveConnections();

	// override
	virtual void paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget = 0) override;

	virtual QRectF boundingRect() const override;
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
};