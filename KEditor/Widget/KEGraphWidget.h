#pragma once
#include <QWidget>
#include <QMenuBar>

class KEGraphView;
class KEGraphWidget : public QWidget
{
	Q_OBJECT
protected:
	QMenuBar* m_MenuBar;
	KEGraphView* m_View;
public:
	KEGraphWidget();
	~KEGraphWidget();

	void Test();
};