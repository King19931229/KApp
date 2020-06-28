#pragma once
#include "Widget/KERenderWidget.h"

class KEMaterialRenderWidget : public KERenderWidget
{
	Q_OBJECT
protected:
	IKRenderWindowPtr m_SecordaryWindow;
public:
	KEMaterialRenderWidget(QWidget* pParent = NULL);
	~KEMaterialRenderWidget();

	bool Init(IKEnginePtr& engine);
	bool UnInit();

	// 重写基类函数
protected:
	virtual void paintEvent(QPaintEvent *event) override;
};