#include "KEGraphNodeView.h"
#include "KEGraphNodeControl.h"
#include "KEGraphNodeStyle.h"

#include "Graph/KEGraphScene.h"
#include "Graph/KEGraphInteraction.h"
#include "Graph/KEGraphPainter.h"

#include <QtWidgets/QGraphicsEffect>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>

KEGraphNodeView::KEGraphNodeView(KEGraphScene* scene, KEGraphNodeControl* control)
	: m_Scene(scene),
	m_Node(control),
	m_Locked(false),
	m_ProxyWidget(nullptr)
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
	effect->setColor(KEGraphNodeStyle::ShadowColor);
	setGraphicsEffect(effect);

	setOpacity(KEGraphNodeStyle::Opacity);

	setAcceptHoverEvents(true);

	setZValue(0);

	EmbedQWidget();

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
	const KEGraphNodeState& nodeState = m_Node->GetNodeState();

	for (PortType portType : {PT_IN, PT_OUT})
	{
		auto const & connectionEntries = nodeState.GetEntries(portType);

		for (auto const & connections : connectionEntries)
		{
			for (auto & con : connections)
				con.second->GetView()->Move();
		}
	}
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

	for (PortType portToCheck : {PT_IN, PT_OUT})
	{
		const KEGraphNodeGeometry& nodeGeometry = m_Node->GetGeometry();

		// TODO do not pass sceneTransform
		const PortIndexType portIndex = nodeGeometry.CheckHitScenePoint(
			portToCheck,
			event->scenePos(),
			sceneTransform());

		if (portIndex != INVALID_PORT_INDEX)
		{
			const KEGraphNodeState& nodeState = m_Node->GetNodeState();

			auto connections = nodeState.Connections(portToCheck, portIndex);

			// start dragging existing connection
			if (!connections.empty() && portToCheck == PT_IN)
			{
				KEGraphConnectionControl* conn = connections.begin()->second;
				KEGraphInteraction interaction(*m_Node, *conn, *m_Scene);
				interaction.Disconnect(portToCheck);
			}
			else // initialize new Connection
			{
				if (portToCheck == PT_OUT)
				{
					const ConnectionPolicy outPolicy = m_Node->GetModel()->PortOutConnectionPolicy(portIndex);
					if (!connections.empty() &&	outPolicy == CP_ONE)
					{
						m_Scene->DeleteConnection(connections.begin()->second);
					}
				}

				// todo add to FlowScene
				KEGraphConnectionControl* connection = m_Scene->CreateConnection(portToCheck,
					m_Node,
					portIndex);

				m_Node->GetNodeState().SetConnection(portToCheck,
					portIndex,
					*connection);

				connection->GetView()->grabMouse();
			}
		}
	}

	QPointF pos = event->pos();
	KEGraphNodeGeometry& geom = m_Node->GetGeometry();
	KEGraphNodeState& state = m_Node->GetNodeState();

	if (m_Node->GetModel()->Resizable() &&
		geom.ResizeRect().contains(QPoint(pos.x(), pos.y())))
	{
		state.SetResizing(true);
	}
}

void KEGraphNodeView::EmbedQWidget()
{
	KEGraphNodeGeometry& geom = m_Node->GetGeometry();

	if (auto w = m_Node->GetModel()->EmbeddedWidget())
	{
		m_ProxyWidget = new QGraphicsProxyWidget(this);

		m_ProxyWidget->setWidget(w);

		m_ProxyWidget->setPreferredWidth(5);

		geom.RecalculateSize();

		m_ProxyWidget->setPos(geom.WidgetPosition());

		update();

		m_ProxyWidget->setOpacity(1.0);
		m_ProxyWidget->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
	}
}

void KEGraphNodeView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	KEGraphNodeGeometry& geom = m_Node->GetGeometry();
	KEGraphNodeState& state = m_Node->GetNodeState();

	if (state.Resizing())
	{
		auto diff = event->pos() - event->lastPos();

		if (auto w = m_Node->GetModel()->EmbeddedWidget())
		{
			prepareGeometryChange();

			auto oldSize = w->size();

			oldSize += QSize(diff.x(), diff.y());

			w->setFixedSize(oldSize);

			m_ProxyWidget->setMinimumSize(oldSize);
			m_ProxyWidget->setMaximumSize(oldSize);
			m_ProxyWidget->setPos(geom.WidgetPosition());

			geom.RecalculateSize();
			update();

			MoveConnections();

			event->accept();
		}
	}
	else
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

	state.SetResizing(false);

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

void KEGraphNodeView::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	QPointF pos = event->pos();
	KEGraphNodeGeometry& geom = m_Node->GetGeometry();

	if (m_Node->GetModel()->Resizable() && geom.ResizeRect().contains(QPoint(pos.x(), pos.y())))
	{
		setCursor(QCursor(Qt::SizeFDiagCursor));
	}
	else
	{
		setCursor(QCursor());
	}

	event->accept();
}

void KEGraphNodeView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsItem::mouseDoubleClickEvent(event);
	m_Scene->SingalNodeDoubleClicked(m_Node);
}

void KEGraphNodeView::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
	m_Scene->SingalNodeContextMenu(m_Node, mapToScene(event->pos()));
}