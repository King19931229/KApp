#pragma once
#include <QWidget>
#include "KEngine/Interface/IKEngine.h"
#include "KBase/Publish/KInput.h"

class KEQtRenderWindow;
class KERenderWidget : public QWidget
{
	Q_OBJECT
protected:
	IKEngine* m_Engine;
	IKRenderDevice* m_RenderDevice;
	KEQtRenderWindow* m_RenderWindow;
	bool m_KeyDown[INPUT_KEY_COUNT];

	static bool QtButtonToMouseButton(Qt::MouseButton button, InputMouseButton& mouseButton);
	static bool QtKeyToInputKeyboard(Qt::Key button, InputKeyboard& keyboard);

	void HandleMouseEvent(QMouseEvent *event, InputAction action);
	void HandleKeyEvent(QKeyEvent *event, InputAction action);
	void HandleFocusEvent(QFocusEvent *event);
public:
	KERenderWidget(QWidget* pParent = NULL);
	~KERenderWidget();

	// 重写基类函数
public:
	virtual QPaintEngine *paintEngine() const override { return NULL; }
protected:
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void paintEvent(QPaintEvent *event) override;

	virtual void keyPressEvent(QKeyEvent *event) override;
	virtual void keyReleaseEvent(QKeyEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void wheelEvent(QWheelEvent *event) override;

	virtual void focusInEvent(QFocusEvent *event) override;
	virtual void focusOutEvent(QFocusEvent *event) override;

	virtual void dragEnterEvent(QDragEnterEvent *event) override;
	virtual void dragMoveEvent(QDragMoveEvent *event) override;
	virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
	// virtual void dropEvent(QDropEvent *event) override;
};