#pragma once
#include <QtWidgets/QGraphicsView>
#include "KEGraphConfig.h"

class KEGraphView : public QGraphicsView
{
	Q_OBJECT
	typedef std::unique_ptr<QAction> QActionPtr;
public:
	KEGraphView(QWidget *parent = nullptr);
	virtual ~KEGraphView();

	void RegisterModel(const QString& name, KEGraphNodeModelCreateFunc func);
	void UnRegisterGraphModel(const QString& name);
public Q_SLOTS:
	void OnNodeContextMenu(KEGraphNodeControl* n, const QPointF& pos);
	void OnConnectionContextMenu(KEGraphConnectionControl* conn, const QPointF& pos);

	void ScaleUp();
	void ScaleDown();
protected:
	KEGraphScenePtr m_Scene;
	QPointF m_ClickPos;

	QActionPtr m_ClearSelectionAction;
	QActionPtr m_DeleteSelectionAction;

	void DeleteSelectedNodes();

	virtual void contextMenuEvent(QContextMenuEvent *event) override;
	virtual void wheelEvent(QWheelEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void drawBackground(QPainter* painter, const QRectF& r) override;
	virtual void showEvent(QShowEvent *event) override;
};