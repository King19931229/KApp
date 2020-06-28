#pragma once
#include <QDockWidget>
#include <QMainWindow>
#include "Widget/Material/KEMaterialRenderWidget.h"

class KEMaterialEditWindow : public QMainWindow
{
	Q_OBJECT
protected:
	QWidget* m_MainWindow;
	KEMaterialRenderWidget* m_RenderWidget;

	bool Init();
	bool UnInit();
public:
	KEMaterialEditWindow(QWidget *parent = Q_NULLPTR);
	~KEMaterialEditWindow();

	bool SetEditTarget(const std::string& path);

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;
};