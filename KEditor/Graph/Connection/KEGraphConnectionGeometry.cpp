#include "KEGraphConnectionGeometry.h"
#include "Graph/Connection/KEGraphConnectionStyle.h"
#include <assert.h>

KEGraphConnectionGeometry::KEGraphConnectionGeometry()
	: m_In(0, 0),
	m_Out(0, 0),
	m_LineWidth(3.0),
	m_Hovered(false)
{
}

KEGraphConnectionGeometry::~KEGraphConnectionGeometry()
{
}

const QPointF& KEGraphConnectionGeometry::GetEndPoint(PortType portType) const
{
	assert(portType != PT_NONE);
	return portType == PT_OUT ? m_Out : m_In;
}

void KEGraphConnectionGeometry::SetEndPoint(PortType portType, QPointF const& point)
{
	switch (portType)
	{
	case PT_IN:
		m_In = point;
		break;
	case PT_OUT:
		m_Out = point;
		break;
	default:
		break;
	}
}

void KEGraphConnectionGeometry::MoveEndPoint(PortType portType, const QPointF& offset)
{
	switch (portType)
	{
	case PT_IN:
		m_In += offset;
		break;
	case PT_OUT:
		m_Out += offset;
		break;
	default:
		break;
	}
}

QRectF KEGraphConnectionGeometry::BoundingRect() const
{
	auto points = PointsC1C2();

	QRectF basicRect = QRectF(m_Out, m_In).normalized();

	QRectF c1c2Rect = QRectF(points.first, points.second).normalized();

	float const diam = KEGraphConnectionStyle::PointDiameter;

	QRectF commonRect = basicRect.united(c1c2Rect);

	const QPointF cornerOffset(diam, diam);

	commonRect.setTopLeft(commonRect.topLeft() - cornerOffset);
	commonRect.setBottomRight(commonRect.bottomRight() + 2 * cornerOffset);

	return commonRect;
}

std::pair<QPointF, QPointF>	KEGraphConnectionGeometry::PointsC1C2() const
{
	const double defaultOffset = 200;

	double xDistance = m_In.x() - m_Out.x();

	double horizontalOffset = std::min(defaultOffset, std::abs(xDistance));

	double verticalOffset = 0;

	double ratioX = 0.5;

	if (xDistance <= 0)
	{
		double yDistance = m_In.y() - m_Out.y() + 20;

		double vector = yDistance < 0 ? -1.0 : 1.0;

		verticalOffset = std::min(defaultOffset, std::abs(yDistance)) * vector;

		ratioX = 1.0;
	}

	horizontalOffset *= ratioX;

	QPointF c1(m_Out.x() + horizontalOffset, m_Out.y() + verticalOffset);
	QPointF c2(m_In.x() - horizontalOffset, m_In.y() - verticalOffset);

	return std::make_pair(c1, c2);
}