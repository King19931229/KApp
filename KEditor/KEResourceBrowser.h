#pragma once

#include <QHBoxLayout>
#include "ui_KEResourceBrowser.h"

// QDockWidget处理不了Resize 布局无法自动调整大小

class KEResourceBrowser : public QWidget
{
	Q_OBJECT
public:
	KEResourceBrowser(QWidget *parent = Q_NULLPTR);
	~KEResourceBrowser();

	QSize sizeHint() const override;

	bool Init();
	bool UnInit();
protected:
	QHBoxLayout* m_Layout;
	QWidget* m_MainWindow;
private:
	Ui::KEResourceBrowser ui;
};
