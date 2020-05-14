#pragma once
#include <QWidget>

class KEReflectPropertyWidget : public QWidget
{
	Q_OBJECT
protected:
	QWidget* m_MainWindow;
public:
	KEReflectPropertyWidget(QWidget *parent = Q_NULLPTR);
	~KEReflectPropertyWidget();

	QSize sizeHint() const override;

	bool Init();
	bool UnInit();
};