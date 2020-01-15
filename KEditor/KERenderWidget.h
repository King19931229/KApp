#pragma once
#include <QWidget>
#include "KRender/Interface/IKRenderCore.h"

class KERenderWidget : public QWidget
{
	Q_OBJECT
protected:
	IKRenderCore* m_RenderCore;
public:
	KERenderWidget(QWidget* pParent = NULL);
	~KERenderWidget();

	bool Init(IKRenderCorePtr& core);
	bool UnInit();

	// 重写基类函数
public:
	virtual QPaintEngine *paintEngine() const { return NULL; }
private:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent *event);
};