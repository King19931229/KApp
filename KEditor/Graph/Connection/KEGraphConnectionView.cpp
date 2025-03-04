#include "KEGraphConnectionView.h"

#include "Graph/Connection/KEGraphConnectionControl.h"
#include "Graph/Connection/KEGraphConnectionView.h"

#include "Graph/KEGraphScene.h"
#include "Graph/KEGraphPort.h"
#include "Graph/KEGraphPainter.h"
#include "Graph/KEGraphInteraction.h"

#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QGraphicsBlurEffect>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QGraphicsView>

KEGraphConnectionView::KEGraphConnectionView(KEGraphScene* scene, KEGraphConnectionControl* connection)
	: m_Scene(scene),
	m_Connection(connection)
{
	//m_Scene->addItem(this);

	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);

	setAcceptHoverEvents(true);

	// AddGraphicsEffect();

	setZValue(-1.0);
}

KEGraphConnectionView::~KEGraphConnectionView()
{
	//m_Scene->removeItem(this);
}

void KEGraphConnectionView::AddGraphicsEffect()
{
	auto effect = KNEW QGraphicsBlurEffect;

	effect->setBlurRadius(5);
	setGraphicsEffect(effect);

	// QGraphicsDropShadowEffect* effect = KNEW QGraphicsDropShadowEffect;
	// ConnectionBlurEffect* effect = KNEW ConnectionBlurEffect(this);
	// effect->setOffset(4, 4);
	// effect->setColor(QColor(Qt::gray).darker(800));
}

void KEGraphConnectionView::SetGeometryChanged()
{
	prepareGeometryChange();
}

void KEGraphConnectionView::Move()
{
	for (PortType portType : { PT_IN, PT_OUT })
	{
		KEGraphNodeControl* node = m_Connection->Node(portType);
		if (node)
		{
			const KEGraphNodeView* nodeGraphics = node->GetView();
			const KEGraphNodeGeometry& nodeGeom = node->GetGeometry();

			QPointF scenePos = nodeGeom.PortScenePosition(m_Connection->GetPortIndex(portType),
				portType, nodeGraphics->sceneTransform());

			QTransform sceneTransform = this->sceneTransform();

			QPointF connectionPos = sceneTransform.inverted().map(scenePos);

			m_Connection->ConnectionGeometry().SetEndPoint(portType, connectionPos);

			m_Connection->GetView()->SetGeometryChanged();
			m_Connection->GetView()->update();
		}
	}
}

void KEGraphConnectionView::Lock(bool locked)
{
	setFlag(QGraphicsItem::ItemIsMovable, !locked);
	setFlag(QGraphicsItem::ItemIsFocusable, !locked);
	setFlag(QGraphicsItem::ItemIsSelectable, !locked);
}

QRectF KEGraphConnectionView::boundingRect() const
{
	return m_Connection->ConnectionGeometry().BoundingRect();
}

QPainterPath KEGraphConnectionView::shape() const
{
	const KEGraphConnectionGeometry& geom = m_Connection->ConnectionGeometry();
	return KEGraphPainter::GetPainterStroke(geom);
}

void KEGraphConnectionView::paint(QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget* widget)
{
	painter->setClipRect(option->exposedRect);
	KEGraphPainter::Paint(painter, *m_Connection);
}

void KEGraphConnectionView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsItem::mousePressEvent(event);
	//event->ignore();
}

void KEGraphConnectionView::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	prepareGeometryChange();

	QGraphicsView* view = static_cast<QGraphicsView*>(event->widget());
	KEGraphNodeControl* node = m_Scene->LocateNodeAt(event->scenePos());

	KEGraphConnectionState& state = m_Connection->ConnectionState();

	state.InteractWithNode(node);
	if (node)
	{
		node->ReactToPossibleConnection(state.RequiredPort(),
			m_Connection->DataType(OppositePort(state.RequiredPort())),
			event->scenePos());
	}

	QPointF offset = event->pos() - event->lastPos();
	auto requiredPort = m_Connection->RequiredPort();
	if (requiredPort != PT_NONE)
	{
		m_Connection->ConnectionGeometry().MoveEndPoint(requiredPort, offset);
	}

	update();
	event->accept();
}

void KEGraphConnectionView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	ungrabMouse();
	event->accept();

	KEGraphNodeControl* node = m_Scene->LocateNodeAt(event->scenePos());

	KEGraphInteraction interaction(*node, *m_Connection, *m_Scene);

	if (node && interaction.TryConnect())
	{
		node->ResetReactionToConnection();
	}

	if (m_Connection->ConnectionState().RequiresPort())
	{
		m_Scene->DeleteConnection(m_Connection->ID());
	}
}

void KEGraphConnectionView::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	m_Connection->ConnectionGeometry().SetHovered(true);
	update();
	m_Scene->SingalConnectionHovered(Connection(), event->screenPos());
	event->accept();
}

void KEGraphConnectionView::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	m_Connection->ConnectionGeometry().SetHovered(false);
	update();
	m_Scene->SingalConnectionLeft(Connection(), event->screenPos());
	event->accept();
}

void KEGraphConnectionView::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
	m_Scene->SingalConnectionContextMenu(m_Connection, mapToScene(event->pos()));
}