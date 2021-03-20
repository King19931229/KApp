#include "KEGraphNodeGeometry.h"
#include "KEGraphNodeModel.h"
#include "KEGraphNodeStyle.h"

#include <algorithm>
#include <cmath>

KEGraphNodeGeometry::KEGraphNodeGeometry(KEGraphNodeModelPtr& model)
	: m_Width(100),
	m_Height(150),
	m_EntryWidth(70),
	m_EntryHeight(20),
	m_InputPortWidth(70),
	m_OutputPortWidth(70),
	m_Spacing(20),
	m_Hovered(false),
	m_nSources(model->NumPorts(PT_OUT)),
	m_nSinks(model->NumPorts(PT_IN)),
	m_DraggingPos(-1000, -1000),
	m_DataModel(model),
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

unsigned int KEGraphNodeGeometry::CaptionHeight() const
{
	if (!m_DataModel->CaptionVisible())
		return 0;
	QString name = m_DataModel->Caption();
	return m_BoldFontMetrics.boundingRect(name).height();
}

unsigned int KEGraphNodeGeometry::CaptionWidth() const
{
	if (!m_DataModel->CaptionVisible())
		return 0;
	QString name = m_DataModel->Caption();
	return m_BoldFontMetrics.boundingRect(name).width();
}

unsigned int KEGraphNodeGeometry::PortWidth(PortType portType) const
{
	unsigned width = 0;

	for (unsigned int i = 0; i < m_DataModel->NumPorts(portType); ++i)
	{
		QString name;

		if (m_DataModel->PortCaptionVisible(portType, i))
		{
			name = m_DataModel->PortCaption(portType, i);
		}
		else
		{
			name = m_DataModel->DataType(portType, i).name;
		}

		width = std::max(unsigned(m_FontMetrics.width(name)), width);
	}

	return width;
}

QRectF KEGraphNodeGeometry::BoundingRect() const
{
	double addon = 4 * KEGraphNodeStyle::ConnectionPointDiameter;
	return QRectF(0 - addon, 0 - addon, m_Width + 2 * addon, m_Height + 2 * addon);
}

void KEGraphNodeGeometry::RecalculateSize()
{
	m_EntryHeight = m_FontMetrics.height();

	{
		unsigned int maxNumOfEntries = std::max(m_nSinks, m_nSources);
		unsigned int step = m_EntryHeight + m_Spacing;
		m_Height = step * maxNumOfEntries;
	}

	if (auto w = m_DataModel->EmbeddedWidget())
	{
		m_Height = std::max(m_Height, static_cast<unsigned>(w->height()));
	}

	m_Height += CaptionHeight();

	m_InputPortWidth = PortWidth(PortType::PT_IN);
	m_OutputPortWidth = PortWidth(PortType::PT_OUT);

	m_Width = m_InputPortWidth + m_OutputPortWidth + 2 * m_Spacing;

	if (auto w = m_DataModel->EmbeddedWidget())
	{
		m_Width += w->width();
	}

	m_Width = std::max(m_Width, CaptionWidth());
	/* TODO
	if (m_DataModel->ValidationState() != NodeValidationState::Valid)
	{
		m_Width = std::max(m_Width, ValidationWidth());
		m_Height += ValidationHeight() + m_Spacing;
	}
	*/
}

void KEGraphNodeGeometry::RecalculateSize(QFont const & font) 
{
	QFontMetrics fontMetrics(font);
	QFont boldFont = font;

	boldFont.setBold(true);

	QFontMetrics boldFontMetrics(boldFont);

	if (m_BoldFontMetrics != boldFontMetrics)
	{
		m_FontMetrics = fontMetrics;
		m_BoldFontMetrics = boldFontMetrics;

		RecalculateSize();
	}
}

QRect KEGraphNodeGeometry::ResizeRect() const
{
	unsigned int rectSize = 7;

	return QRect(m_Width - rectSize,
		m_Height - rectSize,
		rectSize,
		rectSize);
}

QPointF KEGraphNodeGeometry::WidgetPosition() const
{
	if (auto w = m_DataModel->EmbeddedWidget())
	{
		/* TODO
		if (m_DataModel->validationState() != NodeValidationState::Valid)
		{
			return QPointF(m_Spacing + PortWidth(PT_IN),
				(CaptionHeight() + m_Height - ValidationHeight() - m_Spacing - w->height()) / 2.0);
		}
		*/
		return QPointF(m_Spacing + PortWidth(PT_IN), (CaptionHeight() + m_Height - w->height()) / 2.0);
	}

	return QPointF();
}

QPointF	KEGraphNodeGeometry::PortScenePosition(PortIndexType index, PortType portType, const QTransform& t) const
{
	unsigned int step = m_EntryHeight + m_Spacing;

	QPointF result;

	double totalHeight = 0.0;
	totalHeight += CaptionHeight();
	totalHeight += step * index;
	// TODO: why?
	totalHeight += step / 2.0;

	switch (portType)
	{
		case PT_OUT:
		{
			double x = m_Width + KEGraphNodeStyle::ConnectionPointDiameter;
			result = QPointF(x, totalHeight);
			break;
		}
		case PT_IN:
		{
			double x = 0.0 - KEGraphNodeStyle::ConnectionPointDiameter;
			result = QPointF(x, totalHeight);
			break;
		}
		default:
			break;
	}

	return t.map(result);
}

PortIndexType KEGraphNodeGeometry::CheckHitScenePoint(PortType portType, QPointF scenePoint, QTransform const & sceneTransform) const
{
	PortIndexType result = INVALID_PORT_INDEX;

	if (portType == PT_NONE)
		return result;

	double const tolerance = 2.0 * KEGraphNodeStyle::ConnectionPointDiameter;

	unsigned int const nItems = m_DataModel->NumPorts(portType);

	for (unsigned int i = 0; i < nItems; ++i)
	{
		auto pp = PortScenePosition(i, portType, sceneTransform);

		QPointF p = pp - scenePoint;
		double distance = std::sqrt(QPointF::dotProduct(p, p));

		if (distance < tolerance)
		{
			result = PortIndexType(i);
			break;
		}
	}

	return result;
}