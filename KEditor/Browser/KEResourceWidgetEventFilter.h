#pragma once
#include <QObject>
#include <QEvent>

class KEResourceWidgetEventFilter : public QObject
{
	Q_OBJECT
public:
	bool eventFilter(QObject *object, QEvent *event) override
	{
		QEvent::Type type = event->type();
		if (type == QEvent::HoverEnter ||
			type == QEvent::HoverLeave ||
			type == QEvent::HoverMove ||
			type == QEvent::MouseMove ||
			type == QEvent::Enter ||
			type == QEvent::Leave)
		{
			return true;
		}
		return false;
	}
};