#pragma once
#include <QWidget>

class KEGraphView;
class KEGraphWidget : public QWidget
{
	Q_OBJECT
protected:
	KEGraphView* m_View;
public:
	KEGraphWidget();
	~KEGraphWidget();
};