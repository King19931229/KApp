#pragma once
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtGui/QTransform>
#include <QtGui/QFontMetrics>

#include "Graph/KEGraphConfig.h"

class KEGraphNodeGeometry
{
protected:
	KEGraphNodeModelPtr& m_DataModel;
	unsigned int m_Width;
	unsigned int m_Height;
	unsigned int m_EntryWidth;
	unsigned int m_EntryHeight;
	unsigned int m_InputPortWidth;
	unsigned int m_OutputPortWidth;
	unsigned int m_Spacing;
	bool m_Hovered;

	unsigned int m_nSources;
	unsigned int m_nSinks;

	QFontMetrics m_FontMetrics;
	QFontMetrics m_BoldFontMetrics;

	unsigned int CaptionHeight() const;
	unsigned int CaptionWidth() const;
	unsigned int PortWidth(PortType portType) const;
public:
	KEGraphNodeGeometry(KEGraphNodeModelPtr& model);
	~KEGraphNodeGeometry();

	unsigned int Width() { return m_Width; }
	unsigned int Height() { return m_Height; }
	unsigned int Spacing() { return m_Spacing; }
	unsigned int EntryWidth() { return m_EntryWidth; }
	unsigned int EntryHeight() { return m_EntryHeight; }

	QRectF BoundingRect() const;
	/// Updates size unconditionally
	void RecalculateSize();
	/// Updates size if the QFontMetrics is changed
	void RecalculateSize(QFont const &font);

	QPointF	PortScenePosition(PortIndexType index, PortType portType, const QTransform &t = QTransform()) const;
};