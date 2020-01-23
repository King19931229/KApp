#pragma once
#include <QWidget>
#include "KRender/Interface/IKRenderCore.h"
#include "KBase/Publish/KInput.h"

class KEQtRenderWindow;
class KERenderWidget : public QWidget
{
	Q_OBJECT
protected:
	IKRenderCore* m_RenderCore;
	KEQtRenderWindow* m_RenderWindow;

	static bool QtButtonToMouseButton(Qt::MouseButton button, InputMouseButton& mouseButton);
	static bool QtKeyToInputKeyboard(Qt::Key button, InputKeyboard& keyboard);

	void HandleMouseEvent(QMouseEvent *event, InputAction action);
	void HandleKeyEvent(QKeyEvent *event, InputAction action);
public:
	KERenderWidget(QWidget* pParent = NULL);
	~KERenderWidget();

	bool Init(IKRenderCorePtr& core);
	bool UnInit();

	// 重写基类函数
public:
	virtual QPaintEngine *paintEngine() const { return NULL; }
protected:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent *event);

	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
};