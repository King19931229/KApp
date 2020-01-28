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

	QPointF m_DraggingPos;

	unsigned int CaptionHeight() const;
	unsigned int CaptionWidth() const;
	unsigned int PortWidth(PortType portType) const;
public:
	KEGraphNodeGeometry(KEGraphNodeModelPtr& model);
	~KEGraphNodeGeometry();

	inline unsigned int Width() const { return m_Width; }
	inline unsigned int Height() const { return m_Height; }
	inline unsigned int Spacing() const { return m_Spacing; }
	inline bool Hovered() const { return m_Hovered; }
	inline unsigned int EntryWidth() const { return m_EntryWidth; }
	inline unsigned int EntryHeight() const { return m_EntryHeight; }
	inline unsigned int	NumSources() const { return m_nSources; }
	inline unsigned int	NumSinks() const { return m_nSinks; }
	inline const QPointF& DraggingPos() const { return m_DraggingPos; }

	inline void	SetEntryHeight(unsigned int h) { m_EntryHeight = h; }
	inline void SetEntryWidth(unsigned int w) { m_EntryWidth = w; }
	inline void	SetSpacing(unsigned int s) { m_Spacing = s; }
	inline void	SetHovered(bool h) { m_Hovered = h; }
	inline void SetDraggingPosition(QPointF const& pos) { m_DraggingPos = pos; }

	QRectF BoundingRect() const;
	/// Updates size unconditionally
	void RecalculateSize();
	/// Updates size if the QFontMetrics is changed
	void RecalculateSize(QFont const &font);

	QRect ResizeRect() const;
	QPointF WidgetPosition() const;

	QPointF	PortScenePosition(PortIndexType index, PortType portType, const QTransform &t = QTransform()) const;
	PortIndexType CheckHitScenePoint(PortType portType, QPointF point, QTransform const & t = QTransform()) const;
};