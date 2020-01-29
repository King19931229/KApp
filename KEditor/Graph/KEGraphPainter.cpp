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

	painter->setPen(KEGraphNodeStyle::FontColor);
	painter->drawText(position, name);

	f.setBold(false);
	painter->setFont(f);
}

void KEGraphPainter::DrawEntryLabels(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model)
{
	QFontMetrics const & metrics = painter->fontMetrics();

	for (PortType portType : {PT_OUT, PT_IN})
	{
		auto& entries = state.GetEntries(portType);

		size_t n = entries.size();

		for (size_t i = 0; i < n; ++i)
		{
			QPointF p = geom.PortScenePosition(i, portType);

			if (entries[i].empty())
				painter->setPen(KEGraphNodeStyle::FontColorFaded);
			else
				painter->setPen(KEGraphNodeStyle::FontColor);

			QString s;

			if (model->PortCaptionVisible(portType, i))
			{
				s = model->PortCaption(portType, i);
			}
			else
			{
				s = model->DataType(portType, i).name;
			}

			auto rect = metrics.boundingRect(s);

			p.setY(p.y() + rect.height() / 4.0);

			switch (portType)
			{
			case PT_IN:
				p.setX(5.0);
				break;

			case PT_OUT:
				p.setX(geom.Width() - 5.0 - rect.width());
				break;

			default:
				break;
			}

			painter->drawText(p, s);
		}
	}
}

void KEGraphPainter::DrawConnectionPoints(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model, KEGraphScene const & scene)
{
	float diameter = KEGraphNodeStyle::ConnectionPointDiameter;
	double reducedDiameter = diameter * 0.6;

	for (PortType portType : {PT_OUT, PT_IN})
	{
		size_t n = state.GetEntries(portType).size();

		for (unsigned int i = 0; i < n; ++i)
		{
			QPointF p = geom.PortScenePosition(i, portType);

			auto const & dataType = model->DataType(portType, i);

			bool canConnect = (state.GetEntries(portType)[i].empty() ||
				(portType == PT_OUT && model->PortOutConnectionPolicy(i) == CP_MANY));

			double r = 1.0;
			if (state.IsReacting() && canConnect && portType == state.ReactingPortType())
			{

				QPointF diff = geom.DraggingPos() - p;
				double dist = std::sqrt(QPointF::dotProduct(diff, diff));
				bool typeConvertable = false;

				{
					if (portType == PT_IN)
					{
						// TODO
						// typeConvertable = scene.registry().getTypeConverter(state.reactingDataType(), dataType) != nullptr;
					}
					else
					{
						// TODO
						// typeConvertable = scene.registry().getTypeConverter(dataType, state.reactingDataType()) != nullptr;
					}
				}

				if (state.ReactingDataType().id == dataType.id || typeConvertable)
				{
					double const thres = 40.0;
					r = (dist < thres) ?
						(2.0 - dist / thres) :
						1.0;
				}
				else
				{
					double const thres = 80.0;
					r = (dist < thres) ?
						(dist / thres) :
						1.0;
				}
			}

			if (KEGraphConnectionStyle::UseDataDefinedColors)
			{
				painter->setBrush(KEGraphConnectionStyle::NormalColor(dataType.id));
			}
			else
			{
				painter->setBrush(KEGraphNodeStyle::ConnectionPointColor);
			}

			painter->drawEllipse(p, reducedDiameter * r, reducedDiameter * r);
		}
	}
}

void KEGraphPainter::DrawFilledConnectionPoints(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeState const& state, KEGraphNodeModel const * model)
{
	float diameter = KEGraphNodeStyle::ConnectionPointDiameter;

	for (PortType portType : {PT_OUT, PT_IN})
	{
		size_t n = state.GetEntries(portType).size();
		for (size_t i = 0; i < n; ++i)
		{
			QPointF p = geom.PortScenePosition(i, portType);

			if (!state.GetEntries(portType)[i].empty())
			{
				auto const & dataType = model->DataType(portType, i);

				if (KEGraphConnectionStyle::UseDataDefinedColors)
				{
					QColor const c = KEGraphConnectionStyle::NormalColor(dataType.id);
					painter->setPen(c);
					painter->setBrush(c);
				}
				else
				{
					painter->setPen(KEGraphNodeStyle::FilledConnectionPointColor);
					painter->setBrush(KEGraphNodeStyle::FilledConnectionPointColor);
				}

				painter->drawEllipse(p, diameter * 0.4, diameter * 0.4);
			}
		}
	}
}

void KEGraphPainter::DrawResizeRect(QPainter* painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const * model)
{
	if (model->Resizable())
	{
		painter->setBrush(Qt::gray);
		painter->drawEllipse(geom.ResizeRect());
	}
}

void KEGraphPainter::DrawValidationRect(QPainter * painter, KEGraphNodeGeometry& geom, KEGraphNodeModel const * model, KEGraphNodeView const & graphicsObject)
{

}

void KEGraphPainter::DrawSketchLine(QPainter * painter, KEGraphConnectionControl const & connection)
{
	KEGraphConnectionState const& state = connection.ConnectionState();

	if (state.RequiresPort())
	{
		QPen p;
		p.setWidth(KEGraphConnectionStyle::ConstructionLineWidth);
		p.setColor(KEGraphConnectionStyle::ConstructionColor);
		p.setStyle(Qt::DashLine);

		painter->setPen(p);
		painter->setBrush(Qt::NoBrush);

		KEGraphConnectionGeometry const& geom = connection.ConnectionGeometry();

		auto cubic = CubicPath(geom);
		// cubic spline
		painter->drawPath(cubic);
	}
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