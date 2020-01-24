#include "KEGraphNodeGeometry.h"

KEGraphNodeGeometry::KEGraphNodeGeometry(KEGraphNodeModelPtr& model)
	: m_Width(100),
	m_Height(150),
	m_EntryWidth(70),
	m_EntryHeight(20),
	m_InputPortWidth(70),
	m_OutputPortWidth(70),
	m_Spacing(20),
	m_Hovered(false),
	m_Model(model),
	m_FontMetrics(QFont()),
	m_BoldFontMetrics(QFont())
{
	QFont f;
	f.setBold(true);
	m_BoldFontMetrics = QFontMetrics(f);
}

KEGraphNodeGeometry::~KEGraphNodeGeometry()
{

}

QRectF KEGraphNodeGeometry::boundingRect() const
{
	return QRectF(0, 0, m_Width, m_Height);
}