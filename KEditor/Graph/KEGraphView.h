#pragma once
#include <QtWidgets/QGraphicsView>
#include "KEGraphConfig.h"

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
	KEGraphScenePtr m_Scene;
	QPointF m_ClickPos;

	virtual void contextMenuEvent(QContextMenuEvent *event) override;
	virtual void wheelEvent(QWheelEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void drawBackground(QPainter* painter, const QRectF& r) override;
	virtual void showEvent(QShowEvent *event) override;
};