#pragma once
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtGui/QTransform>
#include <QtGui/QFontMetrics>

#include "Graph/KEGraphPredefine.h"

class KEGraphNodeGeometry
{
protected:
	KEGraphNodeModelPtr& m_Model;
	unsigned int m_Width;
	unsigned int m_Height;
	unsigned int m_EntryWidth;
	unsigned int m_EntryHeight;
	unsigned int m_InputPortWidth;
	unsigned int m_OutputPortWidth;
	unsigned int m_Spacing;
	bool m_Hovered;

	QFontMetrics m_FontMetrics;
	QFontMetrics m_BoldFontMetrics;
public:
	KEGraphNodeGeometry(KEGraphNodeModelPtr& model);
	~KEGraphNodeGeometry();

	QRectF boundingRect() const;
};