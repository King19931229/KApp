#pragma once

#include <QtCore/QUuid>
#include <QtWidgets/QGraphicsObject>
#include "Graph/KEGraphConfig.h"

class KEGraphConnectionView : public QGraphicsObject
{
	Q_OBJECT
protected:
	KEGraphScene* m_Scene;
	KEGraphConnectionControl* m_Connection;
	void AddGraphicsEffect();
public:
	KEGraphConnectionView(KEGraphScene* scene, KEGraphConnectionControl* connection);
	virtual	~KEGraphConnectionView();

	void SetGeometryChanged();
	/// Updates the position of both ends
	void Move();
	void Lock(bool locked);

	enum { Type = UserType + 2 };
	inline KEGraphConnectionControl* Connection() { return m_Connection; }
	// override
	virtual int	type() const override { return Type; }
	virtual QRectF boundingRect() const override;
	virtual QPainterPath shape() const override;
protected:
	// override
	virtual void paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget = 0) override;
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
};