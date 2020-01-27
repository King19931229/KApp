#include "KEGraphPainter.h"
#include "Node/KEGraphNodeStyle.h"
#include "Connection/KEGraphConnectionStyle.h"
#include <QIcon>

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
	QColor color = graphicsObject.isSelected() ? KEGraphNodeStyle::SelectedBoundaryColor : KEGraphNodeStyle::NormalBoundaryColor;
	if (geom.Hovered())
	{
		QPen p(color, KEGraphNodeStyle::HoveredPenWidth);
		painter->setPen(p);
	}
	else
	{
		QPen p(color, KEGraphNodeStyle::PenWidth);
		painter->setPen(p);
	}

	QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(2.0, geom.Height()));

	gradient.setColorAt(0.0, KEGraphNodeStyle::GradientColor0);
	gradient.setColorAt(0.03, KEGraphNodeStyle::GradientColor1);
	gradient.setColorAt(0.97, KEGraphNodeStyle::GradientColor2);
	gradient.setColorAt(1.0, KEGraphNodeStyle::GradientColor3);

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

void KEGraphPainter::DrawSketchLine(QPainter * painter, KEGraphConnectionControl const & connection)
{

}

void KEGraphPainter::DrawHoveredOrSelected(QPainter * painter, KEGraphConnectionControl const & connection)
{
	KEGraphConnectionGeometry const& geom = connection.ConnectionGeometry();
	bool const hovered = geom.Hovered();

	const KEGraphConnectionView* graphicsObject = connection.GetConnectionView();

	bool const selected = graphicsObject->isSelected();

	// drawn as a fat background
	if (hovered || selected)
	{
		QPen p;

		double const lineWidth = KEGraphConnectionStyle::LineWidth;

		p.setWidth(2 * lineWidth);
		p.setColor(selected ?
			KEGraphConnectionStyle::SelectedHaloColor :
			KEGraphConnectionStyle::HoveredColor);

		painter->setPen(p);
		painter->setBrush(Qt::NoBrush);

		// cubic spline
		auto cubic = CubicPath(geom);
		painter->drawPath(cubic);
	}
}

void KEGraphPainter::DrawNormalLine(QPainter * painter, KEGraphConnectionControl const & connection)
{
	KEGraphConnectionState const& state = connection.ConnectionState();

	if (state.RequiresPort())
		return;

	// colors
	QColor normalColorOut = KEGraphConnectionStyle::NormalColor();
	QColor normalColorIn = KEGraphConnectionStyle::NormalColor();
	QColor selectedColor = KEGraphConnectionStyle::SelectedColor;

	bool gradientColor = false;

	if (KEGraphConnectionStyle::UseDataDefinedColors)
	{
		KEGraphNodeDataType dataTypeOut = connection.DataType(PT_OUT);
		KEGraphNodeDataType dataTypeIn = connection.DataType(PT_IN);

		gradientColor = (dataTypeOut.id != dataTypeIn.id);

		normalColorOut = KEGraphConnectionStyle::NormalColor(dataTypeOut.id);
		normalColorIn = KEGraphConnectionStyle::NormalColor(dataTypeIn.id);
		selectedColor = normalColorOut.darker(200);
	}

	// geometry
	KEGraphConnectionGeometry const& geom = connection.ConnectionGeometry();

	double const lineWidth = KEGraphConnectionStyle::LineWidth;

	// draw normal line
	QPen p;

	p.setWidth(lineWidth);

	const KEGraphConnectionView* graphicsObject = connection.GetView();
	bool const selected = graphicsObject->isSelected();

	auto cubic = CubicPath(geom);
	if (gradientColor)
	{
		painter->setBrush(Qt::NoBrush);

		QColor c = normalColorOut;
		if (selected)
			c = c.darker(200);
		p.setColor(c);
		painter->setPen(p);

		unsigned int const segments = 60;

		for (unsigned int i = 0ul; i < segments; ++i)
		{
			double ratioPrev = double(i) / segments;
			double ratio = double(i + 1) / segments;

			if (i == segments / 2)
			{
				QColor c = normalColorIn;
				if (selected)
					c = c.darker(200);

				p.setColor(c);
				painter->setPen(p);
			}
			painter->drawLine(cubic.pointAtPercent(ratioPrev),
				cubic.pointAtPercent(ratio));
		}

		// TODO
		{
			QIcon icon(":convert.png");

			QPixmap pixmap = icon.pixmap(QSize(22, 22));
			painter->drawPixmap(cubic.pointAtPercent(0.50) - QPoint(pixmap.width() / 2,
				pixmap.height() / 2),
				pixmap);

		}
	}
	else
	{
		p.setColor(normalColorOut);

		if (selected)
		{
			p.setColor(selectedColor);
		}

		painter->setPen(p);
		painter->setBrush(Qt::NoBrush);

		painter->drawPath(cubic);
	}
}

void KEGraphPainter::Paint(QPainter* painter, KEGraphConnectionControl const& connection)
{
	DrawHoveredOrSelected(painter, connection);

	DrawSketchLine(painter, connection);

	DrawNormalLine(painter, connection);

#ifdef NODE_DEBUG_DRAWING
	DebugDrawing(painter, connection);
#endif

	// draw end points
	KEGraphConnectionGeometry const& geom = connection.ConnectionGeometry();

	QPointF const & source = geom.Source();
	QPointF const & sink = geom.Sink();

	double const pointDiameter = KEGraphConnectionStyle::PointDiameter;

	painter->setPen(KEGraphConnectionStyle::ConstructionColor);
	painter->setBrush(KEGraphConnectionStyle::ConstructionColor);
	double const pointRadius = pointDiameter / 2.0;
	painter->drawEllipse(source, pointRadius, pointRadius);
	painter->drawEllipse(sink, pointRadius, pointRadius);
}

QPainterPath KEGraphPainter::CubicPath(KEGraphConnectionGeometry const& geom)
{
	QPointF const& source = geom.Source();
	QPointF const& sink = geom.Sink();

	auto c1c2 = geom.PointsC1C2();

	// cubic spline
	QPainterPath cubic(source);

	cubic.cubicTo(c1c2.first, c1c2.second, sink);

	return cubic;
}

QPainterPath KEGraphPainter::GetPainterStroke(KEGraphConnectionGeometry const& geom)
{
	auto cubic = CubicPath(geom);

	const QPointF& source = geom.Source();
	QPainterPath result(source);

	unsigned segments = 20;

	for (auto i = 0ul; i < segments; ++i)
	{
		double ratio = double(i + 1) / segments;
		result.lineTo(cubic.pointAtPercent(ratio));
	}

	QPainterPathStroker stroker; stroker.setWidth(10.0);

	return stroker.createStroke(result);
}