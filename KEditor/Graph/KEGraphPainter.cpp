#include "KEGraphPainter.h"
#include "Node/KEGraphNodeStyle.h"

void KEGraphPainter::Paint(QPainter* painter, KEGraphNodeControl& node, KEGraphScene const& scene)
{
	KEGraphNodeGeometry& geom = node.GetGeometry();
	KEGraphNodeState const & state = node.GetNodeState();
	KEGraphNodeView const & graphicsObject = *(node.GetView());

	geom.RecalculateSize(painter->font());

	//--------------------------------------------
	KEGraphNodeModel const * model = node.GetModel();

	DrawNodeRect(painter, geom, model, graphicsObject);

	DrawConnectionPoints(painter, geom, state, model, scene);

	DrawFilledConnectionPoints(painter, geom, state, model);

	DrawModelName(painter, geom, state, model);

	DrawEntryLabels(painter, geom, state, model);

	DrawResizeRect(painter, geom, model);

	DrawValidationRect(painter, geom, model, graphicsObject);
}

void KEGraphPainter::DrawNodeRect(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const* model, KEGraphNodeView const & graphicsObject)
{
	// TODO
	QPen p(QColor(255, 255, 255), 1.0f);
	painter->setPen(p);

	QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(2.0, geom.Height()));

	gradient.setColorAt(0.0, QColor("mintcream"));
	gradient.setColorAt(0.03, QColor("mintcream"));
	gradient.setColorAt(0.97, QColor("mintcream"));
	gradient.setColorAt(1.0, QColor("mintcream"));
	painter->setBrush(gradient);

	float diam = KEGraphNodeStyle::ConnectionPointDiameter;
	QRectF boundary(-diam, -diam, 2.0 * diam + geom.Width(), 2.0 * diam + geom.Height());
	double const radius = 3.0;
	painter->drawRoundedRect(boundary, radius, radius);
}

void KEGraphPainter::DrawModelName(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model)
{
	Q_UNUSED(state);

	if (!model->CaptionVisible())
		return;

	QString const &name = model->Caption();

	QFont f = painter->font();

	f.setBold(true);

	QFontMetrics metrics(f);

	auto rect = metrics.boundingRect(name);

	QPointF position((geom.Width() - rect.width()) / 2.0, (geom.Spacing() + geom.EntryHeight()) / 3.0);

	painter->setFont(f);
	// TODO
	painter->setPen(QColor(0, 0, 0));
	painter->drawText(position, name);

	f.setBold(false);
	painter->setFont(f);
}

void KEGraphPainter::DrawEntryLabels(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model)
{

}

void KEGraphPainter::DrawConnectionPoints(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model, KEGraphScene const & scene)
{

}

void KEGraphPainter::DrawFilledConnectionPoints(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model)
{

}

void KEGraphPainter::DrawResizeRect(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const * model)
{

}

void KEGraphPainter::DrawValidationRect(QPainter * painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const * model, KEGraphNodeView const & graphicsObject)
{

}