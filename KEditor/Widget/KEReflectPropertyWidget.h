#pragma once
#include "KEditorConfig.h"
#include <QMainWindow>
#include <QVBoxLayout>

class KEReflectObjectWidget;
class KEReflectObjectTreeView;
class KEReflectObjectTreeModel;

class KEReflectPropertyWidget : public QMainWindow
{
	Q_OBJECT
protected:
	QWidget* m_MainWindow;
	KEReflectObjectTreeView* m_TreeView;
	KEReflectObjectTreeModel* m_TreeModel;
public:
	KEReflectPropertyWidget(QWidget *parent = Q_NULLPTR);
	~KEReflectPropertyWidget();

	QSize sizeHint() const override;

	void SetWidget(KEReflectObjectWidget* widget);
	void SetObject(KReflectionObjectBase* reflection);

	bool Init();
	bool UnInit();
};