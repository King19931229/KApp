#pragma once
#include <QWidget>
#include "KEngine/Interface/IKEngine.h"
#include "KBase/Publish/KInput.h"
#include "KERenderWidget.h"

class KEQtRenderWindow;

class KEMainRenderWidget : public KERenderWidget
{
	Q_OBJECT
protected:
public:
	KEMainRenderWidget(QWidget* pParent = NULL);
	~KEMainRenderWidget();

	bool Init(IKEnginePtr& engine);
	bool UnInit();

	// 重写基类函数
protected:
	virtual void paintEvent(QPaintEvent *event) override;
	virtual void keyPressEvent(QKeyEvent *event) override;
	virtual void keyReleaseEvent(QKeyEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
};