#include "KEGraphNodeView.h"
#include "Graph/KEGraphScene.h"
#include "KEGraphNodeControl.h"

#include <QtWidgets/QGraphicsEffect>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

KEGraphNodeView::KEGraphNodeView(KEGraphScene* scene, KEGraphNodeControl* control)
	: m_Scene(scene),
	m_Control(control)
{
	m_Scene->addItem(this);

	setFlag(QGraphicsItem::ItemDoesntPropagateOpacityToChildren, true);
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

	setCacheMode(QGraphicsItem::DeviceCoordinateCache);

	auto effect = new QGraphicsDropShadowEffect;
	effect->setOffset(4, 4);
	effect->setBlurRadius(20);
	effect->setColor(QColor(20, 20, 20));
	setGraphicsEffect(effect);

	setOpacity(0.8);

	setAcceptHoverEvents(true);

	setZValue(0);

	// connect to the move signals to emit the move signals in FlowScene
	auto onMoveSlot = [this]
	{
		m_Scene->NodeMoved(m_Control, pos());
	};
	connect(this, &QGraphicsObject::xChanged, this, onMoveSlot);
	connect(this, &QGraphicsObject::yChanged, this, onMoveSlot);
}

KEGraphNodeView::~KEGraphNodeView()
{
}

void KEGraphNodeView::SetGeometryChanged()
{
	prepareGeometryChange();
}

void KEGraphNodeView::MoveConnections()
{
	// TODO
	return;
}

// override
void KEGraphNodeView::paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget)
{
	painter->setClipRect(option->exposedRect);
	// TODO
}

QRectF KEGraphNodeView::boundingRect() const
{
	return m_Control->GetGeometry().boundingRect();
}

QVariant KEGraphNodeView::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionChange && scene())
	{
		MoveConnections();
	}

	return QGraphicsItem::itemChange(change, value);
}

void KEGraphNodeView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{

}

void KEGraphNodeView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{

}

void KEGraphNodeView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{

}

void KEGraphNodeView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{

}

void KEGraphNodeView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{

}

void KEGraphNodeView::hoverMoveEvent(QGraphicsSceneHoverEvent *)
{

}

void KEGraphNodeView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{

}

void KEGraphNodeView::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{

}