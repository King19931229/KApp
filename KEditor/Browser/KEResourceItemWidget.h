#pragma once
#include "ui_KEResourceItemWidget.h"
#include "Browser/KEResourceWidgetEventFilter.h"

class KEResourceItemWidget : public QWidget
{
	Q_OBJECT
protected:
	QWidget* m_Browser;
	KEResourceWidgetEventFilter m_Filter;
public:
	Ui::KEResourceItemWidget ui;
	KEResourceItemWidget(QWidget *parent = Q_NULLPTR);
	~KEResourceItemWidget();

	QSize sizeHint() const override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
protected Q_SLOTS:
	void ShowContextMenu(const QPoint& pos);
	void OnOpenItemEditor();
	void OnOpenFileExternal();
	void OnOpenFileLocation();
	void OnDeleteFile();
	void ConvertIntoMesh();
};