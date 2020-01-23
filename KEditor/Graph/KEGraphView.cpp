#include "KEGraphView.h"
#include "KEGraphScene.h"
#include "KEditorConfig.h"
#include <QMouseEvent>
#include <QPainter>

KEGraphView::KEGraphView(QWidget *parent)
	: QGraphicsView(parent),
	m_Scene(nullptr)
{
	setDragMode(QGraphicsView::ScrollHandDrag);
	setRenderHint(QPainter::Antialiasing);

	QBrush backgroundBrush = QBrush(QColor(53, 53, 53));
	setBackgroundBrush(backgroundBrush);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setCacheMode(QGraphicsView::CacheBackground);

	m_Scene = new KEGraphScene(this);
	this->setScene(m_Scene);
}

KEGraphView::~KEGraphView()
{
}

void KEGraphView::ScaleUp()
{
	double const step = 1.2;
	double const factor = std::pow(step, 1.0);

	QTransform t = transform();

	if (t.m11() > 2.0)
		return;

	scale(factor, factor);
}

void KEGraphView::ScaleDown()
{
	double const step = 1.2;
	double const factor = std::pow(step, -1.0);

	scale(factor, factor);
}

void KEGraphView::wheelEvent(QWheelEvent *event)
{
	QPoint delta = event->angleDelta();

	if (delta.y() == 0)
	{
		event->ignore();
		return;
	}

	double const d = delta.y() / std::abs(delta.y());

	if (d > 0.0)
	{
		ScaleUp();
	}
	else
	{
		ScaleDown();
	}
}

void KEGraphView::mousePressEvent(QMouseEvent *event)
{
	QGraphicsView::mousePressEvent(event);
	if (event->button() == Qt::LeftButton)
	{
		m_ClickPos = mapToScene(event->pos());
	}
}

void KEGraphView::mouseMoveEvent(QMouseEvent *event)
{
	if (scene()->mouseGrabberItem() == nullptr && event->buttons() == Qt::LeftButton)
	{
		// Make sure shift is not being pressed
		if ((event->modifiers() & Qt::ShiftModifier) == 0)
		{
			QPointF difference = m_ClickPos - mapToScene(event->pos());
			setSceneRect(sceneRect().translated(difference.x(), difference.y()));
		}
	}
}

void KEGraphView::drawBackground(QPainter* painter, const QRectF& r)
{
	QGraphicsView::drawBackground(painter, r);

	auto drawGrid =	[&](double gridStep)
	{
		QRect   windowRect = rect();
		QPointF tl = mapToScene(windowRect.topLeft());
		QPointF br = mapToScene(windowRect.bottomRight());

		double left = std::floor(tl.x() / gridStep - 0.5);
		double right = std::floor(br.x() / gridStep + 1.0);
		double bottom = std::floor(tl.y() / gridStep - 0.5);
		double top = std::floor(br.y() / gridStep + 1.0);

		// vertical lines
		for (int xi = int(left); xi <= int(right); ++xi)
		{
			QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep, top * gridStep);
			painter->drawLine(line);
		}

		// horizontal lines
		for (int yi = int(bottom); yi <= int(top); ++yi)
		{
			QLineF line(left * gridStep, yi * gridStep, right * gridStep, yi * gridStep);
			painter->drawLine(line);
		}
	};

	QPen pfine(QBrush(QColor(60, 60, 60)), 1.0);
	painter->setPen(pfine);
	drawGrid(15);	
	QPen p(QBrush(QColor(25, 25, 25)), 1.0);
	painter->setPen(p);
	drawGrid(150);
}

void KEGraphView::showEvent(QShowEvent *event)
{
	m_Scene->setSceneRect(this->rect());
	QGraphicsView::showEvent(event);
}