#pragma once
#include <QtWidgets/QGraphicsView>

class KEGraphScene;
class KEGraphView : public QGraphicsView
{
	Q_OBJECT
public:
	KEGraphView(QWidget *parent = nullptr);
	~KEGraphView();

public Q_SLOTS:
	void ScaleUp();
	void ScaleDown();
protected:
	KEGraphScene* m_Scene;
	QPointF m_ClickPos;
	virtual void wheelEvent(QWheelEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void drawBackground(QPainter* painter, const QRectF& r);
	virtual void showEvent(QShowEvent *event);
};