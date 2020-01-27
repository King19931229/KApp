#include "KEGraphConnectionView.h"
#include "Graph/KEGraphScene.h"
#include "Graph/Connection/KEGraphConnectionControl.h"
#include "Graph/Connection/KEGraphConnectionView.h"
#include "Graph/KEGraphPainter.h"

#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QGraphicsBlurEffect>
#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QGraphicsView>

KEGraphConnectionView::KEGraphConnectionView(KEGraphScene* scene, KEGraphConnectionControl* connection)
	: m_Scene(scene),
	m_Connection(connection)
{
	m_Scene->addItem(this);

	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);

	setAcceptHoverEvents(true);

	// AddGraphicsEffect();

	setZValue(-1.0);
}

KEGraphConnectionView::~KEGraphConnectionView()
{
	m_Scene->removeItem(this);
}

void KEGraphConnectionView::AddGraphicsEffect()
{
	auto effect = new QGraphicsBlurEffect;

	effect->setBlurRadius(5);
	setGraphicsEffect(effect);

	// QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
	// ConnectionBlurEffect* effect = new ConnectionBlurEffect(this);
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
		KEGraphNodeControl* node = nullptr;
		if (m_Connection->GetNode(portType, &node))
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
	// TODO
}

void KEGraphConnectionView::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	// TODO
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