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
	bool m_bInit;
public:
	KEGraphWidget(QWidget* parent = Q_NULLPTR);
	virtual ~KEGraphWidget();
	virtual KEGraphView* CreateViewImpl() = 0;
	virtual bool Init();
	virtual bool UnInit();

	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
};