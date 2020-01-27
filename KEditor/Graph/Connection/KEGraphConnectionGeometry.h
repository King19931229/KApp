#pragma once

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include "Graph/KEGraphConfig.h"

class KEGraphConnectionGeometry
{
protected:
	QPointF m_In;
	QPointF m_Out;
	double m_LineWidth;
	bool m_Hovered;
public:
	KEGraphConnectionGeometry();
	~KEGraphConnectionGeometry();

	const QPointF& GetEndPoint(PortType portType) const;
	void SetEndPoint(PortType portType, const QPointF& point);
	void MoveEndPoint(PortType portType, const QPointF& offset);
	QRectF BoundingRect() const;
	std::pair<QPointF, QPointF>	PointsC1C2() const;

	inline QPointF Source() const { return m_Out; }
	inline QPointF Sink() const { return m_In; }
	inline double LineWidth() const { return m_LineWidth; }
	inline bool Hovered() const { return m_Hovered; }
	inline void SetHovered(bool hovered) { m_Hovered = hovered; }
};