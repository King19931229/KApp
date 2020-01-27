#include "KEGraphNodeView.h"
#include "KEGraphNodeControl.h"

#include "Graph/KEGraphScene.h"
#include "Graph/KEGraphPainter.h"

#include <QtWidgets/QGraphicsEffect>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>

KEGraphNodeView::KEGraphNodeView(KEGraphScene* scene, KEGraphNodeControl* control)
	: m_Scene(scene),
	m_Node(control),
	m_Locked(false)
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
		m_Scene->SingalNodeMoved(m_Node, pos());
	};
	connect(this, &QGraphicsObject::xChanged, this, onMoveSlot);
	connect(this, &QGraphicsObject::yChanged, this, onMoveSlot);
}

KEGraphNodeView::~KEGraphNodeView()
{

}

void KEGraphNodeView::Lock(bool locked)
{
	m_Locked = locked;

	setFlag(QGraphicsItem::ItemIsMovable, !locked);
	setFlag(QGraphicsItem::ItemIsFocusable, !locked);
	setFlag(QGraphicsItem::ItemIsSelectable, !locked);
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
	KEGraphPainter::Paint(painter, *m_Node, *m_Scene);
}

QRectF KEGraphNodeView::boundingRect() const
{
	return m_Node->GetGeometry().BoundingRect();
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
	if (m_Locked)
		return;

	// deselect all other items after this one is selected
	if (!isSelected() && !(event->modifiers() & Qt::ControlModifier))
	{
		m_Scene->clearSelection();
	}

	// TODO port resize
}

void KEGraphNodeView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	KEGraphNodeGeometry& geom = m_Node->GetGeometry();
	KEGraphNodeState& state = m_Node->GetNodeState();

	/*
	TODO
	if (state.resizing())
	{
		auto diff = event->pos() - event->lastPos();

		if (auto w = _node.nodeDataModel()->embeddedWidget())
		{
			prepareGeometryChange();

			auto oldSize = w->size();

			oldSize += QSize(diff.x(), diff.y());

			w->setFixedSize(oldSize);

			_proxyWidget->setMinimumSize(oldSize);
			_proxyWidget->setMaximumSize(oldSize);
			_proxyWidget->setPos(geom.widgetPosition());

			geom.recalculateSize();
			update();

			moveConnections();

			event->accept();
		}
	}
	else
	*/
	{
		QGraphicsObject::mouseMoveEvent(event);

		if (event->lastPos() != event->pos())
			MoveConnections();

		event->ignore();
	}

	QRectF r = scene()->sceneRect();

	r = r.united(mapToScene(boundingRect()).boundingRect());
	
	scene()->setSceneRect(r);
}

void KEGraphNodeView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	KEGraphNodeState& state = m_Node->GetNodeState();

	// TODO
	// state.setResizing(false);

	QGraphicsObject::mouseReleaseEvent(event);

	// position connections precisely after fast node move
	MoveConnections();
}

void KEGraphNodeView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	// bring all the colliding nodes to background
	QList<QGraphicsItem *> overlapItems = collidingItems();

	for (QGraphicsItem *item : overlapItems)
	{
		if (item->zValue() > 0.0)
		{
			item->setZValue(0.0);
		}
	}

	// bring this node forward
	setZValue(1.0);

	m_Node->GetGeometry().SetHovered(true);
	update();
	m_Scene->SingalNodeHovered(m_Node, event->screenPos());
	event->accept();
}

void KEGraphNodeView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	m_Node->GetGeometry().SetHovered(false);
	update();
	m_Scene->SingalNodeHoverLeft(m_Node);
	event->accept();
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